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
  !api update        - Force cache refresh
  !api help          - Show help

Requirements:
  pip install fastapi uvicorn

Single-Interpreter vs Sub-Interpreter Mode:
--------------------------------------------
SUB-INTERPRETER MODE (default):
  + Each script runs in isolated Python environment
  + Scripts cannot interfere with each other's globals
  + Memory leak in one script doesn't affect others
  
SINGLE-INTERPRETER MODE (required for this script):
  + Full threading and async/await support
  + Better performance (no interpreter switching overhead)
  - All scripts share same Python environment (use unique names!)
  - Global variables are shared between scripts (use proper namespacing)

Thread Safety Note:
-------------------
This script uses threading (uvicorn runs FastAPI in background threads).
The vh module is NOT thread-safe - it can only be called from the main
Verlihub thread. This script uses a cache pattern to work around this:

  1. OnTimer() hook (main thread) updates data cache from vh module
  2. FastAPI endpoints (background threads) read from the cache
  3. Never call vh.* functions from background threads!

See scripts/README.md for detailed thread-safety patterns and best practices.

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

# Try to find and add venv site-packages to path
script_dir = os.path.dirname(os.path.abspath(__file__))

# Look for venv in multiple locations
venv_locations = [
    # Script's own directory
    os.path.join(script_dir, 'venv'),
    # Parent directory (for installed scripts)
    os.path.join(os.path.dirname(script_dir), 'venv'),
    # Build directory (for tests)
    os.path.join(script_dir, '..', '..', 'venv'),
]

# Also check environment variable
if 'VERLIHUB_PYTHON_VENV' in os.environ:
    venv_locations.insert(0, os.environ['VERLIHUB_PYTHON_VENV'])

# Try each location and find site-packages
venv_found = False
for venv_base in venv_locations:
    if not os.path.exists(venv_base):
        continue
    
    # Try different Python version patterns
    lib_dir = os.path.join(venv_base, 'lib')
    if os.path.exists(lib_dir):
        for item in os.listdir(lib_dir):
            if item.startswith('python'):
                site_packages = os.path.join(lib_dir, item, 'site-packages')
                if os.path.exists(site_packages):
                    print(f"[Hub API] Found venv at: {venv_base}")
                    print(f"[Hub API] Adding to path: {site_packages}")
                    sys.path.insert(0, site_packages)
                    venv_found = True
                    break
    if venv_found:
        break

if not venv_found:
    print("[Hub API] No venv found, using system Python packages")

# Debug: Print current sys.path
print(f"[Hub API] Current sys.path: {sys.path[:3]}...")  # First 3 entries

try:
    print("[Hub API] Attempting to import FastAPI...")
    from fastapi import FastAPI, HTTPException
    from fastapi.responses import JSONResponse, RedirectResponse
    from fastapi.middleware.cors import CORSMiddleware
    print("[Hub API] FastAPI imported successfully!")
    print("[Hub API] Attempting to import uvicorn...")
    import uvicorn
    print("[Hub API] uvicorn imported successfully!")
    FASTAPI_AVAILABLE = True
    print("[Hub API] ✓ All dependencies loaded")
except ImportError as e:
    FASTAPI_AVAILABLE = False
    print(f"[Hub API] ✗ ImportError: {e}")
    print(f"[Hub API] sys.path when import failed: {sys.path[:5]}")
    import traceback
    traceback.print_exc()

# Try to import gufo.traceroute
try:
    print("[Hub API] Attempting to import gufo.traceroute...")
    from gufo.traceroute import Traceroute
    print("[Hub API] gufo.traceroute imported successfully!")
    TRACEROUTE_AVAILABLE = True
except ImportError as e:
    TRACEROUTE_AVAILABLE = False
    print(f"[Hub API] ✗ gufo.traceroute not available: {e}")
    print("[Hub API] Install with: pip install gufo-traceroute")

# Try to import python-nmap for OS detection
try:
    print("[Hub API] Attempting to import python-nmap...")
    import nmap
    print("[Hub API] python-nmap imported successfully!")
    NMAP_AVAILABLE = True
except ImportError as e:
    NMAP_AVAILABLE = False
    print(f"[Hub API] ✗ python-nmap not available: {e}")
    print("[Hub API] Install with: pip install python-nmap")
    print("[Hub API] Note: Also requires nmap system package (apt install nmap)")

# Try to import icmplib for network quality monitoring
try:
    print("[Hub API] Attempting to import icmplib...")
    from icmplib import ping, multiping
    print("[Hub API] icmplib imported successfully!")
    ICMPLIB_AVAILABLE = True
except ImportError as e:
    ICMPLIB_AVAILABLE = False
    print(f"[Hub API] ✗ icmplib not available: {e}")
    print("[Hub API] Install with: pip install icmplib")
    print("[Hub API] Note: ICMP ping requires root privileges")

# Global state
api_server = None
api_thread = None
api_port = 8000
server_running = False
cors_origins = []  # Will be populated when server starts
hub_start_time = None  # Track when hub started (or when script loaded)

# Support flags tracking (nick -> list of support flags)
support_flags_cache = {}
support_flags_lock = threading.Lock()

# Traceroute cache (ip -> traceroute result)
traceroute_cache = {}
traceroute_lock = threading.Lock()
traceroute_threads = {}  # ip -> thread object
traceroute_in_progress = set()  # IPs currently being traced
traceroute_priority_queue = {}  # ip -> priority (1=on-demand, 2=periodic, 3=background)
TRACEROUTE_TTL = 3600  # Cache traceroute results for 1 hour
TRACEROUTE_INTERVAL = 300  # Re-trace every 5 minutes if user still online
MAX_CONCURRENT_TRACEROUTES = 5  # Limit concurrent traceroutes

# OS detection cache (ip -> OS detection result)
os_detection_cache = {}
os_detection_lock = threading.Lock()
os_detection_threads = {}  # ip -> thread object
os_detection_in_progress = set()  # IPs currently being scanned
os_detection_priority_queue = {}  # ip -> priority (1=on-demand, 2=periodic, 3=background)
OS_DETECTION_TTL = 7200  # Cache OS detection for 2 hours (longer than traceroute)
OS_DETECTION_INTERVAL = 3600  # Re-scan every 1 hour if user still online
MAX_CONCURRENT_OS_SCANS = 3  # Limit concurrent OS scans (lower than traceroute, more resource intensive)

# ICMP ping cache (ip -> ping quality result)
ping_cache = {}
ping_lock = threading.Lock()
ping_threads = {}  # batch_id -> thread object
ping_in_progress = set()  # IPs currently being pinged
ping_priority_queue = {}  # ip -> priority (1=on-demand, 2=periodic, 3=background)
PING_TTL = 300  # Cache ping results for 5 minutes (network conditions change frequently)
PING_INTERVAL = 60  # Re-ping every 1 minute if user still online
MAX_CONCURRENT_PINGS = 10  # Limit concurrent ping operations
PING_COUNT = 10  # Number of ICMP packets to send per ping
PING_INTERVAL_MS = 0.2  # Interval between packets in seconds

# Thread-safe cache for hub data (updated by OnTimer in main thread)
data_cache = {
    "hub_info": {},
    "users": [],
    "bots": [],
    "geo_stats": {},
    "share_stats": {},
    "hub_encoding": "cp1251",  # Default to CP1251 (common for DC++ hubs)
    "last_update": 0
}
data_cache_lock = threading.Lock()
CACHE_UPDATE_INTERVAL = 5.0  # Update cache every 5 seconds (not every timer tick!)
last_cache_update = 0  # Track when cache was last updated

