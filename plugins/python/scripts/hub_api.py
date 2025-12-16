#!/usr/bin/env python3
"""
Verlihub FastAPI HTTP REST API Script

Provides HTTP endpoints for querying hub information:
- Hub statistics (name, users online, total share)
- User list with details
- User information by nickname
- Geographic distribution
- Share statistics

Admin commands:
  !api start [port]  - Start the API server (default port: 8000)
  !api stop          - Stop the API server
  !api status        - Check API server status
  !api help          - Show help

Requirements:
  pip install fastapi uvicorn

Author: Verlihub Team
Version: 1.0.0
"""

import vh
import sys
import os
import asyncio
import threading
import time
from typing import Optional, List, Dict, Any
from datetime import datetime

# Add venv to path if it exists
script_dir = os.path.dirname(os.path.abspath(__file__))
venv_path = os.path.join(script_dir, 'venv', 'lib', 'python3.12', 'site-packages')
if os.path.exists(venv_path):
    sys.path.insert(0, venv_path)

try:
    from fastapi import FastAPI, HTTPException
    from fastapi.responses import JSONResponse
    import uvicorn
    FASTAPI_AVAILABLE = True
except ImportError:
    FASTAPI_AVAILABLE = False
    print("WARNING: FastAPI not installed. Run: pip install fastapi uvicorn")

# Global state
api_server = None
api_thread = None
api_port = 8000
server_running = False

# Thread-safe cache for hub data (updated by OnTimer in main thread)
data_cache = {
    "hub_info": {},
    "users": [],
    "geo_stats": {},
    "share_stats": {},
    "last_update": 0
}
data_cache_lock = threading.Lock()
CACHE_TTL = 1.0  # Cache time-to-live in seconds

# Initialize FastAPI app
if FASTAPI_AVAILABLE:
    app = FastAPI(
        title="Verlihub API",
        description="REST API for Verlihub DC++ Hub",
        version="1.0.0"
    )

def name_and_version():
    """Script metadata"""
    return "HubAPI", "1.0.0"

# =============================================================================
# Helper Functions
# =============================================================================

def update_data_cache():
    """Update cached data from vh module (called from main thread only)"""
    try:
        # Gather all data
        hub_info = _get_hub_info_unsafe()
        users = _get_all_users_unsafe()
        geo_stats = _get_geographic_stats_unsafe()
        share_stats = _get_share_stats_unsafe()
        
        # Update cache atomically
        with data_cache_lock:
            data_cache["hub_info"] = hub_info
            data_cache["users"] = users
            data_cache["geo_stats"] = geo_stats
            data_cache["share_stats"] = share_stats
            data_cache["last_update"] = time.time()
    except Exception as e:
        print(f"Error updating data cache: {e}")

def get_cached_data(key: str) -> Any:
    """Get cached data (thread-safe)"""
    with data_cache_lock:
        return data_cache.get(key)

def _get_hub_info_unsafe() -> Dict[str, Any]:
    """Get basic hub information (UNSAFE - call only from main thread)"""
    try:
        hub_name = vh.GetConfig("config", "hub_name") or "Verlihub"
        hub_desc = vh.GetConfig("config", "hub_desc") or "DC++ Hub"
        topic = vh.Topic() or ""
        max_users = vh.GetConfig("config", "max_users") or "0"
        
        return {
            "name": hub_name,
            "description": hub_desc,
            "topic": topic,
            "max_users": int(max_users) if max_users.isdigit() else 0,
            "version": vh.name_and_version()
        }
    except Exception as e:
        return {
            "name": "Verlihub",
            "description": "DC++ Hub",
            "topic": "",
            "max_users": 0,
            "version": "Unknown",
            "error": str(e)
        }

