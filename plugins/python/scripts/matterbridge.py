#!/usr/bin/env python3
"""
Verlihub Matterbridge Integration Script

Bridges Verlihub DC++ hub chat with Matterbridge, allowing integration with
Mattermost, Slack, IRC, Discord, Telegram, Matrix, and many other platforms.

This script communicates with a Matterbridge API instance to:
- Send Verlihub chat messages to Matterbridge
- Receive messages from Matterbridge and display in hub
- Support bidirectional chat bridging

Admin commands:
  !bridge start              - Start Matterbridge connection
  !bridge stop               - Stop Matterbridge connection  
  !bridge status             - Check connection status
  !bridge config <url>       - Set Matterbridge API URL
  !bridge token <token>      - Set API authentication token
  !bridge gateway <name>     - Set gateway name (default: verlihub)
  !bridge channel <name>     - Set channel name (default: #sublevels)
  !bridge help               - Show help

Configuration:
  Edit the CONFIG section below or use commands to configure.

Requirements:
  pip install requests

Author: Verlihub Team
Version: 1.0.0
"""

import vh
import sys
import os
import time
import threading
import json
from typing import Optional, Dict, Any
from datetime import datetime
from queue import Queue, Empty

# Add venv to path if it exists
script_dir = os.path.dirname(os.path.abspath(__file__))
venv_path = os.path.join(script_dir, 'venv', 'lib', 'python3.12', 'site-packages')
if os.path.exists(venv_path):
    sys.path.insert(0, venv_path)

try:
    import requests
    REQUESTS_AVAILABLE = True
except ImportError:
    REQUESTS_AVAILABLE = False
    print("WARNING: requests library not installed. Run: pip install requests")

# =============================================================================
# Configuration
# =============================================================================

CONFIG = {
    # Matterbridge API endpoint (must be configured in matterbridge.toml)
    "api_url": "http://localhost:4242",
    
    # Optional: API authentication token (if configured in matterbridge)
    "api_token": "",
    
    # Gateway name as configured in matterbridge.toml
    "gateway": "verlihub",
    
    # Channel name to bridge to
    "channel": "#sublevels",
    
    # Hub bot name (how messages from matterbridge appear in hub)
    "bot_nick": "[Bridge]",
    
    # Format for relaying hub messages to matterbridge
    # Available: {nick}, {message}, {hub}, {class}
    "hub_to_bridge_format": "<{nick}> {message}",
    
    # Format for displaying bridge messages in hub
    # Available: {username}, {text}, {protocol}, {channel}
    "bridge_to_hub_format": "[{protocol}] <{username}> {text}",
    
    # Polling interval for messages (seconds)
    "poll_interval": 0.5,
    
    # Connection retry settings
    "retry_interval": 5,
    "max_retries": 10,
    
    # Filter: Don't relay messages from these users
    "ignore_nicks": [],
    
    # Minimum user class to send messages (0=all, 1=reg, 3=op)
    "min_class_to_send": 0,
}

# =============================================================================
# Global State
# =============================================================================

bridge_thread = None
bridge_running = False
bridge_connected = False
message_queue = Queue()  # Thread-safe queue for messages from bridge to hub
last_error = None
retry_count = 0

def name_and_version():
    """Script metadata"""
    return "MatterbridgeConnector", "1.0.0"

# =============================================================================
# Helper Functions
# =============================================================================

def get_hub_name() -> str:
    """Get hub name from config"""
    try:
        return vh.GetConfig("config", "hub_name") or "Verlihub"
    except:
        return "Verlihub"

def get_user_class_name(user_class: int) -> str:
    """Convert user class to readable name"""
    classes = {
        0: "Guest",
        1: "Regular", 
        2: "VIP",
        3: "Operator",
        4: "Cheef",
        5: "Admin",
        10: "Master"
    }
    return classes.get(user_class, f"Class{user_class}")

def format_message_for_bridge(nick: str, message: str) -> str:
    """Format hub message for sending to bridge"""
    try:
        user_class = vh.GetUserClass(nick)
        class_name = get_user_class_name(user_class)
        hub_name = get_hub_name()
        
        return CONFIG["hub_to_bridge_format"].format(
            nick=nick,
            message=message,
            hub=hub_name,
            class_name=class_name,
            user_class=user_class
        )
    except Exception as e:
        print(f"Error formatting message: {e}")
        return f"<{nick}> {message}"

