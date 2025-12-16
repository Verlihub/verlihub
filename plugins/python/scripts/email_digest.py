#!/usr/bin/env python3
"""
Verlihub Email Digest Script

Sends periodic email digests containing:
1. Chat logs - Buffered chat messages sent after inactivity period
2. Client statistics - Join/quit events tracked over time period

Features:
- Configurable inactivity timeout for chat digest
- Configurable tracking period for client stats
- Multiple recipient support
- SMTP authentication support
- Thread-safe buffering
- Automatic cleanup on script unload

Thread Safety:
- All vh module calls happen in OnTimer (main thread)
- Thread-safe locks protect shared data structures
- No background threads access vh module

Author: Verlihub Team
License: GPL
"""

import vh
import smtplib
import threading
import time
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from datetime import datetime, timedelta
from collections import defaultdict
from typing import List, Dict, Any

# =============================================================================
# Configuration
# =============================================================================

CONFIG = {
    # Email server settings
    "smtp_server": "smtp.gmail.com",
    "smtp_port": 587,
    "smtp_use_tls": True,
    "smtp_username": "",  # Your email address
    "smtp_password": "",  # Your email password or app password
    "from_address": "",   # Sender address (usually same as smtp_username)
    
    # Recipients
    "chat_recipients": [],  # List of emails for chat digests
    "stats_recipients": [], # List of emails for client stats
    
    # Chat digest settings
    "chat_inactivity_minutes": 15,  # Send digest after this many minutes of inactivity
    "chat_max_messages": 1000,      # Maximum messages to buffer
    
    # Client stats settings
    "stats_interval_minutes": 60,   # Send stats every N minutes
    "stats_include_ips": False,     # Include IP addresses in stats
    "stats_include_geo": True,      # Include country codes in stats
    "stats_include_clients": True,  # Include client software versions
    "stats_include_share": True,    # Include share size information
    "stats_detailed": False,        # Show all historical data vs just latest
    
    # General settings
    "hub_name": "Verlihub Hub",     # Hub name for email subject
    "timezone": "UTC",              # Timezone for timestamps
}

# =============================================================================
# Global State
# =============================================================================

# Chat buffer
chat_buffer = []
chat_buffer_lock = threading.Lock()
last_chat_time = 0

# Client statistics
client_stats = defaultdict(lambda: {
    "joins": 0,
    "quits": 0,
    "ips": set(),
    "countries": set(),
    "clients": set(),  # Client software versions
    "share_sizes": [],  # Track share size at each login
    "last_seen": None,
    "last_ip": "",
    "last_country": "",
    "last_client": "",
    "last_share": 0
})
client_stats_lock = threading.Lock()
stats_start_time = time.time()

# Timers
running = True
last_stats_send = time.time()

# =============================================================================
# Email Functions
# =============================================================================

def send_email(subject: str, body: str, recipients: List[str], html: bool = False) -> bool:
    """Send an email via SMTP"""
    if not recipients:
        print("[Email Digest] No recipients configured")
        return False
    
    if not CONFIG["smtp_username"] or not CONFIG["smtp_password"]:
        print("[Email Digest] SMTP credentials not configured")
        return False
    
    try:
        # Create message
        msg = MIMEMultipart('alternative')
        msg['Subject'] = subject
        msg['From'] = CONFIG["from_address"] or CONFIG["smtp_username"]
        msg['To'] = ", ".join(recipients)
        
        # Attach body
        mime_type = 'html' if html else 'plain'
        msg.attach(MIMEText(body, mime_type))
        
        # Send via SMTP
        server = smtplib.SMTP(CONFIG["smtp_server"], CONFIG["smtp_port"])
        
        if CONFIG["smtp_use_tls"]:
            server.starttls()
        
        if CONFIG["smtp_username"] and CONFIG["smtp_password"]:
            server.login(CONFIG["smtp_username"], CONFIG["smtp_password"])
        
        server.send_message(msg)
        server.quit()
        
        print(f"[Email Digest] Sent email to {len(recipients)} recipient(s)")
        return True
        
    except Exception as e:
        print(f"[Email Digest] Failed to send email: {e}")
        return False

