# Verlihub Python Script Examples

This directory contains example Python scripts demonstrating the capabilities of the Verlihub Python plugin.

## Table of Contents

- [Important: Thread Safety](#important-thread-safety)
- [Scripts](#scripts)
  - [hub_api.py - REST API Server](#hub_apipy---rest-api-server)
  - [matterbridge.py - Chat Bridge Connector](#matterbridgepy---chat-bridge-connector)
- [Installation](#installation)
  - [Using Virtual Environment (Recommended)](#using-virtual-environment-recommended)
  - [System-wide Installation](#system-wide-installation)
- [Development](#development)
  - [Creating New Scripts](#creating-new-scripts)
  - [Best Practices](#best-practices)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Important: Thread Safety

**⚠️ CRITICAL: The `vh` module is NOT thread-safe and must only be called from the main Verlihub thread.**

### The Problem

The Verlihub C++ API was not designed with thread-safety in mind. When Python scripts create background threads (for HTTP servers, network I/O, etc.), calling `vh` module functions from these threads can cause:

- **Race conditions** with the main hub event loop
- **Memory corruption** from concurrent access to C++ objects
- **Crashes** or undefined behavior
- **Data inconsistencies** in hub state

### Which Threads Are Safe?

| Thread Type | Safe for `vh` calls? | Examples |
|-------------|---------------------|----------|
| **Main Verlihub thread** | ✅ **YES** | Event hooks (`OnParsedMsgChat`, `OnUserLogin`, etc.), `OnTimer` |
| **Background Python threads** | ❌ **NO** | FastAPI/uvicorn workers, HTTP streaming threads, asyncio tasks |

### Safe Patterns

Both example scripts demonstrate thread-safe patterns to work around this limitation:

#### Pattern 1: Queue-Based Message Passing (matterbridge.py)

```python
from queue import Queue

# Global queue for cross-thread communication
message_queue = Queue()

def background_thread():
    """Worker thread doing network I/O"""
    while running:
        data = fetch_from_network()
        # DON'T call vh.SendToAll() here - wrong thread!
        message_queue.put(data)  # Queue it instead

def OnTimer(msec=0):
    """Verlihub hook - runs in MAIN thread (SAFE)"""
    max_per_cycle = 10
    processed = 0
    
    while processed < max_per_cycle:
        try:
            message = message_queue.get_nowait()
            vh.SendToAll(message)  # ✅ Safe - main thread
            processed += 1
        except Empty:
            break
    
    return 1  # Continue timer
```

**Use when:** You need to send data from background threads to the hub.

#### Pattern 2: Cache-Based Data Access (hub_api.py)

```python
import threading

# Global cache updated from main thread
data_cache = {
    "hub_info": {},
    "users": [],
    "last_update": 0
}
data_cache_lock = threading.Lock()

def OnTimer(msec=0):
    """Update cache from MAIN thread (SAFE)"""
    # All vh module calls happen here
    with data_cache_lock:
        data_cache["hub_info"] = {
            "name": vh.GetConfig("config", "hub_name"),  # ✅ Safe
            "users": vh.GetUsersCount(),  # ✅ Safe
        }
        data_cache["last_update"] = time.time()
    return 1

async def api_endpoint():
    """FastAPI handler - runs in BACKGROUND thread"""
    # DON'T call vh.GetConfig() here - wrong thread!
    with data_cache_lock:
        return data_cache["hub_info"]  # ✅ Safe - read from cache
```

**Use when:** Background threads need to read hub data (queries, API responses).

### Implementation Guidelines

When creating scripts with background threads:

1. **Never call `vh.*` from background threads** - This includes:
   - `vh.SendToAll()`, `vh.pm()`, `vh.usermc()`
   - `vh.GetConfig()`, `vh.GetNickList()`, `vh.GetUserClass()`
   - `vh.Topic()`, `vh.GetUsersCount()`, etc.
   - Any function from the `vh` module

2. **Use OnTimer for main thread processing:**
   ```python
   def OnTimer(msec=0):
       # Process queued work
       # Update caches
       # Send messages
       return 1  # Must return 1 to continue
   ```

3. **Choose the right pattern:**
   - **Queue pattern**: For sending data TO the hub (messages, commands)
   - **Cache pattern**: For reading data FROM the hub (stats, user info)
   - **Hybrid**: Combine both for bidirectional communication

4. **Protect shared data with locks:**
   ```python
   import threading
   cache_lock = threading.Lock()
   
   with cache_lock:
       # Access shared data
   ```

5. **Handle graceful shutdown:**
   ```python
   def UnLoad():
       global running
       running = False  # Signal threads to exit
       if worker_thread:
           worker_thread.join(timeout=2)  # Wait for cleanup
   ```

### Future Improvements

This limitation exists because the Verlihub C++ core was designed for single-threaded operation. Potential future solutions:

- **Command queue in C++**: Queue operations and execute them in the main event loop
- **Thread-local contexts**: Per-thread hub connection instances
- **Mutex protection**: Add fine-grained locking to C++ API
- **Async C++ API**: Redesign for concurrent access

Until then, use the queue/cache patterns demonstrated in these example scripts.

---

## Scripts

### hub_api.py - REST API Server

A FastAPI-based HTTP REST API server that exposes Verlihub hub information through HTTP endpoints.

**Features:**
- Real-time hub statistics (users online, total share)
- User information queries (by nickname or list all)
- Geographic distribution of users by country
- Share size statistics with human-readable formatting
- Interactive API documentation (Swagger/OpenAPI)
- Background server operation (non-blocking)
- Admin commands to start/stop the API

**API Endpoints:**
- `GET /` - API overview with endpoint listing
- `GET /api/hub` - Hub information (name, description, topic, max users, version)
- `GET /api/stats` - Real-time statistics (users online, total share)
- `GET /api/users` - List all online users (supports pagination with `?limit=N&offset=N`)
- `GET /api/user/{nick}` - Detailed information for specific user
- `GET /api/geo` - Geographic distribution by country code
- `GET /api/share` - Share statistics (total, average, formatted)
- `GET /health` - Health check endpoint
- `GET /docs` - Interactive Swagger/OpenAPI documentation

**Admin Commands:**
```
!api start [port]  - Start API server (default port: 8000)
!api stop          - Stop API server
!api status        - Check server status
!api help          - Show help
```

**Requirements:**
```bash
pip install fastapi uvicorn
```

**Usage:**
```bash
# Load the script
!pyload /path/to/scripts/hub_api.py

# Start the API server
!api start 8000

# Access the API
# Browse to: http://localhost:8000/
# Documentation: http://localhost:8000/docs
# Example query: curl http://localhost:8000/api/stats
```

**Example Response:**
```json
{
  "timestamp": "2025-12-15T12:00:00",
  "users_online": 42,
  "max_users": 100,
  "total_share": "15.3 TB",
  "hub_name": "My Hub"
}
```

---

### matterbridge.py - Chat Bridge Connector

Bidirectional chat bridge connecting Verlihub with Matterbridge, enabling integration with Mattermost, Slack, IRC, Discord, Telegram, Matrix, and many other platforms.

**Features:**
- Bidirectional message relay (Hub ↔ Matterbridge)
- Real-time message streaming via HTTP API
- Automatic reconnection with exponential backoff
- Message filtering and formatting
- Support for multiple protocols simultaneously
- Thread-safe background operation
- Graceful error handling and recovery
- Configurable message formats

**Supported Platforms (via Matterbridge):**
- Mattermost
- Slack
- Discord
- IRC
- Telegram
- Matrix
- Microsoft Teams
- Rocket.Chat
- Gitter
- XMPP
- Zulip
- And many more...

**Admin Commands:**
```
!bridge start              - Start Matterbridge connection
!bridge stop               - Stop Matterbridge connection
!bridge status             - Check connection status
!bridge config <url>       - Set Matterbridge API URL
!bridge token <token>      - Set API authentication token
!bridge gateway <name>     - Set gateway name
!bridge channel <name>     - Set channel name
!bridge help               - Show help with config examples
```

**Requirements:**
```bash
pip install requests
```

**Matterbridge Configuration:**

Add to your `matterbridge.toml`:

```toml
[api.verlihub]
BindAddress="127.0.0.1:4242"
Buffer=1000
RemoteNickFormat="{NICK}"
# Optional: Token="your_secret_token"

[[gateway]]
name="verlihub"
enable=true

[[gateway.inout]]
account="api.verlihub"
channel="api"

[[gateway.inout]]
account="mattermost.yourserver"
channel="#sublevels"

# Add other platforms as needed
[[gateway.inout]]
account="slack.yourworkspace"
channel="general"

[[gateway.inout]]
account="irc.libera"
channel="#mychannel"
```

**Usage:**
```bash
# Load the script
!pyload /path/to/scripts/matterbridge.py

# Configure (optional, if defaults don't match)
!bridge config http://localhost:4242
!bridge gateway verlihub
!bridge channel #sublevels

# Start the bridge
!bridge start
```

**Message Flow:**

Hub chat:
```
<UserNick> Hello everyone!
```

Appears on all bridged platforms as:
```
<UserNick> Hello everyone!
```

Messages from other platforms appear in hub:
```
[Bridge] [slack] <john.doe> Hey from Slack!
[Bridge] [irc] <alice> IRC checking in
[Bridge] [mattermost] <bob> Message from Mattermost
```

**Configuration Options:**

The script has a `CONFIG` dictionary at the top that you can customize:

```python
CONFIG = {
    "api_url": "http://localhost:4242",        # Matterbridge API URL
    "api_token": "",                            # Optional authentication
    "gateway": "verlihub",                      # Gateway name
    "channel": "#sublevels",                    # Channel name
    "bot_nick": "[Bridge]",                     # Bot display name
    "hub_to_bridge_format": "<{nick}> {message}",
    "bridge_to_hub_format": "[{protocol}] <{username}> {text}",
    "poll_interval": 0.5,                       # Polling frequency
    "retry_interval": 5,                        # Retry delay
    "max_retries": 10,                          # Max reconnection attempts
    "ignore_nicks": [],                         # Nicks to filter
    "min_class_to_send": 0,                     # Min user class (0=all)
}
```

---

## Installation

### Using Virtual Environment (Recommended)

```bash
# Navigate to scripts directory
cd /path/to/verlihub/plugins/python/scripts

# Create virtual environment
python3 -m venv venv

# Activate virtual environment
source venv/bin/activate

# Install dependencies
pip install fastapi uvicorn requests

# Deactivate when done
deactivate
```

The scripts automatically detect and use the virtual environment if present.

### System-wide Installation

```bash
pip install fastapi uvicorn requests
```

## Development

### Creating New Scripts

When creating new Python scripts for Verlihub:

1. **Import the vh module:**
   ```python
   import vh
   ```

2. **Define event hooks** (all optional):
   ```python
   def OnParsedMsgChat(nick, message):
       # Handle chat messages
       return 1  # Allow message
   
   def OnUserLogin(nick):
       # Handle user login
       return 1
   
   def OnHubCommand(nick, command, user_class, in_pm, prefix):
       # Handle custom commands
       return 0  # Command handled
   
   def UnLoad():
       # Cleanup when script unloads
       pass
   ```

3. **Use threading for background tasks:**
   ```python
   import threading
   from queue import Queue
   
   # Safe pattern: queue-based communication
   work_queue = Queue()
   
   def background_worker():
       """Background thread for I/O - DON'T call vh.* here"""
       while running:
           data = do_network_io()
           work_queue.put(data)  # Queue for main thread
   
   def OnTimer(msec=0):
       """Process queued work in main thread - CAN call vh.* here"""
       try:
           data = work_queue.get_nowait()
           vh.SendToAll(data)  # ✅ Safe - main thread
       except:
           pass
       return 1
   
   thread = threading.Thread(target=background_worker, daemon=True)
   thread.start()
   ```

4. **Handle graceful shutdown:**
   ```python
   def UnLoad():
       global running
       running = False
       # Cleanup resources
   ```

### Best Practices

- **Thread Safety**: Never call `vh.*` functions from background threads (see [Thread Safety](#important-thread-safety))
- **Use OnTimer**: Process queued work and update caches in `OnTimer()` hook
- **Queue Pattern**: Use `queue.Queue()` to pass data from background threads to main thread
- **Cache Pattern**: Use thread-safe caches (with locks) for background threads to read hub data
- **Error Handling**: Wrap hub API calls in try/except blocks
- **Threading**: Use daemon threads for background tasks that should exit with the script
- **Resource Cleanup**: Always implement `UnLoad()` for cleanup
- **Permissions**: Check `user_class` before executing admin commands
- **Message Formatting**: Use `vh.pm()` for private messages, `vh.usermc()` for user main chat
- **Logging**: Use `print()` for console output (visible in hub logs)

## Documentation

For complete Python plugin documentation, see:
- [Main README](../README.md) - Full Python plugin documentation
- [Verlihub Wiki](https://github.com/42wim/matterbridge/wiki) - Matterbridge documentation
- [FastAPI Docs](https://fastapi.tiangolo.com/) - FastAPI documentation

## Contributing

When adding new example scripts:

1. Include comprehensive docstrings
2. Add admin commands with `!help` support
3. Implement proper error handling
4. Use virtual environment support
5. Document requirements and usage
6. Update this README with script information

## License

These scripts are part of the Verlihub project and follow the same license.

Copyright (C) 2025 Verlihub Team