def format_message_for_hub(msg_data: Dict[str, Any]) -> str:
    """Format bridge message for display in hub"""
    try:
        username = msg_data.get("username", "Unknown")
        text = msg_data.get("text", "")
        protocol = msg_data.get("protocol", "bridge")
        channel = msg_data.get("channel", "")
        
        return CONFIG["bridge_to_hub_format"].format(
            username=username,
            text=text,
            protocol=protocol,
            channel=channel
        )
    except Exception as e:
        print(f"Error formatting bridge message: {e}")
        return msg_data.get("text", "")

def send_to_hub(message: str):
    """Send message to hub chat (thread-safe via queue)"""
    # Don't call vh module directly from background thread
    # Instead, queue the message for processing in OnTimer
    try:
        message_queue.put(message, block=False)
    except Exception as e:
        print(f"Error queuing message: {e}")

# =============================================================================
# Matterbridge API Functions
# =============================================================================

def send_to_matterbridge(text: str, username: str = "Verlihub") -> bool:
    """Send message to Matterbridge API"""
    if not REQUESTS_AVAILABLE:
        return False
    
    try:
        url = f"{CONFIG['api_url']}/api/message"
        
        payload = {
            "text": text,
            "username": username,
            "gateway": CONFIG["gateway"]
        }
        
        headers = {"Content-Type": "application/json"}
        if CONFIG["api_token"]:
            headers["Authorization"] = f"Bearer {CONFIG['api_token']}"
        
        response = requests.post(
            url,
            json=payload,
            headers=headers,
            timeout=5
        )
        
        if response.status_code == 200:
            return True
        else:
            print(f"Matterbridge API error: {response.status_code} - {response.text}")
            return False
            
    except Exception as e:
        print(f"Error sending to Matterbridge: {e}")
        return False

def stream_from_matterbridge():
    """Stream messages from Matterbridge API (runs in separate thread)"""
    global bridge_running, bridge_connected, last_error, retry_count
    
    url = f"{CONFIG['api_url']}/api/stream"
    headers = {}
    if CONFIG["api_token"]:
        headers["Authorization"] = f"Bearer {CONFIG['api_token']}"
    
    while bridge_running:
        try:
            print(f"Connecting to Matterbridge stream: {url}")
            
            response = requests.get(
                url,
                headers=headers,
                stream=True,
                timeout=30
            )
            
            if response.status_code != 200:
                raise Exception(f"HTTP {response.status_code}: {response.text}")
            
            bridge_connected = True
            retry_count = 0
            last_error = None
            print("Connected to Matterbridge stream")
            
            # Send connection notification
            send_to_hub("🌉 Bridge connection established")
            
            # Process stream line by line
            for line in response.iter_lines():
                if not bridge_running:
                    break
                
                if line:
                    try:
                        msg_data = json.loads(line.decode('utf-8'))
                        
                        # Filter events
                        event = msg_data.get("event", "")
                        if event == "api_connected":
                            print("API connected event received")
                            continue
                        
                        # Filter out our own messages (avoid loops)
                        account = msg_data.get("account", "")
                        if "api" in account.lower():
                            continue
                        
                        # Filter empty messages
                        text = msg_data.get("text", "").strip()
                        if not text:
                            continue
                        
                        # Format and send to hub
                        formatted = format_message_for_hub(msg_data)
                        send_to_hub(formatted)
                        
                    except json.JSONDecodeError as e:
                        print(f"JSON decode error: {e}")
                    except Exception as e:
                        print(f"Error processing message: {e}")
            
        except requests.exceptions.RequestException as e:
            bridge_connected = False
            last_error = str(e)
            print(f"Connection error: {e}")
            
            # Retry logic
            if bridge_running:
                retry_count += 1
                if retry_count <= CONFIG["max_retries"]:
                    wait_time = min(CONFIG["retry_interval"] * retry_count, 60)
                    print(f"Retrying in {wait_time} seconds (attempt {retry_count}/{CONFIG['max_retries']})...")
                    time.sleep(wait_time)
                else:
                    print("Max retries reached, giving up")
                    bridge_running = False
                    send_to_hub("⚠️ Bridge disconnected (max retries reached)")
                    break
        
        except Exception as e:
            bridge_connected = False
            last_error = str(e)
            print(f"Unexpected error: {e}")
            if bridge_running:
                time.sleep(CONFIG["retry_interval"])
    
    bridge_connected = False
    print("Bridge thread stopped")