# Initialize FastAPI app
if FASTAPI_AVAILABLE:
    app = FastAPI(
        title="Verlihub API",
        description="REST API for Verlihub DC++ Hub",
        version="1.0.0"
    )
    
    # CORS middleware will be configured dynamically when server starts
    # (see start_api_server function)

def name_and_version():
    """Script metadata for Verlihub plugin system"""
    return "HubAPI", "1.0.0"

# =============================================================================
# Helper Functions
# =============================================================================

def safe_decode(text: str, encoding: str = "cp1251") -> str:
    """Safely decode text from hub encoding to UTF-8 for JSON/web display
    
    Args:
        text: String in hub encoding (may already be Python str if ASCII-compatible)
        encoding: Hub encoding (cp1251, iso-8859-1, etc.)
    
    Returns:
        UTF-8 string safe for JSON, with problematic chars replaced
    """
    if not text:
        return text
    
    try:
        # If text is already a proper Unicode string (all chars < 128), return as-is
        if all(ord(c) < 128 for c in text):
            return text
        
        # Try to encode back to bytes using latin-1 (which preserves byte values)
        # then decode using the actual hub encoding
        try:
            # Python strings from C++ are decoded as latin-1 by default when they
            # contain bytes > 127. We need to reverse that and use the real encoding.
            byte_data = text.encode('latin-1', errors='replace')
            return byte_data.decode(encoding, errors='replace')
        except (UnicodeDecodeError, UnicodeEncodeError, LookupError):
            # If that fails, just replace problematic characters
            return text.encode('utf-8', errors='replace').decode('utf-8', errors='replace')
    except Exception as e:
        print(f"[Hub API] Encoding error for text '{text[:50]}...': {e}")
        # Last resort: keep what we have
        return text

def update_data_cache():
    """Update cached data from vh module (called from main thread only)"""
    try:
        # Get hub encoding first
        hub_encoding = vh.GetConfig("config", "hub_encoding", "cp1251")
        if not hub_encoding or hub_encoding.lower() == "utf-8":
            hub_encoding = "utf-8"  # No conversion needed
        
        # Gather all data
        hub_info = _get_hub_info_unsafe()
        users = _get_all_users_unsafe(hub_encoding)
        bots = _get_bot_list_unsafe()
        geo_stats = _get_geographic_stats_unsafe()
        share_stats = _get_share_stats_unsafe(users)
        
        # Update cache atomically
        with data_cache_lock:
            data_cache["hub_info"] = hub_info
            data_cache["users"] = users
            data_cache["bots"] = bots
            data_cache["geo_stats"] = geo_stats
            data_cache["share_stats"] = share_stats
            data_cache["hub_encoding"] = hub_encoding
            data_cache["last_update"] = time.time()
    except Exception as e:
        import traceback
        print(f"[Hub API] Error updating data cache: {e}")
        print(f"[Hub API] Traceback:\n{traceback.format_exc()}")

def get_cached_data(key: str) -> Any:
    """Get cached data (thread-safe)"""
    with data_cache_lock:
        return data_cache.get(key)

def get_traceroute_result(ip: str) -> Optional[Dict[str, Any]]:
    """Get cached traceroute result for an IP (thread-safe)
    
    Args:
        ip: IP address
    
    Returns:
        Traceroute result dict or None if not available/expired
    """
    with traceroute_lock:
        if ip in traceroute_cache:
            result = traceroute_cache[ip]
            # Check if result is still fresh
            if time.time() - result.get("timestamp", 0) < TRACEROUTE_TTL:
                return result
            else:
                # Expired, remove it
                del traceroute_cache[ip]
        return None

async def perform_traceroute_async(ip: str):
    """Perform traceroute to an IP address asynchronously
    
    Args:
        ip: IP address to trace
    """
    if not TRACEROUTE_AVAILABLE:
        return
    
    try:
        print(f"[Hub API] Starting traceroute to {ip}")
        
        # Perform the traceroute using async context manager
        hops = []
        hop_count = 0
        
        async with Traceroute() as tr:
            async for hop_info in tr.traceroute(ip, tries=3):
                hop_count += 1
                # HopInfo has ttl and hops (list of Hop objects)
                # Each Hop has addr (IP), rtt (seconds), and asn
                # Pick the first valid hop from the list (or use average RTT)
                valid_hops = [h for h in hop_info.hops if h is not None]
                if valid_hops:
                    # Use the first valid hop's IP and average RTT
                    hop_ip = valid_hops[0].addr
                    avg_rtt = sum(h.rtt for h in valid_hops) / len(valid_hops)
                    
                    # Try reverse DNS lookup for the hop IP
                    hop_hostname = None
                    try:
                        import socket
                        hop_hostname = socket.gethostbyaddr(hop_ip)[0]
                    except (socket.herror, socket.gaierror, OSError):
                        # No reverse DNS or lookup failed
                        pass
                    
                    hop_data = {
                        "hop": hop_count,
                        "ip": hop_ip,
                        "hostname": hop_hostname,
                        "rtt_ms": round(avg_rtt * 1000, 2),  # Convert to ms
                    }
                    hops.append(hop_data)
                    
                    # Stop if we reached the destination
                    if hop_ip == ip:
                        break
                else:
                    # Timeout - no response for this hop
                    hop_data = {
                        "hop": hop_count,
                        "ip": None,
                        "hostname": None,
                        "rtt_ms": None,
                    }
                    hops.append(hop_data)
        
        # Store result in cache
        result = {
            "ip": ip,
            "hops": hops,
            "hop_count": hop_count,
            "timestamp": time.time(),
            "success": len(hops) > 0
        }
        
        with traceroute_lock:
            traceroute_cache[ip] = result
            traceroute_in_progress.discard(ip)
            if ip in traceroute_threads:
                del traceroute_threads[ip]
        
        print(f"[Hub API] Traceroute to {ip} completed: {hop_count} hops")
        
    except Exception as e:
        print(f"[Hub API] Traceroute to {ip} failed: {e}")
        import traceback
        traceback.print_exc()
        
        # Store error result
        with traceroute_lock:
            traceroute_cache[ip] = {
                "ip": ip,
                "error": str(e),
                "timestamp": time.time(),
                "success": False
            }
            traceroute_in_progress.discard(ip)
            if ip in traceroute_threads:
                del traceroute_threads[ip]

def perform_traceroute(ip: str):
    """Wrapper to run async traceroute in a thread
    
    Args:
        ip: IP address to trace
    """
    asyncio.run(perform_traceroute_async(ip))

def schedule_traceroute(ip: str, priority: int = 3) -> bool:
    """Schedule a traceroute for an IP address if not already in progress
    
    Args:
        ip: IP address to trace
        priority: Priority level (1=on-demand/highest, 2=periodic, 3=background/lowest)
    
    Returns:
        True if traceroute was scheduled, False if already in progress or limit reached
    """
    if not TRACEROUTE_AVAILABLE:
        return False
    
    if not ip or ip == "127.0.0.1" or ip.startswith("192.168.") or ip.startswith("10."):
        return False  # Skip localhost and private IPs
    
    with traceroute_lock:
        # Check if already in progress
        if ip in traceroute_in_progress:
            # Update priority if new request has higher priority (lower number)
            if ip in traceroute_priority_queue:
                traceroute_priority_queue[ip] = min(traceroute_priority_queue[ip], priority)
            return False
        
        # Check concurrent limit
        if len(traceroute_in_progress) >= MAX_CONCURRENT_TRACEROUTES:
            # Queue it with priority for later
            if ip not in traceroute_priority_queue:
                traceroute_priority_queue[ip] = priority
            else:
                traceroute_priority_queue[ip] = min(traceroute_priority_queue[ip], priority)
            return False
        
        # Check if we have a recent result
        if ip in traceroute_cache:
            result = traceroute_cache[ip]
            age = time.time() - result.get("timestamp", 0)
            if age < TRACEROUTE_INTERVAL:
                # For on-demand requests (priority 1), ignore cache if it's old
                if priority == 1 and age > 60:  # On-demand requests want fresh data after 1 min
                    pass  # Continue to schedule
                else:
                    return False  # Too recent, don't re-trace yet
        
        # Mark as in progress
        traceroute_in_progress.add(ip)
        # Remove from queue if it was there
        traceroute_priority_queue.pop(ip, None)
    
    # Start traceroute in background thread
    thread = threading.Thread(target=perform_traceroute, args=(ip,), daemon=True)
    thread.start()
    
    with traceroute_lock:
        traceroute_threads[ip] = thread
    
    return True