def _get_user_info_unsafe(nick: str) -> Optional[Dict[str, Any]]:
    """Get detailed information about a user (UNSAFE - call only from main thread)"""
    try:
        user_class = vh.GetUserClass(nick)
        if user_class < 0:  # User not found
            return None
        
        ip = vh.GetUserIP(nick)
        host = vh.GetUserHost(nick)
        cc = vh.GetUserCC(nick)
        country = vh.GetIPCN(ip) if ip else ""
        
        # Parse MyINFO for additional details
        myinfo = vh.GetMyINFO(nick)
        share = 0
        desc = ""
        tag = ""
        email = ""
        
        if myinfo:
            # $MyINFO format: $ALL nick desc<tag>$ $conn$email$sharesize$
            parts = myinfo.split('$')
            if len(parts) >= 4:
                # Extract description and tag
                info_part = parts[2] if len(parts) > 2 else ""
                if '<' in info_part and '>' in info_part:
                    desc = info_part[:info_part.index('<')]
                    tag = info_part[info_part.index('<')+1:info_part.index('>')]
                else:
                    desc = info_part
                
                # Extract email and share
                if len(parts) >= 5:
                    email = parts[3]
                if len(parts) >= 6:
                    try:
                        share = int(parts[4])
                    except ValueError:
                        pass
        
        return {
            "nick": nick,
            "class": user_class,
            "class_name": get_class_name(user_class),
            "ip": ip,
            "host": host,
            "country_code": cc,
            "country": country,
            "description": desc,
            "tag": tag,
            "email": email,
            "share": share,
            "share_formatted": format_bytes(share)
        }
    except Exception as e:
        print(f"Error getting user info for {nick}: {e}")
        return None

def get_class_name(user_class: int) -> str:
    """Convert user class number to name"""
    classes = {
        -1: "Disconnected",
        0: "Guest",
        1: "Regular",
        2: "VIP", 
        3: "Operator",
        4: "Cheef",
        5: "Admin",
        10: "Master"
    }
    return classes.get(user_class, f"Class{user_class}")

def format_bytes(size: int) -> str:
    """Format bytes into human-readable string"""
    for unit in ['B', 'KB', 'MB', 'GB', 'TB', 'PB']:
        if size < 1024.0:
            return f"{size:.2f} {unit}"
        size /= 1024.0
    return f"{size:.2f} EB"

def _get_all_users_unsafe() -> List[Dict[str, Any]]:
    """Get list of all users with their information (UNSAFE - call only from main thread)"""
    users = []
    nick_list = vh.GetNickList()
    
    for nick in nick_list:
        user_info = _get_user_info_unsafe(nick)
        if user_info:
            users.append(user_info)
    
    return users

def _get_geographic_stats_unsafe() -> Dict[str, int]:
    """Get user distribution by country (UNSAFE - call only from main thread)"""
    stats = {}
    nick_list = vh.GetNickList()
    
    for nick in nick_list:
        try:
            cc = vh.GetUserCC(nick)
            if cc and cc != "--":
                stats[cc] = stats.get(cc, 0) + 1
        except:
            pass
    
    return stats