# =============================================================================
# Chat Digest Functions
# =============================================================================

def add_chat_message(nick: str, message: str):
    """Add a chat message to the buffer (thread-safe)"""
    global last_chat_time
    
    with chat_buffer_lock:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        chat_buffer.append({
            "timestamp": timestamp,
            "nick": nick,
            "message": message
        })
        
        # Limit buffer size
        if len(chat_buffer) > CONFIG["chat_max_messages"]:
            chat_buffer.pop(0)
        
        last_chat_time = time.time()

def send_chat_digest():
    """Send buffered chat messages via email"""
    with chat_buffer_lock:
        if not chat_buffer:
            return
        
        # Build email body
        message_count = len(chat_buffer)
        subject = f"[{CONFIG['hub_name']}] Chat Digest - {message_count} messages"
        
        # Plain text version
        lines = [
            f"Chat Digest for {CONFIG['hub_name']}",
            f"Period: {chat_buffer[0]['timestamp']} to {chat_buffer[-1]['timestamp']}",
            f"Messages: {message_count}",
            "",
            "=" * 80,
            ""
        ]
        
        for msg in chat_buffer:
            lines.append(f"[{msg['timestamp']}] <{msg['nick']}> {msg['message']}")
        
        body = "\n".join(lines)
        
        # HTML version
        html_lines = [
            "<html><body>",
            f"<h2>Chat Digest for {CONFIG['hub_name']}</h2>",
            f"<p><strong>Period:</strong> {chat_buffer[0]['timestamp']} to {chat_buffer[-1]['timestamp']}</p>",
            f"<p><strong>Messages:</strong> {message_count}</p>",
            "<hr>",
            "<pre style='font-family: monospace; background: #f5f5f5; padding: 10px;'>"
        ]
        
        for msg in chat_buffer:
            html_lines.append(f"[{msg['timestamp']}] &lt;{msg['nick']}&gt; {msg['message']}")
        
        html_lines.append("</pre></body></html>")
        html_body = "\n".join(html_lines)
        
        # Send email
        if send_email(subject, html_body, CONFIG["chat_recipients"], html=True):
            print(f"[Email Digest] Sent chat digest with {message_count} messages")
            chat_buffer.clear()

def check_chat_inactivity():
    """Check if chat has been inactive and send digest if needed"""
    if not CONFIG["chat_recipients"]:
        return
    
    inactivity_seconds = CONFIG["chat_inactivity_minutes"] * 60
    
    with chat_buffer_lock:
        if not chat_buffer:
            return
        
        # Check if enough time has passed since last message
        if time.time() - last_chat_time >= inactivity_seconds:
            send_chat_digest()

# =============================================================================
# Client Statistics Functions
# =============================================================================

def record_client_join(nick: str, ip: str = ""):
    """Record a client join event (thread-safe)"""
    with client_stats_lock:
        client_stats[nick]["joins"] += 1
        client_stats[nick]["last_seen"] = datetime.now()
        
        # IP address
        if ip:
            if CONFIG["stats_include_ips"]:
                client_stats[nick]["ips"].add(ip)
            client_stats[nick]["last_ip"] = ip
        
        # Country code
        try:
            country = vh.GetUserCC(nick) or ""
            if country and CONFIG["stats_include_geo"]:
                client_stats[nick]["countries"].add(country)
                client_stats[nick]["last_country"] = country
        except:
            pass
        
        # Client version
        try:
            client_version = vh.GetUserVersion(nick) or ""
            if client_version and CONFIG["stats_include_clients"]:
                client_stats[nick]["clients"].add(client_version)
                client_stats[nick]["last_client"] = client_version
        except:
            pass
        
        # Share size
        try:
            share = vh.GetUserShare(nick) or 0
            if CONFIG["stats_include_share"]:
                client_stats[nick]["share_sizes"].append(share)
                client_stats[nick]["last_share"] = share
        except:
            pass

def record_client_quit(nick: str, ip: str = ""):
    """Record a client quit event (thread-safe)"""
    with client_stats_lock:
        client_stats[nick]["quits"] += 1
        # Don't update last_seen on quit - keep login time