def update_traceroutes_for_users():
    """Update traceroutes for online users (called periodically)"""
    if not TRACEROUTE_AVAILABLE or not server_running:
        return
    
    users = get_cached_data("users") or []
    
    # Get unique IPs from online users
    user_ips = set()
    for user in users:
        ip = user.get("ip")
        if ip and ip != "127.0.0.1" and not ip.startswith("192.168.") and not ip.startswith("10."):
            user_ips.add(ip)
    
    # Schedule traceroutes for IPs that need updates
    scheduled = 0
    for ip in user_ips:
        if schedule_traceroute(ip):
            scheduled += 1
            # Don't start too many at once
            if scheduled >= 3:
                break
    
    if scheduled > 0:
        print(f"[Hub API] Scheduled {scheduled} traceroutes")

def get_os_detection_result(ip: str) -> Optional[Dict[str, Any]]:
    """Get cached OS detection result for an IP (thread-safe)
    
    Args:
        ip: IP address
    
    Returns:
        OS detection result dict or None if not available/expired
    """
    with os_detection_lock:
        if ip in os_detection_cache:
            result = os_detection_cache[ip]
            # Check if result is still fresh
            if time.time() - result.get("timestamp", 0) < OS_DETECTION_TTL:
                return result
            else:
                # Expired, remove it
                del os_detection_cache[ip]
        return None

def perform_os_detection(ip: str):
    """Perform OS detection scan on an IP address
    
    Args:
        ip: IP address to scan
    """
    if not NMAP_AVAILABLE:
        return
    
    try:
        print(f"[Hub API] Starting OS detection scan for {ip}")
        
        # Create nmap scanner
        nm = nmap.PortScanner()
        
        # Perform OS detection scan
        # -O: Enable OS detection
        # -sV: Service version detection
        # --osscan-limit: Limit OS detection to promising targets
        # -T4: Faster timing template (aggressive)
        # --max-retries 2: Limit retries
        nm.scan(ip, arguments='-O -sV --osscan-limit -T4 --max-retries 2')
        
        os_matches = []
        
        if ip in nm.all_hosts():
            host_data = nm[ip]
            
            # Extract OS match information
            if 'osmatch' in host_data:
                for match in host_data['osmatch']:
                    os_info = {
                        "name": match.get('name', 'Unknown'),
                        "accuracy": int(match.get('accuracy', 0)),
                        "line": match.get('line', 0)
                    }
                    
                    # Include OS class details if available
                    if 'osclass' in match and match['osclass']:
                        os_class = match['osclass'][0] if isinstance(match['osclass'], list) else match['osclass']
                        os_info["os_family"] = os_class.get('osfamily', '')
                        os_info["os_gen"] = os_class.get('osgen', '')
                        os_info["vendor"] = os_class.get('vendor', '')
                        os_info["type"] = os_class.get('type', '')
                    
                    os_matches.append(os_info)
            
            # Sort by accuracy (highest first)
            os_matches.sort(key=lambda x: x.get('accuracy', 0), reverse=True)
        
        # Store result in cache
        result = {
            "ip": ip,
            "os_matches": os_matches,
            "timestamp": time.time(),
            "success": len(os_matches) > 0,
            "best_match": os_matches[0] if os_matches else None,
            "completed": True  # Scan completed (even if no matches found)
        }
        
        with os_detection_lock:
            os_detection_cache[ip] = result
            os_detection_in_progress.discard(ip)
            if ip in os_detection_threads:
                del os_detection_threads[ip]
        
        if os_matches:
            best = os_matches[0]
            print(f"[Hub API] OS detection for {ip} completed: {best['name']} ({best['accuracy']}% accuracy)")
        else:
            print(f"[Hub API] OS detection for {ip} completed: No OS match found")
        
    except Exception as e:
        print(f"[Hub API] OS detection for {ip} failed: {e}")
        import traceback
        traceback.print_exc()
        
        # Store error result with failure flag
        with os_detection_lock:
            os_detection_cache[ip] = {
                "ip": ip,
                "error": str(e),
                "timestamp": time.time(),
                "success": False,
                "failed": True,  # Explicit failure flag for frontend
                "os_matches": [],
                "best_match": None
            }
            os_detection_in_progress.discard(ip)
            if ip in os_detection_threads:
                del os_detection_threads[ip]

def schedule_os_detection(ip: str, priority: int = 3) -> bool:
    """Schedule an OS detection scan for an IP address if not already in progress
    
    Args:
        ip: IP address to scan
        priority: Priority level (1=on-demand/highest, 2=periodic, 3=background/lowest)
    
    Returns:
        True if scan was scheduled, False if already in progress or limit reached
    """
    if not NMAP_AVAILABLE:
        return False
    
    if not ip or ip == "127.0.0.1" or ip.startswith("192.168.") or ip.startswith("10."):
        return False  # Skip localhost and private IPs
    
    with os_detection_lock:
        # Check if already in progress
        if ip in os_detection_in_progress:
            # Update priority if new request has higher priority
            if ip in os_detection_priority_queue:
                os_detection_priority_queue[ip] = min(os_detection_priority_queue[ip], priority)
            return False
        
        # Check concurrent limit
        if len(os_detection_in_progress) >= MAX_CONCURRENT_OS_SCANS:
            # Queue it with priority for later
            if ip not in os_detection_priority_queue:
                os_detection_priority_queue[ip] = priority
            else:
                os_detection_priority_queue[ip] = min(os_detection_priority_queue[ip], priority)
            return False
        
        # Check if we have a recent result
        if ip in os_detection_cache:
            result = os_detection_cache[ip]
            age = time.time() - result.get("timestamp", 0)
            if age < OS_DETECTION_INTERVAL:
                # For on-demand requests, ignore cache if it's old
                if priority == 1 and age > 300:  # On-demand wants fresh data after 5 min
                    pass  # Continue to schedule
                else:
                    return False  # Too recent, don't re-scan yet
        
        # Mark as in progress
        os_detection_in_progress.add(ip)
        # Remove from queue if it was there
        os_detection_priority_queue.pop(ip, None)
    
    # Start OS detection in background thread
    thread = threading.Thread(target=perform_os_detection, args=(ip,), daemon=True)
    thread.start()
    
    with os_detection_lock:
        os_detection_threads[ip] = thread
    
    return True