def start_bridge() -> bool:
    """Start the bridge connection"""
    global bridge_thread, bridge_running
    
    if not REQUESTS_AVAILABLE:
        return False
    
    if bridge_thread and bridge_thread.is_alive():
        return False  # Already running
    
    bridge_running = True
    bridge_thread = threading.Thread(target=stream_from_matterbridge, daemon=True)
    bridge_thread.start()
    
    return True

def stop_bridge() -> bool:
    """Stop the bridge connection"""
    global bridge_running, bridge_connected
    
    if not bridge_running:
        return False
    
    bridge_running = False
    bridge_connected = False
    
    # Send disconnect notification
    send_to_hub("🌉 Bridge disconnected")
    
    return True

def is_bridge_running() -> bool:
    """Check if bridge is running"""
    return bridge_running

def is_bridge_connected() -> bool:
    """Check if bridge is connected"""
    return bridge_connected

# =============================================================================
# Verlihub Event Hooks
# =============================================================================

def OnTimer(msec=0):
    """Process queued messages from bridge thread (runs in main thread)"""
    # Process all queued messages
    messages_processed = 0
    max_per_cycle = 10  # Limit messages per cycle to avoid blocking
    
    while messages_processed < max_per_cycle:
        try:
            message = message_queue.get_nowait()
            # Safe to call vh module here - we're in the main thread
            bot_nick = CONFIG["bot_nick"]
            vh.SendToAll(f"<{bot_nick}> {message}")
            messages_processed += 1
        except Empty:
            break
        except Exception as e:
            print(f"Error processing queued message: {e}")
    
    return 1

def OnParsedMsgChat(nick, message):
    """Relay hub chat messages to Matterbridge"""
    
    # Check if bridge is running
    if not is_bridge_connected():
        return 1
    
    # Filter ignored nicks
    if nick in CONFIG["ignore_nicks"]:
        return 1
    
    # Check user class permissions
    try:
        user_class = vh.GetUserClass(nick)
        if user_class < CONFIG["min_class_to_send"]:
            return 1
    except:
        pass
    
    # Format and send message
    formatted = format_message_for_bridge(nick, message)
    send_to_matterbridge(formatted, username=nick)
    
    return 1  # Allow message to proceed normally