def format_share_size(bytes_size: int) -> str:
    """Format share size in human-readable format"""
    if bytes_size == 0:
        return "0 B"
    
    units = ['B', 'KB', 'MB', 'GB', 'TB', 'PB']
    unit_index = 0
    size = float(bytes_size)
    
    while size >= 1024 and unit_index < len(units) - 1:
        size /= 1024
        unit_index += 1
    
    if unit_index == 0:
        return f"{int(size)} {units[unit_index]}"
    else:
        return f"{size:.2f} {units[unit_index]}"

def send_client_stats():
    """Send client statistics via email"""
    global stats_start_time
    
    with client_stats_lock:
        if not client_stats:
            print("[Email Digest] No client activity to report")
            return
        
        # Calculate period
        period_start = datetime.fromtimestamp(stats_start_time)
        period_end = datetime.now()
        period_minutes = (period_end - period_start).total_seconds() / 60
        
        # Build statistics
        total_joins = sum(stats["joins"] for stats in client_stats.values())
        total_quits = sum(stats["quits"] for stats in client_stats.values())
        unique_clients = len(client_stats)
        
        # Calculate totals
        total_share = sum(stats["last_share"] for stats in client_stats.values() if stats["last_share"])
        unique_countries = set()
        unique_client_versions = set()
        for stats in client_stats.values():
            unique_countries.update(stats["countries"])
            unique_client_versions.update(stats["clients"])
        
        subject = f"[{CONFIG['hub_name']}] Client Statistics - {unique_clients} clients"
        
        # Plain text version
        lines = [
            f"Client Statistics for {CONFIG['hub_name']}",
            f"Period: {period_start.strftime('%Y-%m-%d %H:%M:%S')} to {period_end.strftime('%Y-%m-%d %H:%M:%S')}",
            f"Duration: {period_minutes:.1f} minutes",
            "",
            f"Total Joins: {total_joins}",
            f"Total Quits: {total_quits}",
            f"Unique Clients: {unique_clients}",
        ]
        
        if CONFIG["stats_include_geo"] and unique_countries:
            lines.append(f"Countries: {len(unique_countries)} ({', '.join(sorted(unique_countries))})")
        
        if CONFIG["stats_include_clients"] and unique_client_versions:
            lines.append(f"Client Software: {len(unique_client_versions)} different versions")
        
        if CONFIG["stats_include_share"] and total_share > 0:
            lines.append(f"Total Share: {format_share_size(total_share)}")
            avg_share = total_share / unique_clients if unique_clients > 0 else 0
            lines.append(f"Average Share: {format_share_size(int(avg_share))}")
        
        lines.extend([
            "",
            "=" * 80,
            ""
        ])
        
        # Table header
        header_parts = [f"{'Client':<20}", f"{'Joins':<7}", f"{'Quits':<7}"]
        
        if CONFIG["stats_include_geo"]:
            header_parts.append(f"{'Country':<8}")
        
        if CONFIG["stats_include_share"]:
            header_parts.append(f"{'Share':<12}")
        
        if CONFIG["stats_include_clients"]:
            header_parts.append(f"{'Client Version':<30}")
        
        if CONFIG["stats_include_ips"]:
            header_parts.append("IP Address")
        
        header_parts.append(f"{'Last Seen':<20}")
        
        lines.append(" ".join(header_parts))
        lines.append("-" * 120)
        
        # Sort by total activity (joins + quits)
        sorted_clients = sorted(
            client_stats.items(),
            key=lambda x: x[1]["joins"] + x[1]["quits"],
            reverse=True
        )
        
        for nick, stats in sorted_clients:
            last_seen = stats["last_seen"].strftime('%Y-%m-%d %H:%M:%S') if stats["last_seen"] else "N/A"
            
            row_parts = [
                f"{nick:<20}",
                f"{stats['joins']:<7}",
                f"{stats['quits']:<7}"
            ]
            
            if CONFIG["stats_include_geo"]:
                country = stats["last_country"] if CONFIG["stats_detailed"] else (
                    ", ".join(sorted(stats["countries"])) if stats["countries"] else ""
                )
                row_parts.append(f"{(country or 'N/A'):<8}")
            
            if CONFIG["stats_include_share"]:
                share = format_share_size(stats["last_share"])
                row_parts.append(f"{share:<12}")
            
            if CONFIG["stats_include_clients"]:
                client = stats["last_client"] if not CONFIG["stats_detailed"] else (
                    ", ".join(stats["clients"]) if stats["clients"] else ""
                )
                row_parts.append(f"{(client or 'N/A'):<30}")
            
            if CONFIG["stats_include_ips"]:
                ips = stats["last_ip"] if not CONFIG["stats_detailed"] else (
                    ", ".join(sorted(stats["ips"])) if stats["ips"] else ""
                )
                row_parts.append(ips or "N/A")
            
            row_parts.append(f"{last_seen:<20}")
            
            lines.append(" ".join(row_parts))
        
        body = "\n".join(lines)
        
        # HTML version
        html_lines = [
            "<html><body>",
            f"<h2>Client Statistics for {CONFIG['hub_name']}</h2>",
            f"<p><strong>Period:</strong> {period_start.strftime('%Y-%m-%d %H:%M:%S')} to {period_end.strftime('%Y-%m-%d %H:%M:%S')}</p>",
            f"<p><strong>Duration:</strong> {period_minutes:.1f} minutes</p>",
            "<hr>",
            f"<p><strong>Total Joins:</strong> {total_joins}</p>",
            f"<p><strong>Total Quits:</strong> {total_quits}</p>",
            f"<p><strong>Unique Clients:</strong> {unique_clients}</p>",
        ]
        
        if CONFIG["stats_include_geo"] and unique_countries:
            html_lines.append(f"<p><strong>Countries:</strong> {len(unique_countries)} ({', '.join(sorted(unique_countries))})</p>")
        
        if CONFIG["stats_include_clients"] and unique_client_versions:
            html_lines.append(f"<p><strong>Client Software:</strong> {len(unique_client_versions)} different versions</p>")
        
        if CONFIG["stats_include_share"] and total_share > 0:
            html_lines.append(f"<p><strong>Total Share:</strong> {format_share_size(total_share)}</p>")
            avg_share = total_share / unique_clients if unique_clients > 0 else 0
            html_lines.append(f"<p><strong>Average Share:</strong> {format_share_size(int(avg_share))}</p>")
        
        html_lines.extend([
            "<hr>",
            "<table border='1' cellpadding='5' cellspacing='0' style='border-collapse: collapse;'>",
            "<tr style='background: #f0f0f0;'>",
            "<th>Client</th><th>Joins</th><th>Quits</th>"
        ])
        
        if CONFIG["stats_include_geo"]:
            html_lines.append("<th>Country</th>")
        
        if CONFIG["stats_include_share"]:
            html_lines.append("<th>Share</th>")
        
        if CONFIG["stats_include_clients"]:
            html_lines.append("<th>Client Version</th>")
        
        if CONFIG["stats_include_ips"]:
            html_lines.append("<th>IP Address</th>")
        
        html_lines.append("<th>Last Seen</th>")
        html_lines.append("</tr>")
        
        for nick, stats in sorted_clients:
            last_seen = stats["last_seen"].strftime('%Y-%m-%d %H:%M:%S') if stats["last_seen"] else "N/A"
            html_lines.append("<tr>")
            html_lines.append(f"<td>{nick}</td>")
            html_lines.append(f"<td align='right'>{stats['joins']}</td>")
            html_lines.append(f"<td align='right'>{stats['quits']}</td>")
            
            if CONFIG["stats_include_geo"]:
                country = stats["last_country"] if not CONFIG["stats_detailed"] else (
                    ", ".join(sorted(stats["countries"])) if stats["countries"] else ""
                )
                html_lines.append(f"<td>{country or 'N/A'}</td>")
            
            if CONFIG["stats_include_share"]:
                share = format_share_size(stats["last_share"])
                html_lines.append(f"<td align='right'>{share}</td>")
            
            if CONFIG["stats_include_clients"]:
                client = stats["last_client"] if not CONFIG["stats_detailed"] else (
                    "<br>".join(stats["clients"]) if stats["clients"] else ""
                )
                html_lines.append(f"<td style='font-size: 90%;'>{client or 'N/A'}</td>")
            
            if CONFIG["stats_include_ips"]:
                ips = stats["last_ip"] if not CONFIG["stats_detailed"] else (
                    "<br>".join(sorted(stats["ips"])) if stats["ips"] else ""
                )
                html_lines.append(f"<td>{ips or 'N/A'}</td>")
            
            html_lines.append(f"<td>{last_seen}</td>")
            html_lines.append("</tr>")
        
        html_lines.append("</table>")
        html_lines.append("</body></html>")
        html_body = "\n".join(html_lines)
        
        # Send email
        if send_email(subject, html_body, CONFIG["stats_recipients"], html=True):
            print(f"[Email Digest] Sent client stats for {unique_clients} clients")
            client_stats.clear()
            stats_start_time = time.time()