def update_os_detection_for_users():
    """Update OS detection scans for online users (called periodically)"""
    if not NMAP_AVAILABLE or not server_running:
        return
    
    users = get_cached_data("users") or []
    
    # Get unique IPs from online users
    user_ips = set()
    for user in users:
        ip = user.get("ip")
        if ip and ip != "127.0.0.1":
            user_ips.add(ip)
    
    # Schedule OS detection for IPs that need updates
    scheduled = 0
    for ip in user_ips:
        if schedule_os_detection(ip):
            scheduled += 1
            # Don't start too many at once (OS detection is more resource intensive)
            if scheduled >= 2:
                break
    
    if scheduled > 0:
        print(f"[Hub API] Scheduled {scheduled} OS detection scans")

def get_ping_result(ip: str) -> Optional[Dict[str, Any]]:
    """Get cached ping result for an IP (thread-safe)
    
    Args:
        ip: IP address
    
    Returns:
        Ping result dict or None if not available/expired
    """
    with ping_lock:
        if ip in ping_cache:
            result = ping_cache[ip]
            age = time.time() - result.get("timestamp", 0)
            if age < PING_TTL:
                return result
        return None

def perform_ping_batch(ips: List[str]):
    """Perform ICMP ping on a batch of IP addresses
    
    Args:
        ips: List of IP addresses to ping
    """
    if not ICMPLIB_AVAILABLE:
        print("[Hub API] ICMP ping skipped - icmplib not available")
        return
    
    batch_id = f"batch_{int(time.time())}"
    
    try:
        print(f"[Hub API] Starting ping batch for {len(ips)} IPs: {ips}")
        
        # Use multiping for efficiency (can ping multiple hosts simultaneously)
        # Note: privileged=True requires root/sudo
        hosts = multiping(ips, count=PING_COUNT, interval=PING_INTERVAL_MS, timeout=2, privileged=True)
        
        # Convert to list to avoid consuming iterator multiple times
        hosts_list = list(hosts)
        print(f"[Hub API] Ping batch completed, processing {len(hosts_list)} results")
        
        # Store results in cache
        with ping_lock:
            for host in hosts_list:
                result = {
                    "ip": host.address,
                    "min_rtt": round(host.min_rtt, 3) if host.min_rtt < float('inf') else None,
                    "avg_rtt": round(host.avg_rtt, 3) if host.avg_rtt < float('inf') else None,
                    "max_rtt": round(host.max_rtt, 3) if host.max_rtt < float('inf') else None,
                    "packets_sent": host.packets_sent,
                    "packets_received": host.packets_received,
                    "packet_loss": round(host.packet_loss, 3),
                    "jitter": round(host.jitter, 3) if host.jitter < float('inf') else None,
                    "is_alive": host.is_alive,
                    "timestamp": time.time(),
                    "success": host.is_alive,
                    "completed": True
                }
                
                ping_cache[host.address] = result
                ping_in_progress.discard(host.address)
                
                print(f"[Hub API] Cached ping result for {host.address}: alive={host.is_alive}, avg_rtt={result['avg_rtt']}ms")
            
            # Remove batch thread
            if batch_id in ping_threads:
                del ping_threads[batch_id]
        
        alive_count = sum(1 for h in hosts_list if h.is_alive)
        print(f"[Hub API] Ping batch stored in cache: {len(hosts_list)} hosts, {alive_count} alive")
        
    except Exception as e:
        print(f"[Hub API] Ping batch failed: {e}")
        import traceback
        traceback.print_exc()
        
        # Store error results for all IPs in batch
        with ping_lock:
            for ip in ips:
                ping_cache[ip] = {
                    "ip": ip,
                    "error": str(e),
                    "timestamp": time.time(),
                    "success": False,
                    "failed": True,
                    "completed": True
                }
                ping_in_progress.discard(ip)
            
            if batch_id in ping_threads:
                del ping_threads[batch_id]

def schedule_ping(ip: str, priority: int = 3) -> bool:
    """Schedule a ping for an IP address if not already in progress
    
    Args:
        ip: IP address to ping
        priority: Priority level (1=on-demand/highest, 2=periodic, 3=background/lowest)
    
    Returns:
        True if ping was scheduled, False if already in progress or limit reached
    """
    if not ICMPLIB_AVAILABLE:
        return False
    
    if not ip or ip == "127.0.0.1":
        return False  # Skip localhost
    
    with ping_lock:
        # Check if already in progress
        if ip in ping_in_progress:
            # Update priority if new request has higher priority
            if ip in ping_priority_queue:
                ping_priority_queue[ip] = min(ping_priority_queue[ip], priority)
            return False
        
        # Check concurrent limit
        if len(ping_in_progress) >= MAX_CONCURRENT_PINGS:
            # Queue it with priority for later
            if ip not in ping_priority_queue:
                ping_priority_queue[ip] = priority
            else:
                ping_priority_queue[ip] = min(ping_priority_queue[ip], priority)
            return False
        
        # Check if we have a recent result
        if ip in ping_cache:
            result = ping_cache[ip]
            age = time.time() - result.get("timestamp", 0)
            if age < PING_INTERVAL:
                # For on-demand requests, accept slightly older cache
                if priority == 1 and age > 30:  # On-demand wants fresh data after 30 sec
                    pass  # Continue to schedule
                else:
                    return False  # Still fresh
        
        # Mark as in progress
        ping_in_progress.add(ip)
        # Remove from queue if it was there
        ping_priority_queue.pop(ip, None)
    
    return True

def update_pings_for_users():
    """Update ping results for online users (called periodically)"""
    if not ICMPLIB_AVAILABLE or not server_running:
        return
    
    users = get_cached_data("users") or []
    
    # Get unique IPs from online users
    user_ips = set()
    for user in users:
        ip = user.get("ip")
        if ip and ip != "127.0.0.1" and not ip.startswith("192.168.") and not ip.startswith("10."):
            user_ips.add(ip)
    
    print(f"[Hub API] update_pings_for_users: Found {len(user_ips)} unique user IPs")
    
    # Filter IPs that need updates
    ips_to_ping = []
    for ip in user_ips:
        if schedule_ping(ip):
            ips_to_ping.append(ip)
    
    print(f"[Hub API] update_pings_for_users: {len(ips_to_ping)} IPs need pinging")
    
    if ips_to_ping:
        # Ping in batch for efficiency (limit batch size)
        batch_size = min(len(ips_to_ping), MAX_CONCURRENT_PINGS)
        batch = ips_to_ping[:batch_size]
        
        batch_id = f"batch_{int(time.time())}"
        thread = threading.Thread(target=perform_ping_batch, args=(batch,), daemon=True)
        thread.start()
        
        with ping_lock:
            ping_threads[batch_id] = thread
        
        print(f"[Hub API] Scheduled ping batch: {len(batch)} IPs, thread started")

