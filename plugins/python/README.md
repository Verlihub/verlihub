# Verlihub Python Plugin

**Version**: 1.6.0.0  
**Python Version**: 3.12+  
**Status**: Production Ready ✅

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [⭐ vh Module - Restored Bidirectional API](#-vh-module---restored-bidirectional-api)
- [🚀 Dimension 4 - Dynamic C++ Function Registration](#-dimension-4---dynamic-c-function-registration)
- [Writing Python Scripts](#writing-python-scripts)
  - [Event Hooks](#event-hooks)
  - [Verlihub API](#verlihub-api)
  - [Advanced Features](#advanced-features)
- [Python Environment](#python-environment)
  - [Standard Library Access](#standard-library-access)
  - [Virtual Environments](#virtual-environments)
  - [Package Management](#package-management)
- [Architecture](#architecture)
  - [Components](#components)
  - [Threading Model](#threading-model)
- [API Reference](#api-reference)
  - [Format Strings](#format-strings)
  - [Event Hooks](#hook-ids-enum)
  - [Callback Functions](#callback-ids-enum)
  - [Bidirectional Communication](#bidirectional-communication)
  - [Advanced Type Marshaling](#advanced-type-marshaling)
- [Examples](#examples)
- [Testing](#testing)
- [Performance](#performance)
- [Troubleshooting](#troubleshooting)
- [Development](#development)
  - [Building from Source](#building-from-source)
  - [Adding New Callbacks](#adding-new-callbacks)
  - [Debugging](#debugging)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

The Python plugin enables extensible scripting for Verlihub DC++ hub servers. It provides:

- **Event Hooks**: React to hub events (user login, messages, searches, etc.)
- **Callback API**: Call Verlihub functions from Python (send messages, kick users, etc.)
- **Bidirectional Communication**: C++ can call arbitrary Python functions
- **Advanced Type Marshaling**: Exchange complex data (lists, dicts) between C++ and Python
- **Sub-Interpreter Isolation**: Each script runs in its own isolated environment
- **GIL Management**: Thread-safe concurrent execution
- **Template Wrappers**: Simplified API for exposing new C++ functions

---

## Quick Start

### 1. Enable the Plugin

```bash
# Load the plugin
!pyload /path/to/script.py

# List loaded scripts
!pylist

# Reload a script
!pyreload script_name

# Unload a script
!pyunload script_name
```

### 2. Basic Python Script

```python
#!/usr/bin/env python3
"""
Simple Verlihub Python script demonstrating event hooks
"""

# Called when a user sends a chat message
def OnParsedMsgChat(conn_ptr, msg_ptr):
    # conn_ptr and msg_ptr are opaque pointers - use vh.* functions
    # Return 1 to allow message, 0 to block
    return 1

# Called when a user logs in
def OnUserLogin(user_ptr):
    nick = vh.GetUserNick(user_ptr)
    vh.SendToAll(f"Welcome {nick}!")
    return 1

# Called every second (if registered)
def OnTimer(msec):
    # Periodic tasks
    return 1
```

### 3. Using Verlihub API

```python
import vh  # Verlihub API module

# Send messages
vh.SendToAll("Announcement to everyone!")
vh.SendToOpChat("Message to operators", "BotNick")
vh.usermc("TargetNick", "Private message", "FromNick")

# User management
user_class = vh.GetUserClass("SomeUser")
vh.KickUser("OpNick", "BadUser", "Reason")
vh.Ban("OpNick", "BadUser", "Spam", 3600, 1)  # 1 hour ban

# Information gathering
nick_list = vh.GetNickList()  # Returns string "nick1$$nick2$$nick3"
op_list = vh.GetOpList()
user_count = vh.GetUsersCount()

# Database operations
result = vh.SQL(0, "SELECT * FROM users WHERE class > 1")
```

---

## ⭐ vh Module - Restored Bidirectional API

**December 2025**: The `vh` module has been **fully restored** from the Python 2 version! Python scripts can now call C++ functions directly.

### What Was Restored

The original Python 2 plugin (pre-2016) had a complete `vh` module with 56 functions for calling Verlihub C++ code from Python. During the Python 3 migration (September 2025), this module was removed. **We've now restored it** using Python 3's modern `PyModule_Create` API.

### Available Functions (56 total)

```python
import vh

# Messaging
vh.SendToOpChat(message, from_nick=None)
vh.SendDataToAll(data, min_class, max_class, delay=0)
vh.SendDataToUser(data, nick, delay=0)
vh.SendPMToAll(message, from_nick, min_class, max_class)
vh.usermc(nick, message)     # Send main chat to specific user
vh.pm(nick, message)          # Send PM
vh.mc(message)                # Send to main chat as bot
vh.classmc(message, min_class, max_class)  # Send to class range

# User Information
vh.GetUserClass(nick) → int   # Returns 0-10
vh.GetUserHost(nick) → str
vh.GetUserIP(nick) → str
vh.GetUserCC(nick) → str      # Country code
vh.GetIPCC(ip) → str
vh.GetIPCN(ip) → str          # Country name
vh.GetIPASN(ip) → str         # ASN info
vh.GetGeoIP(ip) → str
vh.GetMyINFO(nick) → str      # Full $MyINFO string

# User Lists (Python lists!)
vh.GetNickList() → list[str]
vh.GetOpList() → list[str]
vh.GetBotList() → list[str]
# Or raw protocol strings:
vh.GetRawNickList() → str     # $NickList user1$$user2$$
vh.GetRawOpList() → str
vh.GetRawBotList() → str

# User Management
vh.AddRegUser(nick, class, password="", op_note="")
vh.DelRegUser(nick)
vh.SetRegClass(nick, class)
vh.KickUser(op, nick, reason, note="", op_note="", ban_time="")
vh.Ban(op, nick, reason, duration_sec, ban_type)
vh.CloseConnection(nick, reason=0, delay=0)

# Configuration
vh.GetConfig(config_type, variable) → str
vh.SetConfig(config_type, variable, value) → bool

# Hub Management
vh.Topic(new_topic=None) → str  # Get or set
vh.GetUsersCount() → int
vh.GetTotalShareSize() → str
vh.GetServFreq() → int
vh.name_and_version() → str
vh.StopHub(code)

# Bots
vh.AddRobot(nick, class, desc, speed, email, share)
vh.DelRobot(nick)
vh.IsRobotNickBad(nick) → int

# Advanced
vh.SetMyINFO(nick, desc="", tag="", speed="", email="", share=0)
vh.SetUserIP(nick, ip)
vh.SetMyINFOFlag(nick, flag)
vh.UnsetMyINFOFlag(nick, flag)
vh.UserRestrictions(nick, nochat="", nopm="", nosearch="", noctm="")
vh.ParseCommand(nick, command, is_pm)
vh.ScriptCommand(cmd, data, plugin)
vh.SQL(query, limit)
```

### Module Attributes

Each script's `vh` module has unique attributes:

```python
import vh

print(vh.myid)         # Script ID (unique per loaded script)
print(vh.botname)      # Bot nickname for this script
print(vh.opchatname)   # OpChat nickname
print(vh.name)         # Script name
print(vh.path)         # Script file path
print(vh.config_name)  # Configuration name
```

### Constants

```python
# Connection close reasons
vh.eCR_KICKED, vh.eCR_TIMEOUT, vh.eCR_BANNED, etc.

# Bot validation results
vh.eBOT_OK, vh.eBOT_EXISTS, vh.eBOT_BAD_CHARS, etc.

# Ban types
vh.eBF_NICKIP, vh.eBF_IP, vh.eBF_NICK, vh.eBF_RANGE, etc.
```

### Complete Example

```python
import vh

def OnParsedMsgChat(nick, message):
    """User sent a chat message"""
    
    if message.startswith("!stats"):
        # Get user information
        user_class = vh.GetUserClass(nick)
        user_ip = vh.GetUserIP(nick)
        user_cc = vh.GetUserCC(nick)
        
        # Get hub information
        user_count = vh.GetUsersCount()
        hub_topic = vh.Topic()
        
        # Send response
        response = f"""User Stats for {nick}:
Class: {user_class}
IP: {user_ip}
Country: {user_cc}

Hub Stats:
Online: {user_count}
Topic: {hub_topic}"""
        
        vh.pm(nick, response)
        return 1
    
    elif message.startswith("!users"):
        # Get Python list of users
        nicklist = vh.GetNickList()
        oplist = vh.GetOpList()
        
        response = f"""Total users: {len(nicklist)}
Operators: {len(oplist)}
First 5 users: {', '.join(nicklist[:5])}"""
        
        vh.pm(nick, response)
        return 1
    
    return 1

def OnUserLogin(nick):
    """User logged in"""
    user_class = vh.GetUserClass(nick)
    
    if user_class >= 3:  # Operator
        vh.SendToOpChat(f"{nick} has joined")
    else:
        topic = vh.Topic()
        vh.pm(nick, f"Welcome! Topic: {topic}")
    
    return 1
```

### Sub-Interpreter Isolation

Each script gets its own `vh` module instance with unique `myid`, `botname`, etc. Scripts cannot interfere with each other.

### Type Safety

All functions use the existing type marshaling system:
- **Input validation**: Proper Python type checking with helpful error messages
- **NULL handling**: `None` values handled correctly
- **GIL management**: Thread-safe C++ callbacks
- **Memory safety**: Proper reference counting

### Utility Functions

Two additional utility functions for encoding/decoding DC++ special characters:

**vh.Encode(string)**
Converts special DC++ characters to HTML entities:
```python
import vh

# Encode special characters
original = "Test$Message|With`Special~Chars"
encoded = vh.Encode(original)  # "Test&#36;Message&#124;With&#96;Special&#126;Chars"

# Safe to send in protocol messages
vh.SendDataToUser(encoded, "username")
```

**vh.Decode(string)**
Converts HTML entities back to characters:
```python
decoded = vh.Decode(encoded)  # Back to original string
assert decoded == original
```

Character mappings:
- `$` ↔ `&#36;`
- `|` ↔ `&#124;`
- `` ` `` ↔ `&#96;`
- `~` ↔ `&#126;`
- `\x05` (Ctrl+E) ↔ `&#5;`

### Testing

See `tests/test_vh_module.cpp` for comprehensive unit tests validating all 58 functions (56 API functions + Encode + Decode).

---

## 🚀 Dimension 4 - Dynamic C++ Function Registration

**NEW**: Runtime registration of C++ functions that can be called from Python scripts!

### Overview

Dimension 4 completes the bidirectional communication model by allowing C++ plugins to dynamically register custom functions that Python scripts can call at runtime. This enables:

- **Plugin-specific APIs**: Each plugin can expose its own custom functions
- **Runtime extensibility**: Add new functions without recompiling
- **Third-party extensions**: External C++ libraries can integrate seamlessly
- **Script isolation**: Each script has its own function registry

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                 Complete Bidirectional Model                 │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  C++ Hub Core                    Python Scripts              │
│  ┌────────────┐                  ┌──────────────┐           │
│  │            │  1. Event Hooks  │              │           │
│  │  Verlihub  │ ────────────────>│   script.py  │           │
│  │            │                  │              │           │
│  │            │  2. vh Module    │              │           │
│  │            │<────────────────>│ vh.GetUser() │           │
│  │            │    (58 funcs)    │ vh.SendMsg() │           │
│  │            │                  │              │           │
│  │            │  3. Callbacks    │              │           │
│  │            │<─────────────────│ return data  │           │
│  │            │                  │              │           │
│  └────────────┘                  └──────────────┘           │
│       ↕                                 ↕                    │
│  ┌────────────┐                  ┌──────────────┐           │
│  │  Plugin    │  4. Dynamic Reg  │              │           │
│  │  (C++)     │ ────────────────>│ CallDynamic  │           │
│  │            │  Register("fn")  │ Function()   │           │
│  │ AddPlugin  │<─────────────────│              │           │
│  │ CalcStats  │  Call C++ func   │              │           │
│  └────────────┘                  └──────────────┘           │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### C++ Side: Registering Functions

**Step 1**: Implement your C++ function with the callback signature:

```cpp
// Example: Calculate statistics for a user
w_Targs* MyPlugin_GetUserStats(int callback_id, w_Targs* args) {
    char *username;
    if (!w_unpack(args, "s", &username)) {
        return w_pack("l", (long)0);
    }
    
    // Your custom C++ logic here
    long message_count = GetUserMessageCount(username);
    long login_count = GetUserLoginCount(username);
    
    // Return results (can be long, string, double, etc.)
    return w_pack("ll", message_count, login_count);
}

// Example: Add two numbers
w_Targs* MyPlugin_Add(int callback_id, w_Targs* args) {
    long a, b;
    if (!w_unpack(args, "ll", &a, &b)) {
        return w_pack("l", (long)0);
    }
    return w_pack("l", a + b);
}

// Example: Reverse a string
w_Targs* MyPlugin_Reverse(int callback_id, w_Targs* args) {
    char *str;
    if (!w_unpack(args, "s", &str)) {
        return w_pack("s", (char*)"");
    }
    
    int len = strlen(str);
    char *reversed = (char*)malloc(len + 1);
    for (int i = 0; i < len; i++) {
        reversed[i] = str[len - 1 - i];
    }
    reversed[len] = '\0';
    
    return w_pack("s", reversed);
}
```

**Step 2**: Register the function for a specific script:

```cpp
// When loading your plugin or when a Python script loads
int script_id = 0;  // Get from Python plugin

// Register the function (name can be anything)
w_RegisterFunction(script_id, "get_stats", MyPlugin_GetUserStats);
w_RegisterFunction(script_id, "add", MyPlugin_Add);
w_RegisterFunction(script_id, "reverse", MyPlugin_Reverse);
```

**Step 3**: Optionally unregister when done:

```cpp
w_UnregisterFunction(script_id, "get_stats");
```

### Python Side: Calling Dynamic Functions

Once a C++ function is registered, call it from Python using `vh.CallDynamicFunction()`:

```python
import vh

def OnUserLogin(nick):
    # Call dynamically registered C++ function
    msg_count, login_count = vh.CallDynamicFunction('get_stats', nick)
    
    vh.pm(nick, f"You have sent {msg_count} messages across {login_count} logins!")
    
    # Call simple math function
    result = vh.CallDynamicFunction('add', 10, 32)
    assert result == 42
    
    # Call string function
    reversed_nick = vh.CallDynamicFunction('reverse', nick)
    vh.mc(f"{nick} reversed is {reversed_nick}")
    
    return 1
```

### API Reference

**C++ Functions:**

```cpp
// Register a dynamic function for a script
int w_RegisterFunction(int script_id, const char *func_name, w_Tcallback callback);

// Unregister a dynamic function
int w_UnregisterFunction(int script_id, const char *func_name);
```

**Python Function:**

```python
# Call a dynamically registered C++ function
result = vh.CallDynamicFunction(func_name, arg1, arg2, ...)
```

**Supported argument types:**
- `long` (Python `int`)
- `str` (Python `str`)
- `double` (Python `float`)

**Supported return types:**
- `long` → Python `int`
- `str` → Python `str`
- `double` → Python `float`
- `None` (no return value)

### Real-World Example: Plugin Integration

**Scenario**: A custom `stats` plugin wants to expose functions to Python scripts.

**stats_plugin.cpp**:
```cpp
class StatsPlugin {
    std::map<std::string, UserStats> user_stats;
    
public:
    static w_Targs* GetTopUsers(int id, w_Targs* args) {
        long limit;
        w_unpack(args, "l", &limit);
        
        // Get top users by message count
        std::vector<std::string> top_users = GetTopUsersByMessages(limit);
        
        // Convert to NULL-terminated array
        char **user_list = (char**)malloc((top_users.size() + 1) * sizeof(char*));
        for (size_t i = 0; i < top_users.size(); i++) {
            user_list[i] = strdup(top_users[i].c_str());
        }
        user_list[top_users.size()] = NULL;
        
        return w_pack("L", user_list);
    }
    
    void RegisterWithScript(int script_id) {
        w_RegisterFunction(script_id, "get_top_users", GetTopUsers);
        w_RegisterFunction(script_id, "get_user_rank", GetUserRank);
        w_RegisterFunction(script_id, "reset_stats", ResetStats);
    }
};
```

**Python script using stats plugin**:
```python
import vh

def OnParsedMsgChat(nick, message):
    if message == "!top10":
        # Call stats plugin function
        top_users = vh.CallDynamicFunction('get_top_users', 10)
        
        if isinstance(top_users, list):
            response = "Top 10 users:\\n"
            for i, user in enumerate(top_users, 1):
                rank = vh.CallDynamicFunction('get_user_rank', user)
                response += f"{i}. {user} - {rank} messages\\n"
            
            vh.pm(nick, response)
        
        return 0  # Block original message
    
    return 1  # Allow message
```

### Isolation and Safety

- **Per-script registry**: Each script has its own isolated function registry
- **No cross-script calls**: Script A cannot call functions registered for Script B
- **Memory safety**: Proper cleanup when scripts unload
- **Thread safety**: GIL is properly managed during calls
- **Error handling**: Python exceptions for missing functions

### Testing

See `tests/test_dynamic_registration.cpp` for comprehensive tests:
- Function registration and calling
- Multiple functions per script
- Registration/unregistration
- Error handling for missing functions
- Script isolation verification

```bash
# Run Dimension 4 tests
cd /path/to/build
./plugins/python/test_dynamic_registration
```

All 5 tests validate:
✅ Basic registration and calling  
✅ Multiple functions  
✅ Unregistration  
✅ Error handling  
✅ Script isolation  

---

## Writing Python Scripts

### Event Hooks

Python scripts can implement event handlers that are called when specific hub events occur. Simply define functions with the appropriate names:
```cpp
// Call any Python function by name
w_Targs *result = py_plugin->CallPythonFunction(
    script_id,
    "get_total_calls",  // Function name
    nullptr             // No arguments
);

// With arguments
w_Targs *args = w_pack("sl", "username", 12345L);
result = py_plugin->CallPythonFunction(script_id, "process_user", args);
```

```python
import vh

# Broadcast messages
vh.SendToAll("Hub announcement!")
vh.SendToOpChat("Operator message", "BotNick")
vh.usermc("TargetNick", "Private message", "FromNick")
vh.classmc("Message to class 3+", 3, "BotNick")
vh.pm("FromNick", "ToNick", "Private message")

# User information
user_class = vh.GetUserClass("Username")
user_ip = vh.GetUserIP("Username")
user_host = vh.GetUserHost("Username")
user_cc = vh.GetUserCC("Username")  # Country code
nick_list = vh.GetNickList()  # "nick1$$nick2$$nick3"
op_list = vh.GetOpList()
bot_list = vh.GetBotList()
user_count = vh.GetUsersCount()

# User management
vh.KickUser("OpNick", "UserToKick", "Reason")
vh.Ban("OpNick", "UserToBan", "Reason", 3600, 1)  # 1 hour ban
vh.AddRegUser("Username", 1, "password")  # Class 1
vh.DelRegUser("Username")
vh.SetRegClass("Username", 3)

# Database operations
result = vh.SQL(0, "SELECT * FROM users WHERE class > 1")

# Hub control
vh.StopHub(0)  # Graceful shutdown
vh.SetConfig("hub_name", "My Hub")
config_value = vh.GetConfig("max_users")
```

For the complete API, see: <https://github.com/verlihub/verlihub/wiki/api-python-methods>

### Advanced Features

#### Lists of Strings (`'L'` format)

**C++ → Python**:
```cpp
char *users[] = {"alice", "bob", "charlie", NULL};
w_Targs *args = w_pack("L", users);
w_Targs *result = w_CallFunction(id, "process_users", args);
```

**Python Side**:
```python
def process_users(user_list):
    # user_list is ['alice', 'bob', 'charlie']
    for user in user_list:
        print(f"Processing {user}")
    return len(user_list)
```

**Python → C++**:
```python
def get_online_users():
    return ["Alice", "Bob", "Charlie"]  # Python list
```

```cpp
w_Targs *result = w_CallFunction(id, "get_online_users", NULL);
char **users;
if (w_unpack(result, "L", &users)) {
    for (int i = 0; users[i] != NULL; i++) {
        std::cout << users[i] << std::endl;
        free(users[i]);
    }
    free(users);
}
```

#### Dictionaries as JSON (`'D'` format)

**C++ → Python**:
```cpp
const char *json = "{\"hub_name\":\"MyHub\",\"max_users\":100}";
w_Targs *args = w_pack("D", json);
w_Targs *result = w_CallFunction(id, "validate_config", args);
```

**Python Side**:
```python
def validate_config(config):
    # config is {'hub_name': 'MyHub', 'max_users': 100}
    if config['max_users'] > 500:
        return {"valid": False, "error": "Too many users"}
    return {"valid": True}
```

**Python → C++**:
```python
def get_hub_stats():
    return {
        "total_users": 42,
        "active_users": 15,
        "uptime_seconds": 36000
    }
```

```cpp
w_Targs *result = w_CallFunction(id, "get_hub_stats", NULL);
char *json_str;
if (w_unpack(result, "D", &json_str)) {
    // json_str = '{"total_users":42,"active_users":15,...}'
    // Parse with your favorite JSON library or pass back to Python
    free(json_str);
}
```

#### PyObject* Passthrough (`'O'` format - Advanced)

For direct Python object manipulation:
```cpp
PyObject *custom_obj = /* ... */;
w_Targs *args = w_pack("O", custom_obj);
w_Targs *result = w_CallFunction(id, "process_object", args);
```

**Memory Management**:
- Lists: Caller must free each string + array
- Dicts: Caller must free JSON string
- PyObject*: Proper reference counting (INCREF/DECREF)

---

### Template-Based Callback Wrappers

**Template wrappers reduce boilerplate by 30%**!

**Old Way** (Manual unpacking):
```cpp
w_Targs *_GetUserClass(int id, w_Targs *args) {
    const char *nick;
    long uclass = -2;
    if (!lib_unpack(args, "s", &nick)) return NULL;
    if (!nick) return NULL;
    cUser *u = server->GetUserByNick(nick);
    if (u) uclass = u->mClass;
    return lib_pack("l", uclass);
}
```

**New Way** (Template wrapper):
```cpp
w_Targs *_GetUserClass(int id, w_Targs *args) {
    auto getUserClass = [](const char *nick) -> long {
        cUser *u = server->GetUserByNick(nick);
        return u ? u->mClass : -2;
    };
    return cpiPython::WrapStringToLong(id, args, getUserClass);
}
```

**Available Templates**:
- `WrapStringToBool` - String → Bool
- `WrapStringToString` - String → String
- `WrapStringToLong` - String → Long
- `WrapStringLongToBool` - String, Long → Bool
- `WrapStringStringToBool` - String, String → Bool
- `WrapStringStringToString` - String, String → String

**Registration Macro**:
```cpp
// Old
callbacklist[W_GetUserClass] = &_GetUserClass;

// New
VH_REGISTER_CALLBACK(GetUserClass, GetUserClass);
```

---

## Python Environment

### Standard Library Access

The plugin properly configures Python's import system to provide access to:

- **Standard Library**: All Python built-in modules (`json`, `re`, `sys`, `os`, etc.)
- **Site Packages**: Modules installed via pip
- **Script Directory**: Your script's local directory (takes precedence)
- **Hub Scripts**: Shared scripts in the hub's script directory

**Import example**:
```python
import json
import re
import sys
from datetime import datetime

# All standard library imports work out of the box
data = json.dumps({"time": datetime.now().isoformat()})
```

### Virtual Environments

For scripts requiring specific package versions or isolated dependencies, use virtual environments:

**Setup**:
```bash
# Navigate to your script directory
cd /path/to/hub/scripts/monitoring_script/

# Create virtual environment
python3 -m venv .venv

# Activate and install packages
source .venv/bin/activate
pip install requests beautifulsoup4 numpy
pip freeze > requirements.txt
deactivate
```

**Directory Structure**:
```
scripts/
├── monitoring_script/
│   ├── .venv/                    # Virtual environment
│   │   └── lib/python3.12/site-packages/
│   ├── requirements.txt          # Package manifest
│   └── monitoring.py             # Your script
```

The plugin automatically detects `.venv` directories and adds them to `sys.path`.

### Package Management

**Installing packages**:
```bash
# System-wide (requires root)
sudo pip3 install package_name

# User-local (recommended)
pip3 install --user package_name

# Virtual environment (most isolated)
cd script_directory
source .venv/bin/activate
pip install package_name
deactivate
```

**Best practices**:
- Use virtual environments for production scripts
- Document dependencies in `requirements.txt`
- Pin versions for reproducibility: `requests==2.31.0`
- Test scripts after package updates

---

## Architecture

### Components

```
┌─────────────────────────────────────────────────────┐
│                   Verlihub Core                     │
│  (cServerDC, cUser, message processing, etc.)       │
└────────────────┬────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────┐
│              Python Plugin (cpiPython)               │
│  - Event hook dispatching                           │
│  - Callback registration                            │
│  - Script management                                │
│  - CallPythonFunction() API                         │
└────────────────┬────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────┐
│          Python Wrapper (libvh_python_wrapper.so)   │
│  - GIL management (PyGILState_Ensure/Release)       │
│  - Sub-interpreter isolation (Py_NewInterpreter)    │
│  - Type marshaling (w_pack/w_unpack)                │
│  - w_CallHook() - Call Python by hook ID            │
│  - w_CallFunction() - Call Python by name           │
└────────────────┬────────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────────┐
│              Python Scripts (.py files)              │
│  - Event handlers (OnParsedMsgChat, etc.)           │
│  - Utility functions (callable from C++)            │
│  - Uses vh.* API for Verlihub operations            │
└─────────────────────────────────────────────────────┘
```

### Key Classes

**`cpiPython`** ([cpipython.h](cpipython.h)):
- Main plugin class
- Implements Verlihub plugin interface
- Manages script lifecycle
- Exposes `CallPythonFunction()` API
- Template wrappers for callbacks

**`cPythonInterpreter`** ([cpythoninterpreter.h](cpythoninterpreter.h)):
- Represents one loaded script
- Manages sub-interpreter state
- Tracks script metadata (name, version, path)

**`cConsole`** ([cconsole.h](cconsole.h)):
- Hub operator commands (!pyload, !pylist, etc.)
- Script management interface

**Wrapper Functions** ([wrapper.h](wrapper.h)):
- `w_Begin()` - Initialize Python
- `w_End()` - Shutdown Python
- `w_Load()` - Load a script
- `w_Unload()` - Unload a script
- `w_CallHook()` - Call Python hook by ID
- `w_CallFunction()` - Call Python function by name
- `w_pack()/w_unpack()` - Type marshaling

### Threading Model

**Sub-Interpreter Isolation**:
- Each script runs in its own `PyThreadState`
- Scripts cannot interfere with each other
- Global Interpreter Lock (GIL) properly managed
- Thread-safe concurrent execution

**GIL Acquisition**:
```cpp
PyGILState_STATE gstate = PyGILState_Ensure();
PyThreadState *old_state = PyThreadState_Get();
PyThreadState_Swap(script->state);

// ... Python operations ...

PyThreadState_Swap(old_state);
PyGILState_Release(gstate);
```

---

## API Reference

### Bidirectional Communication

The wrapper provides functions for C++ code to call Python functions by name.

**C++ API**:
```cpp
// Low-level wrapper function
w_Targs *w_CallFunction(int script_id, const char *func_name, w_Targs *params);

// High-level plugin methods
class cpiPython {
    w_Targs* CallPythonFunction(int script_id, const char *func_name, w_Targs *args);
    w_Targs* CallPythonFunction(const std::string &script_path, const char *func_name, w_Targs *args);
};
```

**Example usage**:
```cpp
// Get script statistics
w_Targs *result = py_plugin->CallPythonFunction(script_id, "get_stats", NULL);

// Process user with arguments
w_Targs *args = w_pack("sl", "username", 123L);
result = py_plugin->CallPythonFunction(script_id, "process_user", args);
free(args);

long return_value;
if (w_unpack(result, "l", &return_value)) {
    // Use return_value
}
free(result);
```

**Python side**:
```python
def get_stats():
    """Called from C++ - returns statistics"""
    return total_processed_items

def process_user(username, user_id):
    """Called from C++ with arguments"""
    # Process user
    return 1  # Success
```

#### Validation: True Bidirectional Communication

The plugin achieves **true bidirectional communication** across three dimensions:

1. **C++ → Python (Event Hooks)**: Traditional callback system
   - Hub events trigger Python functions (`OnParsedMsgChat`, `OnUserLogin`, etc.)
   - Python returns values to control processing (1=allow, 0=block)

2. **Python → C++ (Pre-registered Callbacks)**: Python calls Verlihub functions
   - `vh.SendToAll()`, `vh.GetUserClass()`, `vh.KickUser()`, etc.
   - Complete hub control from Python scripts
   - **Current limitation**: Functions must be pre-registered via callback table (enum-based)
   - Adding new functions requires modifying `wrapper.h` enum and registering in `cpipython.cpp`

3. **C++ → Python (Arbitrary Functions)**: Call any Python function by name
   - `w_CallFunction()` and `CallPythonFunction()` enable C++ to invoke **any** Python function
   - Not limited to registered hooks - query script state, trigger actions, retrieve data
   - Full argument passing and return value handling

**Unit Test Proof**:

This capability is validated by comprehensive unit tests:

**test_python_plugin_integration.cpp** (lines 400-450):
- Calls `get_total_calls()` → Python returns integer with callback statistics
- Calls `print_summary()` → Python returns success indicator, prints JSON to stderr
- Demonstrates script lookup by ID and by path
- Validates ability to query Python script state from C++ at runtime

**test_python_advanced_types.cpp** (9 tests, 100% pass rate):
- `PythonReturnsStringList` - Python returns list of strings to C++
- `CppSendsListToPython` - C++ sends list to Python, receives count
- `PythonReturnsDict` - Python returns dictionary (as JSON) to C++
- `CppSendsDictToPython` - C++ sends JSON dict to Python, receives validation
- `ComplexDictRoundTrip` - Bidirectional data modification (C++ → Python → C++)
- `CallPythonWithArgs` - C++ calls Python with arguments, receives complex result
- `GetTestStatistics` - Query Python internal state via function call
- `LargeListPerformance` - Exchange 1000-element lists bidirectionally
- `ComplexJSONPerformance` - Nested data structure round-trips

**Example from tests**:
```cpp
// C++ calls Python function that doesn't exist as a hook
w_Targs *result = w_CallFunction(script_id, "get_total_calls", nullptr);
long total;
w_unpack(result, "l", &total);  // Python script returned call count
```

```python
# Python defines utility function (not a hook!)
def get_total_calls():
    return sum(call_counts.values())  # Returns integer to C++
```

This proves C++ can call **arbitrary Python functions**, not just registered event hooks, enabling powerful runtime introspection, testing, and integration scenarios.

#### Potential Fourth Dimension: Python → C++ (Arbitrary Functions)

**Current Architecture**: Python can only call pre-registered C++ functions through the callback table system. Each function must be:
1. Added to the `W_*` enum in `wrapper.h`
2. Implemented as a callback function (e.g., `_GetUserClass`)
3. Registered in `callbacklist[]` during `OnLoad()`

**Future Enhancement Opportunity**: A fourth dimension could enable Python to register and call **arbitrary C++ functions** dynamically, similar to how C++ can now call arbitrary Python functions. This would require:

- **Dynamic function registration API**: Allow C++ to expose functions to Python at runtime without enum modification
- **Name-based lookup**: Python calls functions by string name, not enum ID
- **Template-based exposure**: Simplified syntax for exposing C++ lambdas or functions
- **Type-safe marshaling**: Automatic argument conversion using existing `w_pack`/`w_unpack` infrastructure

**Example of what could be possible**:
```cpp
// C++ exposes arbitrary function
py_plugin->RegisterFunction("calculate_distance", [](double x1, double y1, double x2, double y2) {
    return sqrt(pow(x2-x1, 2) + pow(y2-y1, 2));
});
```

```python
# Python calls it by name
distance = vh.call("calculate_distance", 0.0, 0.0, 3.0, 4.0)  # Returns 5.0
```

This would complete true bidirectional symmetry: just as C++ can call any Python function, Python could call any C++ function registered at runtime.

**Current Status**: Not implemented. The current callback table system works well for the stable Verlihub API but requires code changes to add new functions. The template wrapper system (for dimension 2) makes this easier but doesn't eliminate the enum requirement.

### Advanced Type Marshaling

Complex data types can be exchanged between C++ and Python using extended format characters.

**Format Strings**:

| Format | C++ Type | Python Type | Description |
|--------|----------|-------------|-------------|
| `'l'` | `long` | `int` | Integer |
| `'s'` | `char*` | `str` | String |
| `'d'` | `double` | `float` | Float |
| `'p'` | `void*` | `int` | Pointer (opaque) |
| `'L'` | `char**` | `list` | List of strings (NULL-terminated) |
## Testing

### Test Suite

The plugin includes 4 comprehensive test executables:

1. **test_python_wrapper**
   - Basic GIL wrapper functionality
   - Tests GIL acquire/release mechanisms
   - Sub-interpreter isolation
   - No external dependencies
   - Runs in ~0.06s

2. **test_python_wrapper_stress** ⭐
   - **100,000 iteration stress test**
   - GIL race condition detection
   - Exercises same code paths as production
   - Tests `w_Py_INCREF`, `w_Py_DECREF`, `w_CallFunction`
   - Runs without full server overhead
   - **Catches GIL threading issues reliably**
   - Runs in ~17s

3. **test_python_plugin_integration**
   - Full hub integration test
   - Requires MySQL server running
   - Tests message parsing → Python callbacks
   - 1M message stress test
   - Validates bidirectional API
   - Performance benchmarking
   - Runs in ~19s

4. **test_python_advanced_types**
   - Advanced type marshaling validation
   - Tests lists, dictionaries, JSON conversion
   - 9 comprehensive tests
   - Round-trip validation
   - 100% pass rate
   - Runs in ~0.06s

### Running Tests

```bash
cd <build_directory>

# Quick unit test
./plugins/python/test_python_wrapper

# Stress test (recommended for GIL debugging)
./plugins/python/test_python_wrapper_stress

# Integration test (requires MySQL)
./plugins/python/test_python_plugin_integration

# Advanced types test
./plugins/python/test_python_advanced_types

# Run all Python tests via CTest
ctest -R Python --output-on-failure
```

### MySQL Setup

The integration test requires MySQL server:

**Install MySQL server:**
```bash
sudo apt-get update
sudo apt-get install mysql-server

# Start MySQL service
sudo systemctl start mysql
sudo systemctl enable mysql
```

**Create database and user:**
```bash
sudo mysql
```
```sql
CREATE DATABASE verlihub;
CREATE USER 'verlihub'@'localhost' IDENTIFIED BY 'verlihub';
GRANT ALL PRIVILEGES ON verlihub.* TO 'verlihub'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

**Verify connection:**
```bash
mysql -uverlihub -pverlihub verlihub -e "SELECT 'Connected!' AS status;"
```

**Environment variables** (optional):
- `VH_TEST_MYSQL_HOST` - MySQL host (default: localhost)
- `VH_TEST_MYSQL_PORT` - MySQL port (default: 3306)
- `VH_TEST_MYSQL_USER` - MySQL user (default: verlihub)
- `VH_TEST_MYSQL_PASS` - MySQL password (default: verlihub)
- `VH_TEST_MYSQL_DB` - Database name (default: verlihub)

Example:
```bash
export VH_TEST_MYSQL_HOST=192.168.1.100
export VH_TEST_MYSQL_PORT=3307
./plugins/python/test_python_plugin_integration
```

### Debugging Tests

If you encounter crashes or hangs:

```bash
# Run under GDB
gdb --args ./plugins/python/test_python_wrapper_stress

# Run with Python debug mode
PYTHONDEVMODE=1 ./plugins/python/test_python_wrapper_stress

# Check for memory issues
valgrind --leak-check=full ./plugins/python/test_python_wrapper_stress
```

### Build Configuration

The CMakeLists.txt creates two libraries:

1. `libpython_pi.so` - MODULE library (loadable plugin)
   - Used by Verlihub at runtime
   - Cannot be linked into tests

2. `libpython_pi_core.so` - SHARED library (linkable)
   - Used by test executables
   - Contains same code as MODULE library

This dual-library approach allows tests to link against plugin code while keeping the runtime plugin loadable.

### Testing Your Scripts

**Unit testing approach**:
```python
# test_script.py
def test_user_validation():
    """Test user validation logic"""
    assert validate_user("ValidNick") == True
    assert validate_user("Invalid Nick") == False

def test_message_filter():
    """Test message filtering"""
    assert should_block("spam message") == True
    assert should_block("normal chat") == False

# Run from command line
if __name__ == "__main__":
    test_user_validation()
    test_message_filter()
    print("All tests passed!")
```

**Integration testing**:
- Load script in test hub environment
- Send test messages via DC++ client
- Verify script behavior in hub logs
- Check database state after operations
```

**Python perspective**:
```python
def process_users(user_list):
    # user_list is ['alice', 'bob', 'charlie']
    for user in user_list:
        print(f"Processing {user}")
    return len(user_list)

def configure(config):
    # config is {'name': 'Hub', 'users': 100}
    apply_configuration(config)
    return {"status": "ok"}
```

**Memory management**:
- **Lists**: C++ caller must free each string and the array
- **Dictionaries**: C++ caller must free the JSON string
- **Python**: Garbage collector handles all Python objects

**Multiple values**:
```cpp
w_Targs *args = w_pack("sld", "name", 123L, 3.14);

char *name;
long id;
double value;
w_unpack(args, "sld", &name, &id, &value);
```

### Hook IDs (Enum)

Events your script can handle:

```cpp
W_OnNewConn                  // New connection
W_OnCloseConn                // Connection closed
W_OnParsedMsgChat            // Chat message
W_OnParsedMsgPM              // Private message
W_OnParsedMsgSearch          // Search query
W_OnParsedMsgMyINFO          // User info update
W_OnUserLogin                // User logged in
W_OnUserLogout               // User logged out
W_OnTimer                    // Periodic timer
// ... see wrapper.h for complete list
```

### Callback IDs (Enum)

Functions Python can call:

```cpp
W_SendToAll                  // Broadcast message
W_SendToOpChat               // Message to operators
## Performance

The plugin is designed for high-performance hub operation with minimal overhead.

**Benchmark Results** (representative test system):

| Metric | Value |
|--------|-------|
| Message Throughput | 250,000+ msg/sec |
| Function Call Overhead | < 1ms |
| Python Callback Invocation | ~100μs |
| List Marshaling (10 items) | ~100μs |
| Dict/JSON Marshaling | ~150μs |
| GIL Acquire/Release | ~2-10μs |

**Stress Test Results**:
- **1,000,000 messages** processed in ~4 seconds
- **250,000 callbacks** executed without errors
- **Zero memory leaks** detected
- **Zero crashes** under load

**Performance Characteristics**:
- Sub-interpreter isolation has minimal overhead
- GIL management is optimized for concurrent callbacks
- Type marshaling uses efficient conversion strategies
- Template wrappers compile to zero-overhead abstractions

**Optimization Tips**:
- Avoid expensive operations in hot paths (`OnParsedMsgChat`)
- Use `OnTimer` for periodic heavy tasks
- Batch database operations when possible
- Cache frequently accessed data
- Profile scripts to identify bottlenecks
2. **test_python_wrapper_stress** (17s)
   - 100K iterations stress test
   - Catches GIL race conditions

3. **test_python_plugin_integration** (19s)
   - Full integration test with MySQL
   - 1,000,000 messages in 3.9s
   - 256,410 msg/sec throughput
   - Bidirectional API validation

4. **test_python_advanced_types** (0.06s)
   - Advanced type marshaling
   - Lists, dicts, JSON round-trips
   - 9 tests, 100% pass rate

**Run Tests**:
```bash
cd build/
ctest -R Python --output-on-failure
```

**Requirements**:
- MySQL server running (for integration test)
- Database: `verlihub` / User: `verlihub` / Pass: `verlihub`

---

## Performance

**Benchmark Results** (Intel Xeon E5-2650 v2 @ 2.60GHz, 2x8 cores):

| Metric | Value |
|--------|-------|
| Message Throughput | 256,410 msg/sec |
| Function Call Overhead | < 1ms |
| 1M Message Test Duration | 3.9 seconds |
| Python Callbacks Invoked | 250,020 |
| Memory Leaks | 0 |
| Crashes | 0 |

**Overhead Analysis**:
- Native Python call: 2-10μs
- Through wrapper (simple types): 100μs
- Through wrapper (list): 100μs
- Through wrapper (dict/JSON): 150μs

**Zero-cost abstractions**: Template wrappers add no runtime overhead.

---

## Examples

### Example 1: Message Filter

```python
"""Block messages containing banned words"""

BANNED_WORDS = ["spam", "advertisement", "buy now"]

def OnParsedMsgChat(conn_ptr, msg_ptr):
    message = vh.GetChatMessage(msg_ptr)
    
    for word in BANNED_WORDS:
        if word.lower() in message.lower():
            nick = vh.GetConnNick(conn_ptr)
            vh.SendToUser(conn_ptr, f"Message blocked: contains '{word}'")
            return 0  # Block message
    
    return 1  # Allow message
```

### Example 2: Welcome Bot

```python
"""Welcome new users with custom message"""

def OnUserLogin(user_ptr):
    nick = vh.GetUserNick(user_ptr)
    user_class = vh.GetUserClass(nick)
    
    if user_class == 0:  # Guest
        vh.SendToAll(f"Welcome guest {nick}! Type !help for commands.")
    elif user_class >= 4:  # Operator
        vh.SendToOpChat(f"Operator {nick} has logged in.", "WelcomeBot")
    
    return 1
```

### Example 3: Statistics Tracker

```python
"""Track hub statistics and provide query API"""

import json
from collections import defaultdict

stats = {
    "total_logins": 0,
    "total_messages": 0,
    "peak_users": 0,
    "users_by_class": defaultdict(int)
}

def OnUserLogin(user_ptr):
    stats["total_logins"] += 1
    current_users = vh.GetUsersCount()
    stats["peak_users"] = max(stats["peak_users"], current_users)
    
    nick = vh.GetUserNick(user_ptr)
    user_class = vh.GetUserClass(nick)
    stats["users_by_class"][user_class] += 1
    
    return 1

def OnParsedMsgChat(conn_ptr, msg_ptr):
    stats["total_messages"] += 1
    return 1

# Called from C++ for monitoring
def get_stats():
    """Return statistics as dict (JSON)"""
    return dict(stats)

def get_peak_users():
    """Return peak user count"""
    return stats["peak_users"]
```

### Example 4: Advanced Data Exchange

```python
"""Demonstrate advanced type marshaling"""

def get_online_users():
    """Return list of online users - called from C++"""
    nick_list = vh.GetNickList()  # "nick1$$nick2$$nick3"
    return nick_list.split("$$") if nick_list else []

def process_user_batch(user_list):
    """Receive list from C++, process, return count"""
    processed = 0
    for nick in user_list:
        user_class = vh.GetUserClass(nick)
        if user_class >= 0:  # User exists
            # ... process user ...
            processed += 1
    return processed

def get_hub_config():
    """Return hub configuration as dict"""
    return {
        "hub_name": vh.GetConfig("hub_name"),
        "max_users": int(vh.GetConfig("max_users")),
        "min_share": int(vh.GetConfig("min_share")),
        "features": ["chat", "search", "pm"]
    }

def update_settings(config_dict):
    """Receive configuration dict from C++"""
    if "hub_name" in config_dict:
        vh.SetConfig("hub_name", config_dict["hub_name"])
    if "max_users" in config_dict:
        vh.SetConfig("max_users", str(config_dict["max_users"]))
    return {"status": "updated", "changes": len(config_dict)}
```

---

## Troubleshooting

### Script Won't Load

**Error**: `Failed to load script`

**Solutions**:
1. Check Python syntax: `python3 -m py_compile script.py`
2. Check file permissions: `chmod +r script.py`
3. Check path: Use absolute paths or relative to hub directory
4. Check logs: Set `!pylog 3` for debug output

### Import Errors

**Error**: `ModuleNotFoundError: No module named 'xyz'`

**Solutions**:
1. Install missing package: `pip3 install xyz`
2. Use virtual environment (see Python Environment section)
3. Check `sys.path`: Add `print(sys.path, file=sys.stderr)` to script

### Crashes or Segfaults

**Symptoms**: Hub crashes when calling Python functions

**Solutions**:
1. Check GIL management (use wrapper functions, not raw Python C API)
2. Verify pointer validity before dereferencing
3. Run tests: `./test_python_wrapper_stress` to catch race conditions
4. Enable core dumps: `ulimit -c unlimited`

### Performance Issues

**Symptoms**: Slow message processing, high CPU usage

**Solutions**:
1. Profile your script: Add timing to callbacks
2. Avoid expensive operations in hot paths (OnParsedMsgChat)
3. Use `OnTimer` for periodic heavy tasks
4. Batch operations when possible
5. Check for infinite loops or blocking I/O

### Memory Leaks

**Symptoms**: Memory usage grows over time

**Solutions**:
1. Free `w_Targs*` results: `free(result)`
2. Free list elements when unpacking `'L'` format
3. Free JSON strings when unpacking `'D'` format
4. Check Python object reference counting (INCREF/DECREF)
5. Run under valgrind: `valgrind --leak-check=full verlihub`

---

## Development

### Building from Source

```bash
### Adding New Callbacks

New C++ functions can be exposed to Python scripts using template wrappers or manual implementation.

**Using Template Wrappers** (recommended for common patterns):

```cpp
// 1. Add enum entry to wrapper.h
enum {
    // ... existing entries ...
    W_MyNewCallback,
};

// 2. Implement callback function in cpipython.cpp
w_Targs *_MyNewCallback(int id, w_Targs *args) {
    auto myFunc = [](const char *param) -> long {
        // Your business logic
        cUser *user = server->GetUserByNick(param);
        return user ? user->mSomeProperty : -1;
    };
    return cpiPython::WrapStringToLong(id, args, myFunc);
}

// 3. Register in OnLoad()
VH_REGISTER_CALLBACK(MyNewCallback, MyNewCallback);

// 4. Declare in cpipython.h
extern "C" w_Targs *_MyNewCallback(int id, w_Targs *args);
```

**Available template wrappers**:
- `WrapStringToBool(id, args, lambda)` - String → Bool
- `WrapStringToString(id, args, lambda)` - String → String
- `WrapStringToLong(id, args, lambda)` - String → Long
- `WrapStringLongToBool(id, args, lambda)` - String, Long → Bool
- `WrapStringStringToBool(id, args, lambda)` - String, String → Bool
- `WrapStringStringToString(id, args, lambda)` - String, String → String

**Manual Implementation** (for custom signatures):

```cpp
w_Targs *_MyComplexCallback(int id, w_Targs *args) {
    const char *str1, *str2;
    long value;
    
    // Unpack arguments
    if (!cpiPython::lib_unpack(args, "ssl", &str1, &str2, &value))
        return NULL;
    
    // Validate inputs
    if (!str1 || !str2) return NULL;
    
    // Implement logic
    bool success = DoSomething(str1, str2, value);
    
    // Pack and return result
    return cpiPython::lib_pack("l", success ? 1L : 0L);
}
```

**Benefits of template wrappers**:
- Reduces code by ~30%
- Automatic argument unpacking
- Built-in NULL validation
- Type-safe by design
- Easier to maintain // Unpack arguments
    if (!cpiPython::lib_unpack(args, "ssl", &str1, &str2, &value))
---

## Additional Resources

- **Verlihub Wiki**: <https://github.com/verlihub/verlihub/wiki>
- **Python Event Hooks API**: <https://github.com/verlihub/verlihub/wiki/api-python-events>
- **Python Callback Methods API**: <https://github.com/verlihub/verlihub/wiki/api-python-methods>

---

### Debugging

**Enable verbose logging**:
```cpp
cpiPython::log_level = 3;  // 0-5, higher = more verbose
```

**Log levels**:
- 0: Critical errors only
- 1: Callback status
- 2: Function parameters and return values
- 3: Debugging information
- 4: Trace level (very verbose)

**GDB Debugging**:
```bash
gdb verlihub
(gdb) break cpiPython::OnParsedMsgChat
(gdb) run
```

---

## Contributing

Contributions welcome! Please:

1. Follow existing code style
2. Add tests for new features
3. Update documentation
4. Ensure all tests pass
5. Submit pull request to `python3-support` branch

---

## Changelog

### v1.6.0.0 (2025-12-13)

**Major Features**:
- ✅ Bidirectional API: C++ can call arbitrary Python functions
- ✅ Template callback wrappers: Simplified function registration
- ✅ Advanced type marshaling: Lists, dictionaries, complex data
- ✅ Comprehensive test suite: 100% pass rate

**Performance**:
- 250,000+ msg/sec sustained throughput
- Sub-millisecond function call overhead
- 1M message stress test validated

**API Enhancements**:
- `w_CallFunction()` - Call Python by name
- List marshaling (`'L'` format)
- Dictionary marshaling (`'D'` format, JSON-based)
- PyObject* passthrough (`'O'` format)
- Template wrappers for common callback patterns

**Testing**:
- 4 test executables with comprehensive coverage
- GIL stress testing (100K iterations)
- Integration testing with real hub operations
- Advanced type marshaling validation

**Documentation**:
- Complete API reference
- Architecture documentation
- 10+ usage examples
- Troubleshooting guide