def check_stats_interval():
    """Check if it's time to send client statistics"""
    global last_stats_send
    
    if not CONFIG["stats_recipients"]:
        return
    
    interval_seconds = CONFIG["stats_interval_minutes"] * 60
    
    if time.time() - last_stats_send >= interval_seconds:
        send_client_stats()
        last_stats_send = time.time()

# =============================================================================
# Verlihub Event Hooks
# =============================================================================

def OnParsedMsgChat(nick, message):
    """Capture chat messages for digest"""
    add_chat_message(nick, message)
    return 1  # Allow message

def OnUserLogin(nick):
    """Record user login for statistics"""
    try:
        ip = vh.GetUserIP(nick) or ""
        record_client_join(nick, ip)
    except Exception as e:
        print(f"[Email Digest] Error recording login for {nick}: {e}")
    return 1

def OnUserLogout(nick):
    """Record user logout for statistics"""
    try:
        ip = vh.GetUserIP(nick) or ""
        record_client_quit(nick, ip)
    except Exception as e:
        print(f"[Email Digest] Error recording logout for {nick}: {e}")
    return 1

def OnTimer(msec=0):
    """Check for digest sending conditions (runs in main thread)"""
    if not running:
        return 0
    
    # Check chat inactivity
    check_chat_inactivity()
    
    # Check stats interval
    check_stats_interval()
    
    return 1