def format_uptime(seconds: float) -> str:
    """Format uptime in human-readable format"""
    days = int(seconds // 86400)
    hours = int((seconds % 86400) // 3600)
    minutes = int((seconds % 3600) // 60)
    secs = int(seconds % 60)
    
    parts = []
    if days > 0:
        parts.append(f"{days}d")
    if hours > 0 or days > 0:
        parts.append(f"{hours}h")
    if minutes > 0 or hours > 0 or days > 0:
        parts.append(f"{minutes}m")
    parts.append(f"{secs}s")
    
    return " ".join(parts)

def _get_support_flags(nick: str) -> List[str]:
    """Get support flags for a user (thread-safe)
    
    Args:
        nick: User nickname
    
    Returns:
        List of support flag strings
    """
    with support_flags_lock:
        return support_flags_cache.get(nick, [])

def _get_hub_info_unsafe() -> Dict[str, Any]:
    """Get basic hub information (UNSAFE - call only from main thread)"""
    try:
        # GetConfig returns None if config value is not set, use third param for default
        hub_name = vh.GetConfig("config", "hub_name", "Verlihub")
        hub_desc = vh.GetConfig("config", "hub_desc", "DC++ Hub")
        hub_host = vh.GetConfig("config", "hub_host", "")
        topic = vh.Topic()
        max_users_str = vh.GetConfig("config", "max_users", "0")
        hub_icon_url = vh.GetConfig("config", "hub_icon_url", "")
        hub_logo_url = vh.GetConfig("config", "hub_logo_url", "")
        
        # Get Verlihub version from config
        version_info = vh.GetConfig("config", "hub_version", "Verlihub")
        if not version_info:
            version_info = "Verlihub"
        
        # Calculate uptime
        uptime_seconds = 0
        uptime_formatted = "Unknown"
        if hub_start_time:
            uptime_seconds = time.time() - hub_start_time
            uptime_formatted = format_uptime(uptime_seconds)
        
        # Ensure we have valid strings (GetConfig might return None)
        if hub_name is None:
            hub_name = "Verlihub"
        if hub_desc is None:
            hub_desc = "DC++ Hub"
        if hub_host is None or hub_host == "":
            # Use API server address as fallback if hub_host not configured
            if server_running and api_port:
                hub_host = f"http://localhost:{api_port}/"
            else:
                hub_host = ""
        if topic is None:
            topic = ""
        if max_users_str is None:
            max_users_str = "0"
        if version_info is None:
            version_info = "Unknown"
        if hub_icon_url is None:
            hub_icon_url = ""
        if hub_logo_url is None:
            hub_logo_url = ""
        
        # Read MOTD file
        motd = ""
        try:
            # Get config directory from vh module (it's available as vh.config_name attribute)
            config_dir = getattr(vh, 'basedir', '/etc/verlihub')
            motd_file = os.path.join(config_dir, 'motd')
            if os.path.exists(motd_file):
                with open(motd_file, 'r', encoding='utf-8', errors='replace') as f:
                    motd = f.read().strip()
        except Exception as e:
            print(f"[Hub API] Could not read MOTD file: {e}")
        
        result = {
            "name": hub_name,
            "description": hub_desc,
            "host": hub_host,
            "topic": topic,
            "motd": motd,
            "max_users": int(max_users_str) if max_users_str.isdigit() else 0,
            "version": version_info,
            "icon_url": hub_icon_url,
            "logo_url": hub_logo_url,
            "uptime_seconds": uptime_seconds,
            "uptime": uptime_formatted
        }
        
        return result
    except Exception as e:
        import traceback
        print(f"[Hub API] Error in _get_hub_info_unsafe: {e}")
        print(f"[Hub API] Traceback:\n{traceback.format_exc()}")
        return {
            "name": "Verlihub",
            "description": "DC++ Hub",
            "host": "",
            "topic": "",
            "motd": "",
            "max_users": 0,
            "version": "Unknown",
            "icon_url": "",
            "logo_url": "",
            "uptime_seconds": 0,
            "uptime": "Unknown",
            "error": str(e)
        }

def _get_support_flags(nick: str) -> List[str]:
    """Get support flags for a user (thread-safe)
    
    Args:
        nick: User nickname
    
    Returns:
        List of support flag strings
    """
    with support_flags_lock:
        return support_flags_cache.get(nick, [])

def _get_user_info_unsafe(nick: str) -> Optional[Dict[str, Any]]:
    """Get detailed information about a user (UNSAFE - call only from main thread)
    
    Args:
        nick: User nickname in hub encoding (exactly as returned by GetNickList)
    """
    try:
        # Get hub encoding for proper display conversion
        hub_encoding = data_cache.get("hub_encoding", "cp1251")
        
        user_class = vh.GetUserClass(nick)
        if user_class < 0:  # User not found
            return None
        
        ip = vh.GetUserIP(nick)
        host = vh.GetUserHost(nick)
        cc = vh.GetUserCC(nick)
        hub_url = vh.GetUserHubURL(nick) or ""
        ext_json = vh.GetUserExtJSON(nick) or ""
        
        # Get comprehensive geographic info
        country = ""
        city = ""
        region = ""
        region_code = ""
        timezone = ""
        continent = ""
        continent_code = ""
        postal_code = ""
        asn = ""
        
        if ip:
            try:
                # Try GetGeoIP first (returns dict with all geographic details)
                geo_data = vh.GetGeoIP(ip, "")
                
                if geo_data and isinstance(geo_data, dict):
                    country = geo_data.get("country", "") or ""
                    city = geo_data.get("city", "") or ""
                    region = geo_data.get("region", "") or ""
                    region_code = geo_data.get("region_code", "") or ""
                    timezone = geo_data.get("time_zone", "") or ""
                    continent = geo_data.get("continent", "") or ""
                    continent_code = geo_data.get("continent_code", "") or ""
                    postal_code = geo_data.get("postal_code", "") or ""
                else:
                    # Fallback to individual geo functions if GetGeoIP fails
                    try:
                        country = vh.GetIPCN(ip) or ""
                    except Exception:
                        country = ""
                    
                    try:
                        city = vh.GetIPCity(ip, "") or ""
                        # Filter out placeholder values
                        if city == "--":
                            city = ""
                    except Exception:
                        city = ""
                
                # Get ASN separately
                asn_raw = vh.GetIPASN(ip, "")
                asn = asn_raw if (asn_raw and asn_raw not in ("--", "")) else ""
            except Exception as e:
                print(f"[Hub API] Error getting geo info for {ip}: {e}")
                traceback.print_exc()
        
        # GetMyINFO returns tuple: (nick, desc, tag, speed, email, sharesize)
        myinfo = vh.GetMyINFO(nick)
        share = 0
        desc = ""
        tag = ""
        email = ""
        speed = ""
        
        if myinfo and isinstance(myinfo, tuple) and len(myinfo) >= 6:
            # Tuple format: (nick, desc, tag, speed, email, sharesize)
            _, desc, tag, speed, email, size_str = myinfo[:6]
            
            # Convert desc, tag, email from hub encoding to UTF-8 for display
            desc = safe_decode(desc, hub_encoding)
            tag = safe_decode(tag, hub_encoding)
            email = safe_decode(email, hub_encoding)
            
            try:
                # Share size is in bytes as a string
                share = int(size_str) if size_str else 0
            except (ValueError, TypeError):
                share = 0
        
        # Convert nick for display (but keep original for lookups)
        nick_display = safe_decode(nick, hub_encoding)
        
        # Get support flags
        support_flags = _get_support_flags(nick)
        
        return {
            "nick": nick_display,  # Converted for display
            "class": user_class,
            "class_name": get_class_name(user_class),
            "ip": ip,
            "host": host,
            "country_code": cc,
            "country": country,
            "city": city,
            "region": region,
            "region_code": region_code,
            "timezone": timezone,
            "continent": continent,
            "continent_code": continent_code,
            "postal_code": postal_code,
            "asn": asn,
            "hub_url": hub_url,
            "ext_json": ext_json,
            "description": desc,
            "tag": tag,
            "email": email,
            "share": share,
            "share_formatted": format_bytes(share),
            "support_flags": support_flags
        }
    except Exception as e:
        print(f"[Hub API] Error getting user info for {nick}: {e}")
        import traceback
        traceback.print_exc()
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

def _get_all_users_unsafe(hub_encoding: str = "cp1251") -> List[Dict[str, Any]]:
    """Get list of all users with their information (UNSAFE - call only from main thread)
    
    Args:
        hub_encoding: The hub's character encoding for proper display conversion
    """
    users = []
    nick_list = vh.GetNickList()
    
    if not isinstance(nick_list, list):
        return users
    
    for nick in nick_list:
        # nick is in hub encoding - use it as-is for lookups
        user_info = _get_user_info_unsafe(nick)
        if user_info:
            users.append(user_info)
    
    # Add clone detection information
    _add_clone_detection(users)
    
    return users

def _add_clone_detection(users: List[Dict[str, Any]]):
    """Add clone detection fields to user list (modifies in place)
    
    Adds fields:
    - cloned: boolean indicating if user is part of a clone group (same IP + share)
    - clone_group: list of nicks with same IP and share size
    - same_ip_users: list of nicks with same IP (NAT group)
    - same_asn_users: list of nicks with same ASN (network group)
    """
    # Build lookup tables
    ip_to_users = {}  # ip -> list of user dicts
    asn_to_users = {}  # asn -> list of user dicts
    ip_share_to_users = {}  # (ip, share) -> list of user dicts
    
    for user in users:
        ip = user.get("ip", "")
        asn = user.get("asn", "")
        share = user.get("share", 0)
        nick = user.get("nick", "")
        
        # Group by IP
        if ip:
            if ip not in ip_to_users:
                ip_to_users[ip] = []
            ip_to_users[ip].append(user)
        
        # Group by ASN
        if asn:
            if asn not in asn_to_users:
                asn_to_users[asn] = []
            asn_to_users[asn].append(user)
        
        # Group by IP + share (clones)
        if ip:
            key = (ip, share)
            if key not in ip_share_to_users:
                ip_share_to_users[key] = []
            ip_share_to_users[key].append(user)
    
    # Add clone detection fields to each user
    for user in users:
        ip = user.get("ip", "")
        asn = user.get("asn", "")
        share = user.get("share", 0)
        nick = user.get("nick", "")
        
        # Clone group (same IP + share)
        clone_key = (ip, share)
        clone_group = ip_share_to_users.get(clone_key, [])
        clone_nicks = [u.get("nick") for u in clone_group if u.get("nick") != nick]
        user["cloned"] = len(clone_group) > 1
        user["clone_group"] = clone_nicks
        
        # Same IP users (NAT group)
        same_ip = ip_to_users.get(ip, [])
        same_ip_nicks = [u.get("nick") for u in same_ip if u.get("nick") != nick]
        user["same_ip_users"] = same_ip_nicks
        
        # Same ASN users (network group)
        same_asn = asn_to_users.get(asn, []) if asn else []
        same_asn_nicks = [u.get("nick") for u in same_asn if u.get("nick") != nick]
        user["same_asn_users"] = same_asn_nicks

def _get_bot_list_unsafe() -> List[str]:
    """Get list of bot nicknames (UNSAFE - call only from main thread)"""
    try:
        bot_list = vh.GetBotList()
        if isinstance(bot_list, list):
            return bot_list
        return []
    except Exception as e:
        print(f"[Hub API] Error getting bot list: {e}")
        return []

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

def _get_share_stats_unsafe(users: List[Dict[str, Any]] = None) -> Dict[str, Any]:
    """Get share size statistics (UNSAFE - call only from main thread)"""
    total_share = 0
    if users is None:
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
                "hub_info": "/hub",
                "statistics": "/stats",
                "users": "/users",
                "operators": "/ops",
                "bots": "/bots",
                "user_detail": "/user/{nick}",
                "geography": "/geo",
                "share": "/share"
            }
        }

    @app.get("/hub")
    async def hub_info():
        """Get hub information"""
        return get_cached_data("hub_info") or {}

    @app.get("/stats")
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

    @app.get("/users")
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
            import traceback
            traceback.print_exc()
            raise HTTPException(status_code=500, detail=str(e))

    @app.get("/ops")
    async def operators(limit: Optional[int] = None, offset: int = 0):
        """Get list of online operators"""
        try:
            all_users = get_cached_data("users") or []
            
            # Filter for operators (class >= 3)
            op_users = [u for u in all_users if u.get("class", -1) >= 3]
            
            # Apply pagination
            total = len(op_users)
            if limit is not None:
                op_users = op_users[offset:offset + limit]
            
            return {
                "operators": op_users,
                "total": total,
                "limit": limit,
                "offset": offset
            }
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))
    
    @app.get("/bots")
    async def bots(limit: Optional[int] = None, offset: int = 0):
        """Get list of bots"""
        try:
            # Get bot list from vh module (cached)
            bot_list = get_cached_data("bots") or []
            
            # Get detailed info for each bot from users cache
            all_users = get_cached_data("users") or []
            bot_users = [u for u in all_users if u.get("nick") in bot_list]
            
            # Apply pagination
            total = len(bot_users)
            if limit is not None:
                bot_users = bot_users[offset:offset + limit]
            
            return {
                "bots": bot_users,
                "total": total,
                "limit": limit,
                "offset": offset
            }
        except Exception as e:
            raise HTTPException(status_code=500, detail=str(e))

    @app.get("/user/{nick}")
    async def user_detail(nick: str):
        """Get detailed information about a specific user"""
        all_users = get_cached_data("users") or []
        
        # Find user in cache
        user_info = next((u for u in all_users if u.get("nick") == nick), None)
        
        if not user_info:
            raise HTTPException(status_code=404, detail=f"User '{nick}' not found")
        
        return user_info

    @app.get("/geo")
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

    @app.get("/share")
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
    
    @app.get("/traceroute/{ip}")
    async def traceroute_result(ip: str):
        """Get traceroute result for an IP address
        
        This endpoint returns cached traceroute results. Traceroutes are performed
        automatically for online users in the background.
        """
        if not TRACEROUTE_AVAILABLE:
            raise HTTPException(
                status_code=503, 
                detail="Traceroute functionality not available. Install gufo-traceroute package."
            )
        
        result = get_traceroute_result(ip)
        
        if not result:
            # Try to schedule a traceroute if not already cached (priority 1 = on-demand)
            scheduled = schedule_traceroute(ip, priority=1)
            
            raise HTTPException(
                status_code=404,
                detail=f"No traceroute data available for {ip}" +
                       (" - high-priority traceroute scheduled, try again in a few seconds" if scheduled else "")
            )
        
        return result
    
    @app.get("/traceroutes")
    async def all_traceroutes():
        """Get all cached traceroute results"""
        if not TRACEROUTE_AVAILABLE:
            raise HTTPException(
                status_code=503,
                detail="Traceroute functionality not available. Install gufo-traceroute package."
            )
        
        with traceroute_lock:
            # Filter out expired results
            current_time = time.time()
            active_results = {
                ip: result 
                for ip, result in traceroute_cache.items()
                if current_time - result.get("timestamp", 0) < TRACEROUTE_TTL
            }
            
            return {
                "count": len(active_results),
                "traceroutes": list(active_results.values()),
                "in_progress": list(traceroute_in_progress)
            }
    
    @app.get("/os/{ip}")
    async def os_detection_result(ip: str):
        """Get OS detection result for an IP address
        
        This endpoint returns cached OS detection results. OS scans are performed
        automatically for online users in the background using nmap.
        """
        if not NMAP_AVAILABLE:
            raise HTTPException(
                status_code=503,
                detail="OS detection functionality not available. Install python-nmap package and nmap system utility."
            )
        
        result = get_os_detection_result(ip)
        
        if not result:
            # Try to schedule an OS scan if not already cached
            scheduled = schedule_os_detection(ip)
            
            raise HTTPException(
                status_code=404,
                detail=f"No OS detection data available for {ip}" +
                       (" - OS scan scheduled, try again in 10-30 seconds" if scheduled else "")
            )
        
        return result
    
    @app.get("/os-detections")
    async def all_os_detections():
        """Get all cached OS detection results"""
        if not NMAP_AVAILABLE:
            raise HTTPException(
                status_code=503,
                detail="OS detection functionality not available. Install python-nmap package and nmap system utility."
            )
        
        with os_detection_lock:
            # Filter out expired results
            current_time = time.time()
            active_results = {
                ip: result
                for ip, result in os_detection_cache.items()
                if current_time - result.get("timestamp", 0) < OS_DETECTION_TTL
            }
            
            return {
                "count": len(active_results),
                "os_detections": list(active_results.values()),
                "in_progress": list(os_detection_in_progress)
            }
    
    @app.get("/ping/{ip}")
    async def ping_result(ip: str):
        """Get ping quality result for an IP address
        
        This endpoint returns cached ICMP ping results. Pings are performed
        automatically for online users in the background.
        """
        if not ICMPLIB_AVAILABLE:
            raise HTTPException(
                status_code=503,
                detail="Ping functionality not available. Install icmplib package. Note: ICMP ping requires root privileges."
            )
        
        result = get_ping_result(ip)
        
        if not result:
            # Try to schedule a ping if not already cached (priority 1 = on-demand)
            scheduled = schedule_ping(ip, priority=1)
            
            if scheduled:
                # Actually launch the ping in a background thread
                thread = threading.Thread(target=perform_ping_batch, args=([ip],), daemon=True)
                thread.start()
                
                batch_id = f"single_{ip}_{int(time.time())}"
                with ping_lock:
                    ping_threads[batch_id] = thread
            
            raise HTTPException(
                status_code=404,
                detail=f"No ping data available for {ip}" +
                       (" - high-priority ping scheduled, try again in a few seconds" if scheduled else "")
            )
        
        return result
    
    @app.get("/pings")
    async def all_pings():
        """Get all cached ping results"""
        if not ICMPLIB_AVAILABLE:
            raise HTTPException(
                status_code=503,
                detail="Ping functionality not available. Install icmplib package."
            )
        
        with ping_lock:
            # Filter out expired results
            current_time = time.time()
            active_results = {
                ip: result
                for ip, result in ping_cache.items()
                if current_time - result.get("timestamp", 0) < PING_TTL
            }
            
            return {
                "count": len(active_results),
                "pings": list(active_results.values()),
                "in_progress": list(ping_in_progress)
            }

    @app.get("/favicon.ico")
    async def favicon():
        """Favicon endpoint - redirects to hub icon URL if available"""
        hub_info = get_cached_data("hub_info") or {}
        icon_url = hub_info.get("icon_url", "")
        
        if icon_url:
            return RedirectResponse(url=icon_url)
        else:
            raise HTTPException(status_code=404, detail="No hub icon configured")