def OnHubCommand(nick, command, user_class, in_pm, prefix):
    """Handle bridge commands"""
    parts = command.split()
    
    if not parts or parts[0] != "bridge":
        return 1
    
    # Check permissions (operators only)
    if user_class < 3:
        vh.pm("Permission denied. Operators only.", nick)
        return 0
    
    # Helper function to send messages (handles the correct vh.pm/vh.usermc signature)
    def send_message(msg):
        if in_pm:
            vh.pm(msg, nick)  # pm(message, destination_nick, [from_nick], [bot_nick])
        else:
            vh.usermc(msg, nick)  # usermc(message, destination_nick, [bot_nick])
    
    if len(parts) < 2:
        send_message("Usage: !bridge [start|stop|status|config|token|gateway|channel|help]")
        return 0
    
    subcmd = parts[1].lower()
    
    if subcmd == "start":
        if not REQUESTS_AVAILABLE:
            send_message("ERROR: requests library not installed. Run: pip install requests")
            return 0
        
        if is_bridge_running():
            send_message("Bridge is already running")
        else:
            if start_bridge():
                send_message(f"Bridge starting... Connecting to {CONFIG['api_url']}")
                send_message(f"Gateway: {CONFIG['gateway']}, Channel: {CONFIG['channel']}")
            else:
                send_message("ERROR: Failed to start bridge")
    
    elif subcmd == "stop":
        if is_bridge_running():
            stop_bridge()
            send_message("Bridge stopped")
        else:
            send_message("Bridge is not running")
    
    elif subcmd == "status":
        if is_bridge_running():
            status = "CONNECTED" if is_bridge_connected() else "CONNECTING"
            send_message(f"Bridge is {status}")
            send_message(f"API: {CONFIG['api_url']}")
            send_message(f"Gateway: {CONFIG['gateway']}")
            send_message(f"Channel: {CONFIG['channel']}")
            if last_error and not is_bridge_connected():
                send_message(f"Last error: {last_error}")
        else:
            send_message("Bridge is STOPPED")
    
    elif subcmd == "config":
        if len(parts) < 3:
            send_message(f"Current API URL: {CONFIG['api_url']}")
            send_message("Usage: !bridge config <url>")
        else:
            CONFIG["api_url"] = parts[2].rstrip('/')
            send_message(f"API URL set to: {CONFIG['api_url']}")
            if is_bridge_running():
                send_message("Restart bridge for changes to take effect")
    
    elif subcmd == "token":
        if len(parts) < 3:
            has_token = "SET" if CONFIG["api_token"] else "NOT SET"
            send_message(f"API token: {has_token}")
            send_message("Usage: !bridge token <token>")
        else:
            CONFIG["api_token"] = parts[2]
            send_message("API token updated")
            if is_bridge_running():
                send_message("Restart bridge for changes to take effect")
    
    elif subcmd == "gateway":
        if len(parts) < 3:
            send_message(f"Current gateway: {CONFIG['gateway']}")
            send_message("Usage: !bridge gateway <name>")
        else:
            CONFIG["gateway"] = parts[2]
            send_message(f"Gateway set to: {CONFIG['gateway']}")
    
    elif subcmd == "channel":
        if len(parts) < 3:
            send_message(f"Current channel: {CONFIG['channel']}")
            send_message("Usage: !bridge channel <name>")
        else:
            CONFIG["channel"] = parts[2]
            send_message(f"Channel set to: {CONFIG['channel']}")
    
    elif subcmd == "help":
        help_text = """
Matterbridge Bridge Commands:
  !bridge start              - Start bridge connection
  !bridge stop               - Stop bridge connection
  !bridge status             - Check connection status
  !bridge config <url>       - Set Matterbridge API URL
  !bridge token <token>      - Set API authentication token
  !bridge gateway <name>     - Set gateway name
  !bridge channel <name>     - Set channel name
  !bridge help               - Show this help

Current Configuration:
  API URL: {api_url}
  Gateway: {gateway}
  Channel: {channel}
  Bot Nick: {bot_nick}

Requirements:
  - Matterbridge server running with API enabled
  - Python requests library: pip install requests
  
Matterbridge Configuration Example:
  [api.{gateway}]
  BindAddress="127.0.0.1:4242"
  Buffer=1000
  RemoteNickFormat="{{NICK}}"
  # Optional: Token="your_secret_token"
  
  [[gateway]]
  name="{gateway}"
  enable=true
  
  [[gateway.inout]]
  account="api.{gateway}"
  channel="api"
  
  [[gateway.inout]]
  account="mattermost.yourserver"
  channel="{channel}"
""".format(**CONFIG)
        
        for line in help_text.strip().split('\n'):
            send_message(line)
    
    else:
        send_message(f"Unknown subcommand: {subcmd}")
        send_message("Use: !bridge help")
    
    return 0  # Command handled

def UnLoad():
    """Cleanup when script unloads"""
    global bridge_running
    
    if is_bridge_running():
        print("Stopping bridge...")
        stop_bridge()
        bridge_running = False
        # Give it a moment to disconnect gracefully
        time.sleep(0.5)
    
    print("Matterbridge script unloaded")

# =============================================================================
# Initialization
# =============================================================================

if REQUESTS_AVAILABLE:
    print("Matterbridge connector loaded successfully")
    print(f"API: {CONFIG['api_url']}")
    print(f"Gateway: {CONFIG['gateway']}, Channel: {CONFIG['channel']}")
    print("Use !bridge help to see available commands")
else:
    print("Matterbridge connector loaded with LIMITED functionality")
    print("Install dependencies: pip install requests")