def OnHubCommand(nick, command, user_class, in_pm, prefix):
    """Handle admin commands
    
    IMPORTANT: Return value logic (Python -> C++ -> Verlihub core):
    - return 1 → C++ returns true → Command is ALLOWED (passes through)
    - return 0 → C++ returns false → Command is BLOCKED (consumed/handled)
    
    So: return 1 for commands we DON'T handle, return 0 for commands we DO handle
    """
    # Debug output
    print(f"[Email Digest] OnHubCommand called: nick={nick}, command='{command}', user_class={user_class}, prefix='{prefix}'")
    
    args = command.split()
    if not args:
        return 1  # Empty command, allow it through
    
    cmd = args[0].lower()
    
    # The prefix (! + etc) is stripped, so command is just "digest ..."
    if cmd != "digest":
        return 1  # Not our command, allow it through
    
    # Check permissions
    if user_class < 10:  # Only ops and above
        vh.pm(nick, "Permission denied. Operators only.")
        return 0  # Block this command
    
    if len(args) < 2:
            vh.pm(nick, "\n".join([
                "Email Digest Commands:",
                "  !digest status            - Show current status",
                "  !digest chat send         - Send chat digest now",
                "  !digest chat clear        - Clear chat buffer",
                "  !digest stats send        - Send client stats now",
                "  !digest stats clear       - Clear client stats",
                "  !digest config            - Show configuration",
                "  !digest test <email>      - Send test email",
            ]))
            return 0  # Block command, we handled it
        
        subcmd = args[1].lower()
        
        if subcmd == "status":
            with chat_buffer_lock:
                chat_count = len(chat_buffer)
                if chat_buffer:
                    chat_age = time.time() - last_chat_time
                    chat_status = f"{chat_count} messages, {chat_age:.0f}s since last"
                else:
                    chat_status = "Empty"
            
            with client_stats_lock:
                stats_count = len(client_stats)
                if stats_count > 0:
                    total_activity = sum(s["joins"] + s["quits"] for s in client_stats.values())
                    stats_status = f"{stats_count} clients, {total_activity} events"
                else:
                    stats_status = "No activity"
            
            vh.pm(nick, "\n".join([
                "Email Digest Status:",
                f"  Chat Buffer: {chat_status}",
                f"  Client Stats: {stats_status}",
                f"  Chat Recipients: {len(CONFIG['chat_recipients'])}",
                f"  Stats Recipients: {len(CONFIG['stats_recipients'])}",
            ]))
            return 0  # Block command, we handled it
        
        elif subcmd == "chat" and len(args) > 2:
            action = args[2].lower()
            if action == "send":
                send_chat_digest()
                vh.pm(nick, "Chat digest sent")
            elif action == "clear":
                with chat_buffer_lock:
                    count = len(chat_buffer)
                    chat_buffer.clear()
                vh.pm(nick, f"Cleared {count} messages from chat buffer")
            return 0  # Block command, we handled it
        
        elif subcmd == "stats" and len(args) > 2:
            action = args[2].lower()
            if action == "send":
                send_client_stats()
                vh.pm(nick, "Client statistics sent")
            elif action == "clear":
                with client_stats_lock:
                    count = len(client_stats)
                    client_stats.clear()
                vh.pm(nick, f"Cleared statistics for {count} clients")
            return 0  # Block command, we handled it
        
        elif subcmd == "config":
            vh.pm(nick, "\n".join([
                "Email Digest Configuration:",
                f"  SMTP Server: {CONFIG['smtp_server']}:{CONFIG['smtp_port']}",
                f"  From: {CONFIG['from_address'] or CONFIG['smtp_username']}",
                f"  Chat Inactivity: {CONFIG['chat_inactivity_minutes']} minutes",
                f"  Stats Interval: {CONFIG['stats_interval_minutes']} minutes",
                f"  Chat Recipients: {', '.join(CONFIG['chat_recipients']) or 'None'}",
                f"  Stats Recipients: {', '.join(CONFIG['stats_recipients']) or 'None'}",
            ]))
            return 0  # Block command, we handled it
        
        elif subcmd == "test" and len(args) > 2:
            test_email = args[2]
            subject = f"[{CONFIG['hub_name']}] Test Email"
            body = f"This is a test email from the Verlihub Email Digest script.\n\nSent at: {datetime.now()}"
            if send_email(subject, body, [test_email]):
                vh.pm(nick, f"Test email sent to {test_email}")
            else:
                vh.pm(nick, "Failed to send test email - check console for errors")
            return 0  # Block command, we handled it
    
    return 1  # Not our command (or unrecognized subcommand), allow it through

def UnLoad():
    """Cleanup when script unloads"""
    global running
    running = False
    
    # Send any pending digests
    print("[Email Digest] Unloading - sending pending digests...")
    
    with chat_buffer_lock:
        if chat_buffer:
            send_chat_digest()
    
    with client_stats_lock:
        if client_stats:
            send_client_stats()
    
    print("[Email Digest] Script unloaded")

# =============================================================================
# Initialization
# =============================================================================

print("=" * 80)
print("Email Digest Script Loaded")
print("=" * 80)
print("Configuration:")
print(f"  Chat inactivity timeout: {CONFIG['chat_inactivity_minutes']} minutes")
print(f"  Stats interval: {CONFIG['stats_interval_minutes']} minutes")
print(f"  Chat recipients: {len(CONFIG['chat_recipients'])}")
print(f"  Stats recipients: {len(CONFIG['stats_recipients'])}")
print("")
print("IMPORTANT: Edit CONFIG dictionary in script to set SMTP credentials and recipients")
print("Commands: !digest help")
print("=" * 80)