# =============================================================================
# Server Management
# =============================================================================

def run_server(port: int):
    """Run the FastAPI server in a separate thread"""
    global server_running
    
    try:
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
        print(f"[Hub API] API server error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        server_running = False

def start_api_server(port: int = 8000, additional_origins: List[str] = None) -> bool:
    """Start the API server with dynamic CORS configuration
    
    Args:
        port: Port to run the server on
        additional_origins: Additional CORS origins to allow (beyond defaults)
    """
    global api_thread, api_port, server_running, cors_origins
    
    if not FASTAPI_AVAILABLE:
        return False
    
    if api_thread and api_thread.is_alive():
        return False  # Already running
    
    # Build CORS origins list
    cors_origins = [
        f"http://localhost:{port}",
        f"http://127.0.0.1:{port}",
        f"http://0.0.0.0:{port}",
    ]
    
    # Add additional origins if provided
    if additional_origins:
        cors_origins.extend(additional_origins)
    
    # Configure CORS middleware dynamically
    app.add_middleware(
        CORSMiddleware,
        allow_origins=cors_origins,
        allow_credentials=True,
        allow_methods=["GET", "POST", "PUT", "DELETE", "OPTIONS"],
        allow_headers=["*"],
        expose_headers=["*"],
        max_age=3600,
    )
    
    # Set server_running BEFORE starting thread so OnTimer will update cache
    server_running = True
    
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

def OnParsedMsgSupports(ip, msg, back):
    """Called when user sends $Supports message
    
    Args:
        ip: User IP address
        msg: The full $Supports message string
        back: Response string (unused)
    
    Returns:
        1 to allow the message to be processed normally
    """
    try:
        # Extract nick from connection by IP (use GetNickList to find user)
        nick_list = vh.GetNickList()
        user_nick = None
        
        # Find the user with this IP
        for nick in nick_list:
            if vh.GetUserIP(nick) == ip:
                user_nick = nick
                break
        
        if user_nick:
            # Parse support flags from message
            # $Supports format: "$Supports FLAG1 FLAG2 FLAG3 ..."
            if msg.startswith("$Supports "):
                flags_str = msg[10:]  # Skip "$Supports "
                flags = [f.strip() for f in flags_str.split() if f.strip()]
                
                # Store in cache
                with support_flags_lock:
                    support_flags_cache[user_nick] = flags
                
                print(f"[Hub API] Captured {len(flags)} support flags for {user_nick}: {flags}")
    except Exception as e:
        print(f"[Hub API] Error in OnParsedMsgSupports: {e}")
        import traceback
        traceback.print_exc()
    
    return 1  # Allow message to be processed

def OnTimer(msec):
    """Update data cache periodically (runs in main thread)"""
    global last_cache_update
    
    # Only update if API server is running
    if not server_running:
        return 1
    
    # Throttle updates - only update every CACHE_UPDATE_INTERVAL seconds
    current_time = time.time()
    if current_time - last_cache_update < CACHE_UPDATE_INTERVAL:
        return 1
    
    last_cache_update = current_time
    update_data_cache()
    
    # Periodically update pings for online users
    # Do this frequently (every 15 seconds) as network conditions change quickly
    if int(current_time) % 15 == 0:
        try:
            update_pings_for_users()
        except Exception as e:
            print(f"[Hub API] Error updating pings: {e}")
    
    # Periodically update traceroutes for online users
    # Do this less frequently (every 30 seconds)
    if int(current_time) % 30 == 0:
        try:
            update_traceroutes_for_users()
        except Exception as e:
            print(f"[Hub API] Error updating traceroutes: {e}")
    
    # Periodically update OS detection for online users
    # Do this even less frequently (every 60 seconds) as it's more resource intensive
    if int(current_time) % 60 == 0:
        try:
            update_os_detection_for_users()
        except Exception as e:
            print(f"[Hub API] Error updating OS detections: {e}")
    
    return 1

def OnUserLogin(nick):
    """Update cache when user logs in (runs in main thread)
    
    Also proactively schedules network diagnostics for the user's IP
    """
    if server_running:
        update_data_cache()
        
        # Proactively gather network diagnostics for new user (background priority)
        try:
            ip = vh.GetUserIP(nick)
            if ip and ip != "127.0.0.1" and not ip.startswith("192.168.") and not ip.startswith("10."):
                # Schedule all diagnostics with priority 3 (background)
                # This ensures data is ready when/if user clicks on it
                scheduled_ping = schedule_ping(ip, priority=3)
                scheduled_trace = schedule_traceroute(ip, priority=3)
                scheduled_os = schedule_os_detection(ip, priority=3)
                
                # Actually launch ping if scheduled (pings are quick)
                if scheduled_ping:
                    thread = threading.Thread(target=perform_ping_batch, args=([ip],), daemon=True)
                    thread.start()
                    batch_id = f"login_{ip}_{int(time.time())}"
                    with ping_lock:
                        ping_threads[batch_id] = thread
                
                if any([scheduled_ping, scheduled_trace, scheduled_os]):
                    print(f"[Hub API] Proactive diagnostics scheduled for new user {nick} ({ip}): ping={scheduled_ping}, trace={scheduled_trace}, os={scheduled_os}")
        except Exception as e:
            print(f"[Hub API] Error scheduling proactive diagnostics for {nick}: {e}")
    
    return 1

def OnUserLogout(nick):
    """Update cache when user logs out (runs in main thread)"""
    # Clean up support flags cache
    with support_flags_lock:
        if nick in support_flags_cache:
            del support_flags_cache[nick]
    
    if server_running:
        update_data_cache()
    return 1

def OnHubCommand(nick, command, user_class, in_pm, prefix):
    """Handle hub commands
    
    IMPORTANT: Return value logic (Python -> C++ -> Verlihub core):
    - return 1 -> C++ returns true -> Command is ALLOWED (passes through)
    - return 0 -> C++ returns false -> Command is BLOCKED (consumed/handled)
    
    So: return 1 for commands we DO NOT handle, return 0 for commands we DO handle
    """
    parts = command.split()
    
    # The prefix (! + etc) is stripped, so command is just "api ..."
    if not parts or parts[0] != "api":
        return 1  # Not our command, allow it (true in C++)
    
    # Check permissions (operators only)
    if user_class < 3:
        return 0  # Block this command (false in C++)
    
    # Helper function to send messages (handles the correct vh.pm/vh.usermc signature)
    def send_message(msg):
        if in_pm:
            vh.pm(msg, nick)  # pm(message, destination_nick, [from_nick], [bot_nick])
        else:
            vh.usermc(msg, nick)  # usermc(message, destination_nick, [bot_nick])
    
    if len(parts) < 2:
        print(f"[Hub API] No subcommand, showing usage")
        send_message("Usage: !api [start|stop|status|update|help] [port]")
        return 0  # Block command, we handled it
    
    subcmd = parts[1].lower()
    print(f"[Hub API] Processing subcommand: {subcmd}")
    
    if subcmd == "start":
        if not FASTAPI_AVAILABLE:
            send_message("ERROR: FastAPI not installed. Run: pip install fastapi uvicorn")
            return 0  # Block command, we handled it
        
        port = 8000
        additional_origins = []
        
        # Parse port (parts[2]) and optional CORS origins (parts[3:])
        if len(parts) > 2:
            try:
                port = int(parts[2])
                if port < 1024 or port > 65535:
                    send_message("ERROR: Port must be between 1024 and 65535")
                    return 0  # Block command, we handled it
            except ValueError:
                send_message("ERROR: Invalid port number")
                return 0  # Block command, we handled it
        
        # Parse additional CORS origins
        if len(parts) > 3:
            additional_origins = parts[3:]
            send_message(f"Adding CORS origins: {', '.join(additional_origins)}")
        
        if is_api_running():
            send_message(f"API server already running on port {api_port}")
        else:
            if start_api_server(port, additional_origins):
                send_message(f"API server starting on http://0.0.0.0:{port}")
                send_message(f"Documentation: http://localhost:{port}/docs")
                if cors_origins:
                    send_message(f"CORS origins: {', '.join(cors_origins)}")
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
    
    elif subcmd == "update":
        # Force immediate cache refresh (bypasses throttle)
        if server_running:
            global last_cache_update
            last_cache_update = 0  # Reset throttle timer
            update_data_cache()
            send_message("Cache updated successfully")
        else:
            send_message("API server is not running")
    
    elif subcmd == "help":
        help_text = """
Hub API Commands:
  !api start [port] [origins...]  - Start API server (default port: 8000)
                                     Optional: Add CORS origins (space-separated URLs)
                                     Example: !api start 30000 https://example.com https://other.com
  !api stop                        - Stop API server
  !api status                      - Check server status
  !api update                      - Force cache refresh
  !api help                        - Show this help

API Endpoints:
  GET /              - API overview
  GET /hub           - Hub information
  GET /stats         - Hub statistics
  GET /users         - List all users
  GET /user/{nick}   - User details
  GET /geo           - Geographic distribution
  GET /share         - Share statistics
  GET /health        - Health check
  GET /docs          - Interactive API docs

Requirements:
  pip install fastapi uvicorn
"""
        lines = help_text.strip().split('\n')
        for line in lines:
            send_message(line)
    
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
    
    # Wait for traceroute threads to finish (with timeout)
    if TRACEROUTE_AVAILABLE:
        print("Waiting for traceroute threads to finish...")
        with traceroute_lock:
            threads_to_wait = list(traceroute_threads.values())
        
        for thread in threads_to_wait:
            thread.join(timeout=2.0)  # Wait max 2 seconds per thread
        
        print(f"Cleaned up {len(threads_to_wait)} traceroute threads")
    
    # Wait for OS detection threads to finish (with timeout)
    if NMAP_AVAILABLE:
        print("Waiting for OS detection threads to finish...")
        with os_detection_lock:
            threads_to_wait = list(os_detection_threads.values())
        
        for thread in threads_to_wait:
            thread.join(timeout=3.0)  # Wait max 3 seconds per thread (OS scans may take longer)
        
        print(f"Cleaned up {len(threads_to_wait)} OS detection threads")
    
    # Wait for ping threads to finish (with timeout)
    if ICMPLIB_AVAILABLE:
        print("Waiting for ping threads to finish...")
        with ping_lock:
            threads_to_wait = list(ping_threads.values())
        
        for thread in threads_to_wait:
            thread.join(timeout=1.0)  # Wait max 1 second per thread (pings are fast)
        
        print(f"Cleaned up {len(threads_to_wait)} ping threads")
    
    print("Hub API script unloaded")

# =============================================================================
# Initialization
# =============================================================================

# Track hub start time
hub_start_time = time.time()

if FASTAPI_AVAILABLE:
    print("Hub API script loaded successfully")
    print("Use !api help to see available commands")
    if TRACEROUTE_AVAILABLE:
        print("Traceroute functionality enabled")
        print(f"  - Max concurrent traceroutes: {MAX_CONCURRENT_TRACEROUTES}")
        print(f"  - Cache TTL: {TRACEROUTE_TTL} seconds")
        print(f"  - Re-trace interval: {TRACEROUTE_INTERVAL} seconds")
    else:
        print("Traceroute functionality disabled (install with: pip install gufo-traceroute)")
    
    if NMAP_AVAILABLE:
        print("OS detection functionality enabled")
        print(f"  - Max concurrent scans: {MAX_CONCURRENT_OS_SCANS}")
        print(f"  - Cache TTL: {OS_DETECTION_TTL} seconds")
    else:
        print("OS detection functionality disabled (install with: pip install python-nmap)")
    
    if ICMPLIB_AVAILABLE:
        print("ICMP ping functionality enabled")
        print(f"  - Max concurrent pings: {MAX_CONCURRENT_PINGS}")
        print(f"  - Cache TTL: {PING_TTL} seconds")
        print(f"  - Ping count: {PING_COUNT} packets")
    else:
        print("ICMP ping functionality disabled (install with: pip install icmplib)")
        print("Note: ICMP ping requires root privileges")
else:
    print("Hub API script loaded with LIMITED functionality")
    print("Install dependencies: pip install fastapi uvicorn")