def _get_share_stats_unsafe() -> Dict[str, Any]:
    """Get share size statistics (UNSAFE - call only from main thread)"""
    total_share = 0
    users = _get_all_users_unsafe()
    
    for user in users:
        total_share += user.get("share", 0)
    
    return {
        "total": total_share,
        "total_formatted": format_bytes(total_share),
        "average": total_share // len(users) if users else 0,
        "average_formatted": format_bytes(total_share // len(users)) if users else "0 B"
    }

# =============================================================================
# FastAPI Routes
# =============================================================================

if FASTAPI_AVAILABLE:
    @app.get("/")
    async def root():
        """API root endpoint"""
        return {
            "service": "Verlihub REST API",
            "version": "1.0.0",
            "endpoints": {
                "hub_info": "/api/hub",
                "statistics": "/api/stats",
                "users": "/api/users",
                "user_detail": "/api/user/{nick}",
                "geography": "/api/geo",
                "share": "/api/share"
            }
        }

    @app.get("/api/hub")
    async def hub_info():
        """Get hub information"""
        return get_cached_data("hub_info") or {}

    @app.get("/api/stats")
    async def statistics():
        """Get hub statistics"""
        try:
            hub_info = get_cached_data("hub_info") or {}
            users = get_cached_data("users") or []
            share_stats = get_cached_data("share_stats") or {}
            last_update = get_cached_data("last_update") or 0
            
            return {
                "timestamp": datetime.utcnow().isoformat(),
                "users_online": len(users),
                "max_users": hub_info.get("max_users", 0),
                "total_share": share_stats.get("total_formatted", "0 B"),
                "hub_name": hub_info.get("name", "Unknown"),
                "cache_age": time.time() - last_update
            }
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

    @app.get("/api/users")
    async def users(limit: Optional[int] = None, offset: int = 0):
        """Get list of online users"""
        try:
            all_users = get_cached_data("users") or []
            
            # Apply pagination
            if limit:
                paginated = all_users[offset:offset + limit]
            elif offset:
                paginated = all_users[offset:]
            else:
                paginated = all_users
            
            return {
                "count": len(paginated),
                "total": len(all_users),
                "users": paginated
            }
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

    @app.get("/api/user/{nick}")
    async def user_detail(nick: str):
        """Get detailed information about a specific user"""
        all_users = get_cached_data("users") or []
        
        # Find user in cache
        user_info = next((u for u in all_users if u.get("nick") == nick), None)
        
        if not user_info:
            raise HTTPException(status_code=404, detail=f"User '{nick}' not found")
        
        return user_info

    @app.get("/api/geo")
    async def geography():
        """Get geographic distribution of users"""
        try:
            stats = get_cached_data("geo_stats") or {}
            
            # Sort by count descending
            sorted_stats = sorted(stats.items(), key=lambda x: x[1], reverse=True)
            
            return {
                "total_countries": len(stats),
                "distribution": [
                    {"country_code": cc, "users": count}
                    for cc, count in sorted_stats
                ]
            }
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

    @app.get("/api/share")
    async def share_statistics():
        """Get share size statistics"""
        try:
            return get_cached_data("share_stats") or {}
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

    @app.get("/health")
    async def health():
        """Health check endpoint"""
        return {
            "status": "healthy",
            "timestamp": datetime.utcnow().isoformat()
        }

# =============================================================================
# Server Management
# =============================================================================

def run_server(port: int):
    """Run the FastAPI server in a separate thread"""
    global server_running
    
    try:
        server_running = True
        config = uvicorn.Config(
            app,
            host="0.0.0.0",
            port=port,
            log_level="info",
            access_log=True
        )
        server = uvicorn.Server(config)
        
        # Run in the current thread (which is already a separate thread)
        asyncio.run(server.serve())
    except Exception as e:
        print(f"API server error: {e}")
    finally:
        server_running = False

def start_api_server(port: int = 8000) -> bool:
    """Start the API server"""
    global api_thread, api_port, server_running
    
    if not FASTAPI_AVAILABLE:
        return False
    
    if api_thread and api_thread.is_alive():
        return False  # Already running
    
    # Initialize cache before starting server
    update_data_cache()
    
    api_port = port
    api_thread = threading.Thread(target=run_server, args=(port,), daemon=True)
    api_thread.start()
    
    return True

def stop_api_server() -> bool:
    """Stop the API server"""
    global server_running
    
    if not server_running:
        return False
    
    # Signal to stop (uvicorn doesn't have easy way to stop from another thread)
    # The daemon thread will exit when script unloads
    server_running = False
    return True

def is_api_running() -> bool:
    """Check if API server is running"""
    return server_running

# =============================================================================
# Verlihub Event Hooks
# =============================================================================

def OnTimer(msec=0):
    """Update data cache periodically (runs in main thread)"""
    # Only update if API server is running
    if server_running:
        update_data_cache()
    return 1

def OnHubCommand(nick, command, user_class, in_pm, prefix):
    """Handle hub commands
    
    IMPORTANT: Return value logic (Python -> C++ -> Verlihub core):
    - return 1 → C++ returns true → Command is ALLOWED (passes through)
    - return 0 → C++ returns false → Command is BLOCKED (consumed/handled)
    
    So: return 1 for commands we DON'T handle, return 0 for commands we DO handle
    """
    # Debug output
    print(f"[Hub API] OnHubCommand called: nick={nick}, command='{command}', user_class={user_class}, prefix='{prefix}'")
    
    parts = command.split()
    print(f"[Hub API] parts={parts}")
    
    # The prefix (! + etc) is stripped, so command is just "api ..."
    if not parts or parts[0] != "api":
        print(f"[Hub API] Not our command, returning 1 to allow through")
        return 1  # Not our command, allow it (true in C++)
    
    print(f"[Hub API] This is our command! Checking permissions...")
    
    # Check permissions (operators only)
    if user_class < 3:
        print(f"[Hub API] Permission denied for user_class={user_class}")
        vh.pm("Permission denied. Operators only.", nick)
        return 0  # Block this command (false in C++)
    
    print(f"[Hub API] Permission OK, processing command...")
    
    # Helper function to send messages (handles the correct vh.pm/vh.usermc signature)
    def send_message(msg):
        if in_pm:
            vh.pm(msg, nick)  # pm(message, destination_nick, [from_nick], [bot_nick])
        else:
            vh.usermc(msg, nick)  # usermc(message, destination_nick, [bot_nick])
    
    if len(parts) < 2:
        print(f"[Hub API] No subcommand, showing usage")
        send_message("Usage: !api [start|stop|status|help] [port]")
        return 0  # Block command, we handled it
    
    subcmd = parts[1].lower()
    print(f"[Hub API] Processing subcommand: {subcmd}")
    
    if subcmd == "start":
        if not FASTAPI_AVAILABLE:
            send_message("ERROR: FastAPI not installed. Run: pip install fastapi uvicorn")
            return 0  # Block command, we handled it
        
        port = 8000
        if len(parts) > 2:
            try:
                port = int(parts[2])
                if port < 1024 or port > 65535:
                    send_message("ERROR: Port must be between 1024 and 65535")
                    return 0  # Block command, we handled it
            except ValueError:
                send_message("ERROR: Invalid port number")
                return 0  # Block command, we handled it
        
        if is_api_running():
            send_message(f"API server already running on port {api_port}")
        else:
            if start_api_server(port):
                send_message(f"API server starting on http://0.0.0.0:{port}")
                send_message(f"Documentation: http://localhost:{port}/docs")
            else:
                send_message("ERROR: Failed to start API server")
    
    elif subcmd == "stop":
        if is_api_running():
            stop_api_server()
            send_message("API server stopping...")
        else:
            send_message("API server is not running")
    
    elif subcmd == "status":
        if is_api_running():
            send_message(f"API server is RUNNING on port {api_port}")
            send_message(f"Endpoints: http://localhost:{api_port}/")
            send_message(f"Docs: http://localhost:{api_port}/docs")
        else:
            send_message("API server is STOPPED")
    
    elif subcmd == "help":
        print(f"[Hub API] Showing help, in_pm={in_pm}, write function={'vh.pm' if in_pm else 'vh.usermc'}")
        help_text = """
Hub API Commands:
  !api start [port]  - Start API server (default: 8000)
  !api stop          - Stop API server
  !api status        - Check server status
  !api help          - Show this help

API Endpoints:
  GET /              - API overview
  GET /api/hub       - Hub information
  GET /api/stats     - Hub statistics
  GET /api/users     - List all users
  GET /api/user/{nick} - User details
  GET /api/geo       - Geographic distribution
  GET /api/share     - Share statistics
  GET /health        - Health check
  GET /docs          - Interactive API docs

Requirements:
  pip install fastapi uvicorn
"""
        lines = help_text.strip().split('\n')
        print(f"[Hub API] Sending {len(lines)} help lines")
        for i, line in enumerate(lines):
            print(f"[Hub API] Sending line {i}: write({repr(nick)}, {repr(line)})")
            try:
                result = send_message(line)
                print(f"[Hub API] Line {i} result: {result}")
            except Exception as e:
                print(f"[Hub API] ERROR on line {i}: {e}")
                import traceback
                traceback.print_exc()
    
    else:
        send_message(f"Unknown subcommand: {subcmd}")
        send_message("Use: !api help")
    
    return 0  # Block command, we handled it

def UnLoad():
    """Cleanup when script unloads"""
    global server_running
    
    if is_api_running():
        print("Stopping API server...")
        stop_api_server()
        server_running = False
    
    print("Hub API script unloaded")

# =============================================================================
# Initialization
# =============================================================================

if FASTAPI_AVAILABLE:
    print("Hub API script loaded successfully")
    print("Use !api help to see available commands")
else:
    print("Hub API script loaded with LIMITED functionality")
    print("Install dependencies: pip install fastapi uvicorn")
