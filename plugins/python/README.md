# Verlihub Python Plugin

**Version**: 1.7.0.0  
**Python Version**: 3.12+  

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [vh Module API](#vh-module-api)
  - [Available Functions](#available-functions)
  - [Module Attributes](#module-attributes)
  - [Constants](#constants)
  - [Complete Example](#complete-example)
- [Dynamic Function Registration](#dynamic-function-registration)
  - [C++ Side: Registering Functions](#c-side-registering-functions)
  - [Python Side: Calling Functions](#python-side-calling-functions)
  - [Real-World Example](#real-world-example)
- [Writing Python Scripts](#writing-python-scripts)
  - [Event Hooks](#event-hooks)
  - [Script Structure](#script-structure)
- [Python Environment](#python-environment)
- [API Reference](#api-reference)
  - [Type Marshaling](#type-marshaling)
  - [Event Hooks Reference](#event-hooks-reference)
  - [Callback Functions Reference](#callback-functions-reference)
- [Architecture](#architecture)
  - [Bidirectional Communication](#bidirectional-communication)
  - [Sub-Interpreter Isolation](#sub-interpreter-isolation)
  - [Threading Model](#threading-model)
- [Testing](#testing)
- [Development](#development)
  - [Building from Source](#building-from-source)
  - [Adding New Callbacks](#adding-new-callbacks)
  - [Debugging](#debugging)
- [Troubleshooting](#troubleshooting)
- [License](#license)

---

## Overview

The Python plugin enables extensible scripting for Verlihub DC++ hub servers. It provides complete bidirectional communication between C++ and Python:

- **Event Hooks**: Python functions are called when hub events occur (user login, messages, searches, etc.)
- **vh Module API**: Python scripts can call 58 Verlihub C++ functions directly
- **Dynamic Registration**: C++ plugins can register custom functions callable from Python
- **C++ → Python Calls**: C++ can invoke arbitrary Python functions by name
- **Advanced Type Marshaling**: Exchange complex data (lists, dicts) between C++ and Python
- **Two Interpreter Modes**: Sub-interpreter isolation OR single-interpreter compatibility
- **GIL Management**: Thread-safe concurrent execution

### Interpreter Modes

The plugin supports two compilation modes with different trade-offs:

**Sub-Interpreter Mode** (default):
```bash
cmake ..
make
```
- Each script runs in isolated Python environment
- Scripts cannot interfere with each other
- Better for multi-user script development
- **Limitation**: Many modern packages don't work (FastAPI, numpy, torch, etc.)

**Single-Interpreter Mode** (recommended for production):
```bash
cmake -DPYTHON_USE_SINGLE_INTERPRETER=ON ..
make
```
- All scripts share one Python interpreter
- Full compatibility with modern Python ecosystem
- All threading and asyncio features work
- **Trade-off**: Scripts share global namespace (use proper variable naming!)

See [Python Environment](#python-environment) section for detailed comparison and package compatibility information.

---

## Quick Start

### 1. Load the Python Plugin

First, register and load the Python plugin using plugman:

```bash
# Register the plugin (one-time setup, replace path as needed)
!addplug python -p /usr/local/lib/libpython_pi.so -a 1

# Load the plugin
!onplug python
```

### 2. Load Python Scripts

Once the plugin is loaded, you can load Python scripts:

```bash
# Load a script
!pyload /path/to/script.py

# List loaded scripts
!pylist

# Reload a script
!pyreload script_name

# Unload a script
!pyunload script_name
```

### 3. Basic Python Script

```python
#!/usr/bin/env python3
"""
Simple Verlihub Python script demonstrating event hooks
"""
import vh

# Called when a user sends a chat message
def OnParsedMsgChat(nick, message):
    # Access Verlihub functions via vh module
    user_class = vh.GetUserClass(nick)
    if message.startswith("!stats"):
        count = vh.GetUsersCount()
        vh.pm(nick, f"Users online: {count}, Your class: {user_class}")
        return 0  # Block the command from being displayed
    return 1  # Allow normal messages

# Called when a user logs in
def OnUserLogin(nick):
    vh.SendToAll(f"Welcome {nick}!")
    return 1

# Called periodically (every second)
def OnTimer(msec):
    # Periodic tasks can go here
    return 1
```

### 3. Using the vh Module

```python
import vh

# Send messages
vh.SendToAll("Announcement to everyone!")
vh.SendToOpChat("Message to operators")
vh.pm("username", "Private message")

# User management
user_class = vh.GetUserClass("SomeUser")
vh.KickUser("OpNick", "BadUser", "Reason for kick")
vh.Ban("OpNick", "SpamUser", "Spamming", 3600, 1)  # 1 hour ban

# Get information
nick_list = vh.GetNickList()  # Returns Python list
op_list = vh.GetOpList()
user_count = vh.GetUsersCount()
hub_topic = vh.Topic()

# Configuration
max_users = vh.GetConfig("config", "max_users")
vh.SetConfig("config", "hub_topic", "Welcome to our hub!")
```

---

## vh Module API

The `vh` module provides 58 functions for interacting with Verlihub from Python scripts.

### Available Functions

#### Messaging (8 functions)

```python
vh.SendToOpChat(message, from_nick=None)
vh.SendDataToAll(data, min_class, max_class, delay=0)
vh.SendDataToUser(data, nick, delay=0)
vh.SendPMToAll(message, from_nick, min_class, max_class)
vh.usermc(nick, message)     # Send main chat to specific user
vh.pm(nick, message)          # Send private message
vh.mc(message)                # Send to main chat as bot
vh.classmc(message, min_class, max_class)  # Send to class range
```

#### User Information (9 functions)

```python
vh.GetUserClass(nick) → int   # Returns 0-10
vh.GetUserHost(nick) → str
vh.GetUserIP(nick) → str
vh.GetUserCC(nick) → str      # Country code
vh.GetIPCC(ip) → str
vh.GetIPCN(ip) → str          # Country name
vh.GetIPASN(ip) → str         # ASN info
vh.GetGeoIP(ip) → str
vh.GetMyINFO(nick) → str      # Full $MyINFO protocol string
```

#### User Lists (6 functions)

```python
# Modern Python list returns:
vh.GetNickList() → list[str]
vh.GetOpList() → list[str]
vh.GetBotList() → list[str]

# Raw protocol strings (for compatibility):
vh.GetRawNickList() → str     # "$NickList user1$$user2$$"
vh.GetRawOpList() → str
vh.GetRawBotList() → str
```

#### User Management (6 functions)

```python
vh.AddRegUser(nick, user_class, password="", op_note="")
vh.DelRegUser(nick)
vh.SetRegClass(nick, user_class)
vh.KickUser(op_nick, target_nick, reason, note="", op_note="", ban_time=0)
vh.Ban(op_nick, target_nick, reason, duration_sec, ban_type)
vh.CloseConnection(nick, reason_code=0, delay=0)
```

#### Configuration (2 functions)

```python
vh.GetConfig(config_type, variable) → str
vh.SetConfig(config_type, variable, value) → bool
```

#### Hub Management (6 functions)

```python
vh.Topic(new_topic=None) → str  # Get or set topic
vh.GetUsersCount() → int
vh.GetTotalShareSize() → str
vh.GetServFreq() → int
vh.name_and_version() → str
vh.StopHub(exit_code)
```

#### Bots (3 functions)

```python
vh.AddRobot(nick, user_class, desc, speed, email, share)
vh.DelRobot(nick)
vh.IsRobotNickBad(nick) → int
```

#### Advanced Functions (16 functions)

```python
vh.SetMyINFO(nick, desc="", tag="", speed="", email="", share=0)
vh.SetUserIP(nick, ip)
vh.SetMyINFOFlag(nick, flag)
vh.UnsetMyINFOFlag(nick, flag)
vh.UserRestrictions(nick, nochat="", nopm="", nosearch="", noctm="")
vh.ParseCommand(nick, command, is_pm)
vh.ScriptCommand(cmd, data, plugin)
vh.SQL(query_type, query)
vh.Encode(string) → str       # Encode DC++ special chars to HTML entities
vh.Decode(string) → str       # Decode HTML entities back to chars

# Dynamic function calls (see Dynamic Function Registration section)
vh.CallDynamicFunction(func_name, arg1, arg2, ...) → varies
```

### Module Attributes

Each loaded script has its own `vh` module instance with unique attributes:

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
vh.eCR_KICKED
vh.eCR_TIMEOUT
vh.eCR_BANNED
vh.eCR_TO_ANYACTION
vh.eCR_LOGIN_ERR
vh.eCR_SYNTAX
vh.eCR_INVALID_KEY
vh.eCR_RECONNECT
vh.eCR_QUIT
vh.eCR_FORCEMOVE
vh.eCR_HUB_LOAD
vh.eCR_PLUGIN
vh.eCR_SHARE_LIMIT

# Bot validation results
vh.eBOT_OK
vh.eBOT_EXISTS
vh.eBOT_BAD_CHARS

# Ban types
vh.eBF_NICKIP
vh.eBF_IP
vh.eBF_NICK
vh.eBF_RANGE
vh.eBF_HOST1
vh.eBF_HOST2
vh.eBF_HOST3
vh.eBF_HOSTR1
vh.eBF_PREFIX
vh.eBF_REVERSE
```

### Complete Example

```python
import vh

def OnParsedMsgChat(nick, message):
    """Handle chat messages with full hub interaction"""
    
    if message.startswith("!info"):
        # Get user information
        user_class = vh.GetUserClass(nick)
        user_ip = vh.GetUserIP(nick)
        user_cc = vh.GetUserCC(nick)
        
        # Get hub statistics
        user_count = vh.GetUsersCount()
        hub_topic = vh.Topic()
        
        # Send detailed response
        info = f"""User Info for {nick}:
Class: {user_class}
IP: {user_ip}
Country: {user_cc}

Hub Stats:
Online Users: {user_count}
Topic: {hub_topic}"""
        
        vh.pm(nick, info)
        return 0  # Block the command from appearing in chat
    
    elif message.startswith("!ops"):
        # Get operator list
        ops = vh.GetOpList()
        vh.pm(nick, f"Operators: {', '.join(ops)}")
        return 0
    
    elif message.startswith("!users"):
        # Get all users
        users = vh.GetNickList()
        vh.pm(nick, f"Total users ({len(users)}): {', '.join(users[:10])}...")
        return 0
    
    return 1  # Allow normal messages

def OnUserLogin(nick):
    """Welcome new users"""
    user_class = vh.GetUserClass(nick)
    
    if user_class >= 3:  # Operator or higher
        vh.SendToOpChat(f"{nick} (operator) has logged in")
    else:
        vh.SendToAll(f"Welcome {nick}!")
    
    return 1

def OnTimer(msec):
    """Periodic tasks"""
    # Example: Check user count every ~10 seconds
    if int(msec) % 10000 < 1000:
        count = vh.GetUsersCount()
        if count > 100:
            vh.SendToOpChat(f"High load: {count} users online")
    
    return 1
```

### Encoding/Decoding Utilities

The `vh.Encode()` and `vh.Decode()` functions handle DC++ protocol special characters:

```python
import vh

# Encode special characters for safe transmission
original = "Test$Message|With`Special~Chars"
encoded = vh.Encode(original)  
# Result: "Test&#36;Message&#124;With&#96;Special&#126;Chars"

# Decode back to original
decoded = vh.Decode(encoded)
assert decoded == original

# Character mappings:
# $ ↔ &#36;
# | ↔ &#124;
# ` ↔ &#96;
# ~ ↔ &#126;
# \x05 ↔ &#5;
```

---

## Dynamic Function Registration

Dynamic function registration allows C++ plugins to expose custom functions that Python scripts can call at runtime. This enables plugin-specific APIs without modifying the core vh module.

### C++ Side: Registering Functions

**Step 1**: Implement your C++ callback function:

```cpp
#include "wrapper.h"

// Example: Get user statistics
w_Targs* MyPlugin_GetUserStats(int callback_id, w_Targs* args) {
    char *username;
    if (!w_unpack(args, "s", &username)) return NULL;
    
    // Your plugin logic here
    long message_count = GetUserMessageCount(username);
    long login_count = GetUserLoginCount(username);
    
    // Return packed result
    return w_pack("ll", message_count, login_count);
}

// Example: Add two numbers
w_Targs* MyPlugin_Add(int callback_id, w_Targs* args) {
    long a, b;
    if (!w_unpack(args, "ll", &a, &b)) return NULL;
    return w_pack("l", a + b);
}

// Example: Reverse a string
w_Targs* MyPlugin_Reverse(int callback_id, w_Targs* args) {
    char *str;
    if (!w_unpack(args, "s", &str)) return NULL;
    
    // Reverse the string
    int len = strlen(str);
    char *reversed = (char*)malloc(len + 1);
    for (int i = 0; i < len; i++) {
        reversed[i] = str[len - 1 - i];
    }
    reversed[len] = '\0';
    
    w_Targs *result = w_pack("s", reversed);
    free(reversed);
    return result;
}

// Example: Process a list (reverse order using JSON)
w_Targs* MyPlugin_ReverseList(int callback_id, w_Targs* args) {
    char *json_str;
    if (!w_unpack(args, "D", &json_str)) return NULL;
    
#ifdef HAVE_RAPIDJSON
    // Parse JSON array
    nVerliHub::nPythonPlugin::JsonValue val;
    if (!nVerliHub::nPythonPlugin::parseJson(json_str, val) || 
        val.type != nVerliHub::nPythonPlugin::JsonType::ARRAY) {
        return w_pack("D", strdup("[]"));
    }
    
    // Reverse the array
    nVerliHub::nPythonPlugin::JsonValue reversed;
    reversed.type = nVerliHub::nPythonPlugin::JsonType::ARRAY;
    for (auto it = val.array_val.rbegin(); it != val.array_val.rend(); ++it) {
        reversed.array_val.push_back(*it);
    }
    
    // Serialize back to JSON
    std::string result_json = nVerliHub::nPythonPlugin::toJsonString(reversed);
    return w_pack("D", strdup(result_json.c_str()));
#else
    return w_pack("D", strdup("[]"));
#endif
}

// Example: Process a dictionary (add field)
w_Targs* MyPlugin_AddField(int callback_id, w_Targs* args) {
    char *dict_json;
    if (!w_unpack(args, "D", &dict_json)) return NULL;
    
    // Parse JSON, add field, serialize back
    // (simplified - in production use a proper JSON library)
    char *modified = (char*)malloc(strlen(dict_json) + 100);
    sprintf(modified, "{\"processed\":true,%s", dict_json + 1);  // Insert field
    
    w_Targs *result = w_pack("D", modified);
    free(modified);
    return result;
}
```

**Step 2**: Register functions when scripts load:

```cpp
// When your plugin initializes or a Python script loads
int script_id = 0;  // Obtain from the Python plugin

// Register your custom functions
w_RegisterFunction(script_id, "get_stats", MyPlugin_GetUserStats);
w_RegisterFunction(script_id, "add", MyPlugin_Add);
w_RegisterFunction(script_id, "reverse", MyPlugin_Reverse);
w_RegisterFunction(script_id, "reverse_list", MyPlugin_ReverseList);
w_RegisterFunction(script_id, "add_field", MyPlugin_AddField);
```

**Step 3**: Optionally unregister when done:

```cpp
w_UnregisterFunction(script_id, "get_stats");
```

### Python Side: Calling Functions

Once registered, call dynamic functions using `vh.CallDynamicFunction()`:

```python
import vh

def OnUserLogin(nick):
    # Call dynamically registered C++ function
    try:
        result = vh.CallDynamicFunction('get_stats', nick)
        if result:
            msg_count, login_count = result
            vh.pm(nick, f"Your stats: {msg_count} messages, {login_count} logins")
    except Exception as e:
        print(f"Error calling get_stats: {e}")
    
def some_function():
    # Call simple math function
    result = vh.CallDynamicFunction('add', 10, 20)
    print(f"10 + 20 = {result}")  # Output: 30
    
    # Call string function
    reversed_text = vh.CallDynamicFunction('reverse', "Hello")
    print(reversed_text)  # Output: "olleH"
    
    # Call with list argument
    fruits = ['apple', 'banana', 'cherry']
    reversed_fruits = vh.CallDynamicFunction('reverse_list', fruits)
    print(reversed_fruits)  # Output: ['cherry', 'banana', 'apple']
    
    # Call with dictionary argument
    user_data = {'name': 'Alice', 'age': 30}
    processed_data = vh.CallDynamicFunction('add_field', user_data)
    print(processed_data)  # Output: {'processed': True, 'name': 'Alice', 'age': 30}
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

**Supported Types:**
- Arguments: `long` (int), `str`, `double` (float), `list`, `tuple`, `set`, `dict`
- Returns: `long`, `str`, `double`, `list`, `tuple`, `set`, `dict`, `None`, or tuple of these types
- All container types (list/tuple/set/dict) are marshaled as JSON

**Type Conversion Details:**

| Python Type | C++ Type | Notes |
|-------------|----------|-------|
| `int` | `long` | Standard integer conversion |
| `str` | `char*` | UTF-8 strings, automatically copied |
| `float` | `double` | Double-precision floating point |
| `list` | `char*` (JSON) | Serialized as JSON array |
| `tuple` | `char*` (JSON) | Serialized as JSON array (order preserved) |
| `set` | `char*` (JSON) | Serialized as JSON array (order not guaranteed) |
| `dict` | `char*` (JSON) | Serialized as JSON object |

**Complex Type Examples:**

```python
# List handling
items = ['item1', 'item2', 'item3']
result = vh.CallDynamicFunction('process_list', items)
# C++ receives JSON: '["item1","item2","item3"]'

# Dictionary handling
data = {'key1': 'value1', 'key2': 42, 'nested': {'inner': True}}
result = vh.CallDynamicFunction('process_dict', data)
# C++ receives JSON: '{"key1":"value1","key2":42,"nested":{"inner":true}}'

# Set handling
tags = {'alpha', 'beta', 'gamma'}
result = vh.CallDynamicFunction('process_tags', tags)
# C++ receives JSON: '["alpha","beta","gamma"]' (order may vary)

# Tuple handling
coords = (10.5, 20.3, 30.1)
result = vh.CallDynamicFunction('calculate', coords)
# C++ receives JSON: '[10.5,20.3,30.1]'

# Mixed types
result = vh.CallDynamicFunction('complex_func', 'username', 42, ['a', 'b'], {'status': 'active'})
# C++ receives all types as JSON where applicable
```

### Real-World Example

```python
import vh

def OnUserCommand(nick, command):
    """Handle custom commands powered by C++ plugin"""
    
    if command.startswith("!stats"):
        # Call C++ analytics plugin
        stats = vh.CallDynamicFunction('get_detailed_stats', nick)
        if stats:
            messages, searches, downloads = stats
            vh.pm(nick, f"Your activity: {messages} msgs, {searches} searches, {downloads} downloads")
        return 0
    
    elif command.startswith("!calc"):
        # Use C++ math library
        try:
            _, expression = command.split(" ", 1)
            result = vh.CallDynamicFunction('evaluate_expression', expression)
            vh.pm(nick, f"Result: {result}")
        except:
            vh.pm(nick, "Usage: !calc <expression>")
        return 0
    
    elif command.startswith("!check"):
        # Validate using external C++ library
        _, data = command.split(" ", 1)
        is_valid = vh.CallDynamicFunction('validate_data', data)
        vh.pm(nick, "Valid!" if is_valid else "Invalid data")
        return 0
    
    return 1

def OnTimer(msec):
    """Periodic analytics"""
    if int(msec) % 60000 < 1000:  # Every minute
        # Call C++ plugin for intensive computation
        report = vh.CallDynamicFunction('generate_hourly_report')
        if report:
            vh.SendToOpChat(f"Hourly report: {report}")
    
    return 1
```

---

## Writing Python Scripts

### Event Hooks

Python scripts can define hook functions that are called when specific events occur in the hub. All hooks are optional.

**Available Hooks:**

```python
# Connection Events
def OnNewConn(ip): pass
def OnCloseConn(ip): pass
def OnCloseConnEx(ip, reason_code, nick): pass

# User Events
def OnUserLogin(nick): pass
def OnUserLogout(nick): pass
def OnUserInList(nick): pass

# Message Events
def OnParsedMsgChat(nick, message): pass
def OnParsedMsgPM(nick, message, target): pass
def OnParsedMsgSearch(nick, search_query): pass
def OnParsedMsgSR(nick, search_result): pass
def OnParsedMsgMyINFO(nick, desc, tag, speed, email, share): pass
def OnFirstMyINFO(nick, desc, tag, speed, email, share): pass

# Operator Events
def OnOperatorCommand(nick, command): pass
def OnOperatorKicks(op_nick, nick, reason): pass
def OnOperatorDrops(op_nick, nick): pass

# Hub Events
def OnTimer(milliseconds): pass
def OnNewReg(nick, user_class, op_nick): pass
def OnNewBan(ip, nick, op_nick, reason): pass
def OnSetConfig(config, variable, value): pass

# And many more (see Event Hooks Reference below)
```

**Hook Return Values:**
- `1` or `True`: Allow the action/event to proceed
- `0` or `False`: Block the action/event
- `None`: Equivalent to returning `1`

### Script Structure

```python
#!/usr/bin/env python3
"""
Script description and purpose
"""
import vh  # Always import the vh module

# Optional: Define script metadata
def name_and_version():
    return "MyScript", "1.0"

# Define your hook functions
def OnUserLogin(nick):
    vh.SendToAll(f"{nick} joined!")
    return 1

def OnParsedMsgChat(nick, message):
    if "badword" in message.lower():
        vh.pm(nick, "Please watch your language")
        return 0  # Block the message
    return 1

def OnTimer(msec):
    # Periodic tasks
    return 1

# Scripts can also define helper functions
def is_operator(nick):
    return vh.GetUserClass(nick) >= 3

def send_stats_report():
    count = vh.GetUsersCount()
    vh.SendToOpChat(f"Users online: {count}")
```

---

## Python Environment

### Interpreter Mode Selection

The Python plugin can be compiled in two different modes, each with distinct characteristics:

#### Sub-Interpreter Mode (Default)

**Compilation:**
```bash
cmake ..
make
```

**Characteristics:**
- ✅ Each script runs in isolated Python environment
- ✅ Scripts have separate global namespaces
- ✅ Memory leaks in one script don't affect others
- ✅ Scripts can be reloaded independently
- ✅ Better for development/testing environments
- ❌ **Incompatible with many modern Python packages**
- ❌ Threading module has limitations
- ❌ Some asyncio features may not work

**Compatible Packages:**
- Pure Python packages (no C extensions)
- requests, beautifulsoup4, lxml
- Standard library modules
- Most text processing libraries

**Incompatible Packages:**
- FastAPI, Pydantic, uvicorn (PyO3/Rust extensions)
- numpy, pandas, scipy (global C state)
- torch, tensorflow (static initialization)
- cryptography, tokenizers (PyO3)
- Many ML/data science packages

#### Single-Interpreter Mode (Recommended for Production)

**Compilation:**
```bash
cmake -DPYTHON_USE_SINGLE_INTERPRETER=ON ..
make
```

**Characteristics:**
- ✅ **Full compatibility with modern Python packages**
- ✅ Threading and asyncio work perfectly
- ✅ Better performance (no interpreter switching)
- ✅ All C extension packages work
- ⚠️ Scripts share global namespace (use proper naming!)
- ⚠️ Global variables visible across scripts
- ⚠️ Memory leaks affect all scripts

**All Packages Work:**
- FastAPI, Pydantic, uvicorn (web frameworks)
- numpy, pandas, scipy (data science)
- torch, tensorflow (machine learning)
- asyncio, threading, concurrent.futures
- Any PyPI package with C extensions

**Best Practices for Single-Interpreter Mode:**
```python
# Use unique prefixes for globals to avoid conflicts
MYSCRIPT_config = {...}
MYSCRIPT_cache = {}

# Or use a namespace dict
MYSCRIPT = {
    'config': {...},
    'cache': {},
    'state': {}
}

# Avoid bare global names that might conflict
# BAD:  config = {}
# GOOD: myapp_config = {}
```

### Package Compatibility Reference

**Why Incompatibility Occurs:**

Python's sub-interpreter mode requires that all C extensions support interpreter isolation. Many modern packages use:

1. **PyO3 (Rust ↔ Python bindings)**:
   - Explicitly doesn't support sub-interpreters
   - Used by: FastAPI/Pydantic ecosystem, cryptography, polars
   - Error: "PyO3 modules do not yet support subinterpreters"

2. **Global C State**:
   - Static variables, global caches, singleton patterns
   - Used by: numpy, pandas, scipy, ML frameworks
   - Error: "Interpreter change detected" or segfaults

3. **Module Import Caching**:
   - C-level module registration that's global
   - Used by: asyncio, threading internals
   - Error: ImportError or threading crashes

**Compatibility Matrix:**

| Package Category | Sub-Interpreter | Single-Interpreter | Notes |
|------------------|-----------------|-------------------|-------|
| Pure Python | ✅ Works | ✅ Works | Always compatible |
| Standard Library | ✅ Works | ✅ Works | Built-in support |
| requests, urllib3 | ✅ Works | ✅ Works | Pure Python HTTP |
| FastAPI, Pydantic | ❌ **FAILS** | ✅ Works | PyO3/Rust extensions |
| numpy, pandas | ❌ **FAILS** | ✅ Works | Global C state |
| torch, tensorflow | ❌ **FAILS** | ✅ Works | Static initialization |
| asyncio (advanced) | ⚠️ Limited | ✅ Works | Event loop issues |
| threading (heavy) | ⚠️ Limited | ✅ Works | State corruption |

**Real-World Example: hub_api.py**

This script demonstrates why single-interpreter mode is necessary:

```python
# Requires these packages:
from fastapi import FastAPI          # PyO3 - fails in sub-interpreter
from pydantic import BaseModel       # PyO3 - fails in sub-interpreter  
import uvicorn                       # Uses PyO3 deps

# Uses threading for background server
import threading                     # Works better in single-interpreter
import asyncio                       # Event loop isolation issues in sub-interpreter

# All of the above require: -DPYTHON_USE_SINGLE_INTERPRETER=ON
```

**Testing Your Package:**

To check if a package will work in sub-interpreter mode:

```python
import sys
import importlib

def test_subinterpreter_compat(module_name):
    """Test if module supports sub-interpreters"""
    try:
        # Try importing in main interpreter
        mod = importlib.import_module(module_name)
        
        # Check for known incompatible markers
        if hasattr(mod, '__file__') and mod.__file__:
            if '.so' in mod.__file__ or '.pyd' in mod.__file__:
                print(f"⚠️ {module_name} is a C extension - may not work")
        
        # Check for PyO3
        if 'pyo3' in str(type(mod)).lower():
            print(f"❌ {module_name} uses PyO3 - will NOT work")
            return False
            
        print(f"✅ {module_name} may work in sub-interpreter mode")
        return True
    except Exception as e:
        print(f"❌ {module_name} failed: {e}")
        return False

# Test your dependencies
test_subinterpreter_compat('fastapi')   # Will fail
test_subinterpreter_compat('requests')  # Will pass
```

### Standard Library

All Python 3.12+ standard library modules are available in both modes:

```python
import datetime
import json
import re
import urllib.request
import sqlite3
import threading      # Works better in single-interpreter mode
import asyncio        # Works better in single-interpreter mode
# ... any standard library module
```

### Virtual Environments

You can use Python virtual environments to install script dependencies without polluting the system-wide Python installation.

**Create a virtual environment:**

```bash
# Create a venv in your scripts directory
cd /path/to/verlihub/scripts
python3 -m venv venv

# Activate the virtual environment
source venv/bin/activate

# Install packages
pip install requests beautifulsoup4 sqlalchemy

# Deactivate when done
deactivate
```

**Use packages from the venv in your scripts:**

```python
#!/usr/bin/env python3
import sys
import os

# Add venv site-packages to the import path
script_dir = os.path.dirname(os.path.abspath(__file__))
venv_path = os.path.join(script_dir, 'venv', 'lib', 'python3.12', 'site-packages')
sys.path.insert(0, venv_path)

# Now you can import packages from the venv
import requests
import vh

def OnUserLogin(nick):
    """Example: Fetch user stats from external API"""
    try:
        response = requests.get(f'https://api.example.com/user/{nick}')
        if response.status_code == 200:
            data = response.json()
            vh.pm(nick, f"Welcome back! Your rank: {data['rank']}")
    except Exception as e:
        print(f"API error: {e}")
    
    return 1
```

**Note:** Adjust the Python version in the venv path (`python3.12`) to match your system's Python version.

### Module Import Path

The script's directory is automatically added to `sys.path`, so you can import local modules:

```
/path/to/scripts/
  ├── main_script.py     # Your hub script
  ├── venv/              # Virtual environment (optional)
  └── utils/
      ├── __init__.py
      └── helpers.py     # import utils.helpers works!
```

---

## API Reference

### Type Marshaling

The plugin uses **RapidJSON** as a high-performance marshaling layer to exchange data between C++ and Python. This enables passing arbitrary complex nested structures while maintaining type safety and excellent performance.

#### Supported Types

| Format | C++ Type | Python Type | Notes |
|--------|----------|-------------|-------|
| `'l'` | `long` | `int` | Integers, IDs, counts, flags (64-bit on modern systems) |
| `'s'` | `char*` | `str` | UTF-8 strings, nicknames, messages |
| `'d'` | `double` | `float` | Decimals, timestamps (64-bit precision) |
| `'p'` | `void*` | `int` (capsule) | Opaque pointers (advanced usage) |
| `'D'` | `char*` (JSON) | `dict\|list\|set\|tuple` | All container types via JSON |
| `'O'` | `PyObject*` | `object` | Raw Python objects (C++ passthrough) |

#### JSON Marshaling (Format 'D')

Complex data structures (dicts, lists, sets, tuples, nested data) are automatically serialized to JSON and deserialized on the receiving end. This provides:

- **Arbitrary Nesting**: Lists of dicts, dicts of lists, sets of tuples, deeply nested structures
- **Mixed Types**: Combine strings, integers, floats, booleans, null values
- **Type Preservation**: Python `int` → int64_t, `float` → double (64-bit)
- **Set Support**: Python `set` and `frozenset` automatically marshaled
- **Tuple Support**: Python `tuple` preserves immutability semantics
- **High Performance**: RapidJSON parsing is ~10x faster than Python's json module

**Automatic JSON Conversion:**

```python
# Python side - arbitrary nested structures work seamlessly
result = vh.CallDynamicFunction('process_data',
    'user123',  # String (format 's')
    42,         # Integer (format 'l')
    {           # Dict → JSON (format 'D')
        'settings': {
            'theme': 'dark',
            'notifications': True,
            'limits': [10, 100, 1000]
        },
        'stats': {
            'messages': 1234,
            'uptime': 56789.12,
            'flags': [1, 2, 3, 5, 8]
        }
    },
    [           # Complex list → JSON (format 'D')
        {'name': 'Alice', 'score': 95.5},
        {'name': 'Bob', 'score': 87.0},
        {'name': 'Charlie', 'score': 92.3}
    ]
)

# C++ side automatically receives JSON-marshaled data
w_Targs* ProcessData(int id, w_Targs* args) {
    char *username;
    long count;
    char *settings_json;  // Auto-marshaled from Python dict
    char *users_json;     // Auto-marshaled from Python list
    
    w_unpack(args, "slDD", &username, &count, &settings_json, &users_json);
    
    // Can parse JSON in C++ using RapidJSON or return to Python
    return w_pack("D", strdup("{\"status\": \"processed\"}"));
}
```

#### Performance Characteristics

**Integration Test Results** (StressTreatMsg: 1 million message exchanges):

```
Test Duration: ~30 seconds (1M messages)
Memory Growth: 344 KB total
Per-Message:   ~0.34 bytes (negligible leak rate)
Throughput:    ~33,000 messages/second
JSON Parse:    <1μs per message (RapidJSON)
```

**Threading Stress Test Results** (5 worker threads processing 1000 events):

```
Memory Footprint:
  Initial VmRSS: ~17 MB
  Peak VmRSS:    ~21 MB (↑ 4 MB with threads active)
  Final VmRSS:   ~21 MB (stable after threads exit)
  Initial VmData: ~37 MB  
  Peak VmData:    ~82 MB (↑ 45 MB thread stack/heap)
  Final VmData:   ~82 MB (stable after threads exit)

Threading Performance:
  Throughput:    1000 events processed successfully
  Thread Safety: No deadlocks, race conditions, or crashes
  GIL Overhead:  Minimal (Python threads can process queued data)
  Memory Leak:   None detected after thread cleanup
```

**Known Threading Limitation:**
Python's sub-interpreter model has a fundamental incompatibility with the threading module. Even after threads fully exit and join, calling `Py_EndInterpreter()` or `Py_Finalize()` leaves corrupted thread state that causes crashes (see [Python issue #15751](https://bugs.python.org/issue15751)). The plugin implements a workaround: when threading/asyncio modules are detected in `sys.modules`, interpreter cleanup is skipped to prevent crashes. This causes a small memory leak (~4-5 MB per script using threads), but prevents application-wide crashes. For long-running production hubs, avoid heavy threading use in Python scripts or run scripts externally and use IPC.

The JSON marshaling layer is optimized for hub plugin workloads where message volume is typically <1000/sec, making the overhead negligible while providing maximum flexibility for all container types (lists, dicts, sets, tuples).

**Type Mapping Details:**

```python
# Python → JSON → C++ type preservation
42              → int64_t (64-bit signed)
3.14159         → double (64-bit IEEE 754)
"hello"         → std::string (UTF-8)
True/False      → bool
None            → null
[1, 2, 3]       → JSON array (Python list)
(1, 2, 3)       → JSON array (Python tuple)
{1, 2, 3}       → JSON array (Python set - order not preserved)
{'a': 1}        → JSON object (Python dict)

# All JSON types round-trip correctly through C++:
Python dict/list/set/tuple → JSON string → C++ parse → JSON string → Python dict/list
```

**Sets and Tuples:**

```python
# Python sets are automatically marshaled
my_set = {1, 2, 3, 4, 5}
result = vh.CallDynamicFunction('process_set', my_set)
# → C++ receives JSON array: [1,2,3,4,5] (order may vary)

# Frozensets also supported
frozen = frozenset(['apple', 'banana', 'cherry'])
result = vh.CallDynamicFunction('check_fruits', frozen)

# Python tuples preserve order (serialized as JSON arrays)
coordinates = (10.5, 20.3, 30.1)
result = vh.CallDynamicFunction('calculate_distance', coordinates)
# → C++ receives JSON array: [10.5, 20.3, 30.1]

# Nested tuples work
nested = (1, (2, 3), (4, (5, 6)))
result = vh.CallDynamicFunction('process_tree', nested)

# Mixed container types in single call
result = vh.CallDynamicFunction('analyze',
    ['user1', 'user2'],          # list
    ('alpha', 'beta'),            # tuple  
    {'admin', 'moderator'},       # set
    {'status': 'active'}          # dict
)
```

**Important Notes:**
- **Sets** serialize as JSON arrays and lose ordering (sets are unordered by definition)
- **Tuples** serialize as JSON arrays but semantically represent immutable sequences
- When Python receives JSON arrays from C++, they become lists by default
- Use Python's `tuple()` or `set()` constructors if you need to convert after receiving
- All container types can be arbitrarily nested: sets of tuples, lists of dicts, etc.

#### C++ JSON Marshaling Utilities

For C++ plugin developers working with complex nested data, the `json_marshal.h` header provides utilities for serializing and deserializing JSON without directly using RapidJSON.

**API Functions:**

```cpp
#include "json_marshal.h"

// Core JSON parsing and serialization
bool parseJson(const std::string& json_str, JsonValue& out_value);
std::string toJsonString(const JsonValue& value, bool pretty = false);

// Helper utilities for common types
std::string stringListToJson(const std::vector<std::string>& string_list);
std::vector<std::string> jsonToStringList(const std::string& json_str);
std::string stringMapToJson(const std::map<std::string, std::string>& map);
bool jsonToStringMap(const std::string& json_str, std::map<std::string, std::string>& out_map);
```

**Note**: All functions use modern C++ types (`std::string`, `std::vector`, `std::map`). When calling from C code with `char*` pointers, they are automatically converted to `std::string` (temporary objects are created). No manual memory management required for return values.

**JsonValue Structure:**

```cpp
#include "json_marshal.h"

using namespace nVerliHub::nPythonPlugin;

// Create JSON values
JsonValue null_val;  // Default constructor = null
JsonValue bool_val(true);
JsonValue int_val((int64_t)42);
JsonValue float_val(3.14159);
JsonValue str_val("hello");

// Arrays
JsonValue array;
array.type = JsonType::ARRAY;
array.array_val.push_back(JsonValue((int64_t)1));
array.array_val.push_back(JsonValue((int64_t)2));
array.array_val.push_back(JsonValue((int64_t)3));

// Objects (maps)
JsonValue object;
object.type = JsonType::OBJECT;
object.object_val["name"] = JsonValue("Alice");
object.object_val["age"] = JsonValue((int64_t)30);
object.object_val["score"] = JsonValue(95.5);

// Nested structures
JsonValue nested_obj;
nested_obj.type = JsonType::OBJECT;
nested_obj.object_val["users"] = array;
nested_obj.object_val["metadata"] = object;
```

**Parsing JSON Strings:**

```cpp
#include "json_marshal.h"

// Parse JSON from Python
w_Targs* MyCallback(int id, w_Targs* args) {
    char *json_str;
    if (!w_unpack(args, "D", &json_str)) return NULL;
    
    // Parse into JsonValue structure
    JsonValue value;
    if (!parseJson(json_str, value)) {
        // Parse failed
        return w_pack("s", "Error: Invalid JSON");
    }
    
    // Access parsed data
    if (value.isObject()) {
        if (value.object_val.count("name")) {
            std::string name = value.object_val["name"].string_val;
            // Process name...
        }
        
        if (value.object_val.count("scores") && 
            value.object_val["scores"].isArray()) {
            for (const auto& score : value.object_val["scores"].array_val) {
                if (score.isDouble()) {
                    double val = score.double_val;
                    // Process score...
                }
            }
        }
    }
    
    return w_pack("s", "Success");
}
```

**Serializing to JSON:**

```cpp
#include "json_marshal.h"

w_Targs* GetUserStats(int id, w_Targs* args) {
    char *username;
    if (!w_unpack(args, "s", &username)) return NULL;
    
    // Build response object
    JsonValue response;
    response.type = JsonType::OBJECT;
    response.object_val["username"] = JsonValue(username);
    response.object_val["message_count"] = JsonValue((int64_t)1234);
    response.object_val["online_time"] = JsonValue(56789.12);
    
    // Add array of recent actions
    JsonValue actions;
    actions.type = JsonType::ARRAY;
    actions.array_val.push_back(JsonValue("login"));
    actions.array_val.push_back(JsonValue("search"));
    actions.array_val.push_back(JsonValue("download"));
    response.object_val["recent_actions"] = actions;
    
    // Serialize to JSON string
    std::string json_str = toJsonString(response);
    
    // Return to Python (Python will deserialize back to dict)
    return w_pack("D", strdup(json_str.c_str()));
}
```

**Helper Functions:**

```cpp
// Convert string list to JSON array
std::vector<std::string> colors = {"apple", "banana", "cherry"};
std::string json = stringListToJson(colors);
// Result: '["apple","banana","cherry"]'

// Parse JSON array to string list
auto list = jsonToStringList("[\"red\",\"green\",\"blue\"]");
// list[0] = "red", list[1] = "green", list[2] = "blue"
// No manual memory management needed!

// Convert map to JSON object
std::map<std::string, std::string> config;
config["theme"] = "dark";
config["language"] = "en";
std::string json = stringMapToJson(config);
// Result: '{"theme":"dark","language":"en"}'

// Parse JSON object to map
std::map<std::string, std::string> result;
bool ok = jsonToStringMap("{\"key\":\"value\"}", result);
```

**Type Checking:**

```cpp
JsonValue value;
if (parseJson(json_str, value)) {
    if (value.isNull())   { /* handle null */ }
    if (value.isBool())   { bool b = value.bool_val; }
    if (value.isInt())    { int64_t i = value.int_val; }
    if (value.isDouble()) { double d = value.double_val; }
    if (value.isString()) { std::string s = value.string_val; }
    if (value.isArray())  { /* iterate value.array_val */ }
    if (value.isObject()) { /* iterate value.object_val */ }
    if (value.isSet())    { /* handle set */ }
    if (value.isTuple())  { /* handle tuple */ }
}
```

**Complete Example - Processing Complex Data:**

```cpp
w_Targs* ProcessUserData(int id, w_Targs* args) {
    char *user_json;
    if (!w_unpack(args, "D", &user_json)) return NULL;
    
    JsonValue user;
    if (!parseJson(user_json, user) || !user.isObject()) {
        return w_pack("s", "Invalid user data");
    }
    
    // Extract user information
    std::string name = "Unknown";
    if (user.object_val.count("name") && user.object_val["name"].isString()) {
        name = user.object_val["name"].string_val;
    }
    
    // Process nested preferences
    if (user.object_val.count("preferences") && 
        user.object_val["preferences"].isObject()) {
        JsonValue& prefs = user.object_val["preferences"];
        
        if (prefs.object_val.count("notifications") && 
            prefs.object_val["notifications"].isBool()) {
            bool notify = prefs.object_val["notifications"].bool_val;
            // Handle notification preference...
        }
    }
    
    // Process array of tags
    if (user.object_val.count("tags") && user.object_val["tags"].isArray()) {
        for (const auto& tag : user.object_val["tags"].array_val) {
            if (tag.isString()) {
                std::string tag_name = tag.string_val;
                // Process tag...
            }
        }
    }
    
    // Build and return response
    JsonValue response;
    response.type = JsonType::OBJECT;
    response.object_val["status"] = JsonValue("processed");
    response.object_val["user"] = JsonValue(name);
    response.object_val["timestamp"] = JsonValue((int64_t)time(NULL));
    
    std::string response_json = toJsonString(response);
    return w_pack("D", strdup(response_json.c_str()));
}
```

**Benefits:**
- **No RapidJSON knowledge required** - Simple C++ API
- **Type-safe** - Explicit type checking with `isInt()`, `isString()`, etc.
- **64-bit integers** - Full Python int range support (`int64_t`)
- **UTF-8 strings** - Proper Unicode handling
- **Arbitrary nesting** - Objects in arrays, arrays in objects, etc.
- **Automatic conversion** - Python dicts/lists ↔ JSON ↔ C++ structures

### Event Hooks Reference

Python scripts react to hub events by defining hook functions. Return `0` or `False` to block an event, `1`, `True`, or `None` to allow it to proceed.

**Note**: All Python scripts receive events in the order they appear in `!pylist`. Returning `0` only blocks further processing by the hub and other plugins, not other Python scripts.

#### Special Events

##### `OnTimer(milliseconds=0)`
Called approximately every second (configurable via `timer_serv_period`). The `milliseconds` argument is optional for backwards compatibility.

**Example - Periodic tasks:**
```python
import vh, time

last_check = 0
def OnTimer(msec=0):
    global last_check
    now = time.time()
    
    # Run every 60 seconds
    if now - last_check >= 60:
        count = vh.GetUsersCount()
        if count > 100:
            vh.SendToOpChat(f"High load: {count} users")
        last_check = now
    
    return 1
```

##### `UnLoad()`
Called before script removal (`!pyunload`, `!pyreload`, hub shutdown). Use for cleanup: close sockets, save config, remove bots.

##### `OnUnLoad(error_code)`
Called when hub is shutting down. `error_code` is normally `0`, except on SEGV signal. Perform cleanup only when `error_code == 0`.

##### `OnScriptCommand(command, data, plugin, script)`
Generated when any script calls `vh.ScriptCommand()`. All scripts receive it (including caller), so check `script` against `vh.path`. Return value ignored.

**Example - Inter-script communication:**
```python
import vh

blacklist = set()

def OnScriptCommand(cmd, data, plug, script):
    if cmd == "add_to_blacklist" and script != vh.path:
        blacklist.add(data)
        vh.SendToOpChat(f"Blacklist updated by {script}")
```

##### `OnScriptQuery(command, data, plugin, script)`
Like `OnScriptCommand` but you can return a string back to the caller.

##### `OnSetConfig(nick, conf, var, val_new, val_old, val_type)`
Triggered when configuration variable is about to change. Return `0` to block. `val_type` constants: `vh.eIT_BOOL`, `vh.eIT_INT`, `vh.eIT_STRING`, etc.

#### Connection Events

##### `OnNewConn(ip)`
First stage of client connection. Return `0` to disconnect.

##### `OnCloseConn(ip)`
Connection closed. Cannot be blocked.

##### `OnCloseConnEx(ip, close_reason, nick)`
Extended version with close reason and nick (empty if closed before login). Reasons: `vh.eCR_KICKED`, `vh.eCR_TIMEOUT`, `vh.eCR_PASSWORD`, `vh.eCR_BANNED`, etc.

**Example - Log failed logins:**
```python
def OnCloseConnEx(ip, reason, nick):
    if reason == vh.eCR_PASSWORD:
        vh.SendToOpChat(f"Failed login as {nick} from {ip}")
```

#### User Events

##### `OnUserLogin(nick)`
User successfully validated and received hub info. Return `0` to disconnect.

##### `OnUserLogout(nick)`
User disconnected. Cannot be blocked.

##### `OnUserInList(nick)`
User added to user list.

##### `OnNewReg(op, nick, uclass)`
Operator `op` registering user `nick` with class `uclass`. Return `0` to abort.

##### `OnValidateTag(nick, data)`
Check user's client tag. Return `0` to reject: `data` like `"<++ V:0.761,M:A,H:16/0/1,S:17>"`.

##### `OnParsedMsgValidateNick(data)`
Client verifying nick availability. Return `0` to reject: `data` like `"$ValidateNick Mario"`.

#### Commands

##### `OnUserCommand(nick, command)`
User command received. Check `command[1:]` (skip trigger char) not full command. Return `0` if handled.

##### `OnOperatorCommand(nick, command)`
Operator (class ≥3) command. Works like `OnUserCommand`.

##### `OnHubCommand(nick, clean_command, user_class, in_pm, prefix)`
Unified command handler (called after above two). Benefits:
- `clean_command` has trigger char removed
- `user_class` for permission checking
- `in_pm` tells you context (0=main chat, 1=PM)
- `prefix` is the trigger char used

**Example:**
```python
def OnHubCommand(nick, command, uclass, in_pm, prefix):
    write = vh.pm if in_pm else vh.usermc
    
    if command == "myip":
        write(f"Your IP: {vh.GetUserIP(nick)}", nick)
        return 0
    
    if command == "stats" and uclass >= 3:
        write(f"Users: {vh.GetUsersCount()}", nick)
        return 0
```

#### Chat Messages

##### `OnParsedMsgChat(nick, msg)`
Main chat message. Return `0` to block, or `(nick, message)` tuple to substitute.

**Example - Filter and transform:**
```python
import re

banned_words = re.compile(r'\b(badword1|badword2)\b', re.I)

def OnParsedMsgChat(nick, msg):
    if banned_words.search(msg):
        vh.usermc("Message blocked", nick)
        return 0  # Block
    
    # Substitute message for specific users
    if nick in leet_users:
        return nick, make_leet(msg)
    
    return 1  # Allow
```

##### `OnParsedMsgPM(nick, msg, to_nick)`
Private message from `nick` to `to_nick`. Return `0` to block.

##### `OnParsedMsgMCTo(nick, msg, to_nick)`
Private main chat message.

##### `OnOpChatMessage(nick, msg)`
Operator chat message. Return `0` to block.

#### Kicks & Bans

##### `OnOperatorKicks(op, nick, reason)`
Operator `op` kicking `nick`. Return `0` to prevent.

##### `OnOperatorDrops(op, nick)`
Operator dropping user (no reason). Return `0` to prevent.

##### `OnOperatorDropsWithReason(op, nick, reason)`
Operator dropping with reason. Prefer this over `OnOperatorDrops` (both are called for compatibility).

##### `OnNewBan(op, ip, nick, reason)`
Operator banning user. Return `0` to prevent.

#### Protocol Messages

##### `OnParsedMsgSearch(nick, data)`
User searching. `data` like `"$Search ip:port F?T?0?9?TTH:hash"`. Return `0` to block.

##### `OnParsedMsgSR(nick, data)`
Search result to passive user. `data` like `"$SR nick path_bytes (ip:port)target"`.

##### `OnParsedMsgMyINFO(nick, desc, tag, speed, mail, sharesize)`
User updating MyINFO. Return `0` to block, or `(desc, tag, speed, mail, sharesize)` tuple to substitute (use `None` for unchanged fields).

##### `OnFirstMyINFO(nick, desc, tag, speed, mail, sharesize)`
First MyINFO from user. Return `0` to disconnect user.

##### `OnParsedMsgAny(nick, data)`
Every message from logged-in client.

##### `OnParsedMsgAnyEx(ip, data)`
Every message at initial connection stage (before nick known).

##### `OnParsedMsgMyPass(nick, data)`
Login attempt. `data` like `"$MyPass password"`. Return `0` to disconnect.

##### `OnParsedMsgConnectToMe(nick, other_nick, ip, port)`
User wants other_nick to connect. Return `0` to block.

##### `OnParsedMsgRevConnectToMe(nick, other_nick)`
Passive user wants connection. Return `0` to block.

##### `OnParsedMsgSupports(ip, data, filtered)`
Client capability negotiation. `data` is original, `filtered` is processed by hub.

##### `OnParsedMsgMyHubURL(nick, data)`, `OnParsedMsgExtJSON(nick, data)`, `OnParsedMsgBotINFO(nick, data)`, `OnParsedMsgVersion(ip, data)`
Various protocol messages (mostly from bots/pingers).

##### `OnUnknownMsg(nick, data)`
Message not part of NMDC protocol.

**Complete reference**: See [Verlihub Wiki - API Python Events](https://github.com/verlihub/verlihub/wiki/api-python-events) for full details and examples.

### Callback Functions Reference

Functions available through the `vh` module are backed by C++ callbacks. See the [vh Module API](#vh-module-api) section for complete details and [Verlihub Wiki - API Python Methods](https://github.com/verlihub/verlihub/wiki/api-python-methods) for additional documentation.

---

## Architecture

### Bidirectional Communication

The Python plugin supports complete bidirectional communication between C++ and Python:

```
┌─────────────────────────────────────────────────────────────┐
│                 Bidirectional Communication                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  C++ Hub Core                    Python Scripts             │
│  ┌────────────┐                  ┌──────────────┐           │
│  │            │  Event Hooks     │              │           │
│  │  Verlihub  │ ────────────────>│   script.py  │           │
│  │   Core     │  (OnUserLogin,   │              │           │
│  │            │   OnTimer, etc.) │              │           │
│  │            │                  │              │           │
│  │            │  vh Module       │              │           │
│  │            │<────────────────>│ vh.GetUser() │           │
│  │            │  (58 functions)  │ vh.SendMsg() │           │
│  │            │                  │              │           │
│  │            │  Call Function   │              │           │
│  │            │<─────────────────│ return data  │           │
│  │            │  (by name)       │              │           │
│  └────────────┘                  └──────────────┘           │
│       ↕                                 ↕                   │
│  ┌────────────┐                  ┌──────────────┐           │
│  │  C++ Plugin│  Dynamic Reg     │              │           │
│  │            │ ────────────────>│ CallDynamic  │           │
│  │ MyPlugin:: │  Register("fn")  │ Function()   │           │
│  │ GetStats   │<─────────────────│              │           │
│  └────────────┘  Call C++ func   └──────────────┘           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Communication Pathways:**

1. **C++ → Python (Event Hooks)**: Verlihub calls Python when events occur
   - Example: User logs in → `OnUserLogin(nick)` is called
   
2. **Python → C++ (vh Module)**: Python calls Verlihub functions
   - Example: `vh.SendToAll(message)` sends a message from Python
   
3. **C++ → Python (Function Calls)**: C++ invokes arbitrary Python functions
   - Example: C++ calls `calculate_score(username)` defined in Python
   
4. **Python → C++ (Dynamic Registration)**: Python calls runtime-registered C++ functions
   - Example: Plugin registers `get_stats`, Python calls `vh.CallDynamicFunction('get_stats', user)`

### Sub-Interpreter Isolation

Each loaded Python script runs in its own isolated sub-interpreter:

```
Main Python Interpreter
  ├── Sub-Interpreter 1 (script1.py)
  │   ├── Isolated globals
  │   ├── Isolated imports
  │   └── Unique vh module (vh.myid = 0)
  │
  ├── Sub-Interpreter 2 (script2.py)
  │   ├── Isolated globals
  │   ├── Isolated imports
  │   └── Unique vh module (vh.myid = 1)
  │
  └── Sub-Interpreter 3 (script3.py)
      ├── Isolated globals
      ├── Isolated imports
      └── Unique vh module (vh.myid = 2)
```

**Benefits:**
- Scripts cannot interfere with each other's global state
- Each script has its own `vh.myid`, `vh.botname`, etc.
- Errors in one script doesn't affect others
- Scripts can be loaded/unloaded independently

**Limitations:**
- Scripts cannot directly import each other
- Shared data must go through the hub (via configuration or messages)

### Threading Model

The plugin uses the Global Interpreter Lock (GIL) for thread safety:

```
┌─────────────────────────────────────────────────────────────┐
│                      GIL Management                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Hub Thread                         Python Sub-Interpreter  │
│  ┌────────────┐                     ┌──────────────┐        │
│  │            │                     │              │        │
│  │ C++ Code   │ 1. Acquire GIL      │              │        │
│  │            │ ──────────────────> │              │        │
│  │            │                     │ Python Code  │        │
│  │            │ 2. Execute Python   │   Executing  │        │
│  │            │                     │              │        │
│  │ (blocked)  │ <────────────────── │              │        │
│  │            │ 3. Python calls C++ │              │        │
│  │            │                     │              │        │
│  │            │ ──────────────────> │ (blocked)    │        │
│  │ C++ Code   │ 4. Release GIL      │              │        │
│  │ Executing  │                     │              │        │
│  │            │ <────────────────── │              │        │
│  │            │ 5. Return to Python │              │        │
│  │            │                     │              │        │
│  └────────────┘                     └──────────────┘        │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

**Key Points:**
- GIL is acquired before calling Python code
- GIL is released during C++ callback execution
- Multiple scripts can't execute Python simultaneously
- C++ callbacks can run in parallel (after releasing GIL)

**Threading and Daemon Threads:**

Python threading behavior differs significantly between interpreter modes:

**Sub-Interpreter Mode (Default):**
- Scripts are isolated (secure, independent namespaces)
- Using `threading` or `asyncio` causes cleanup issues
- Python bug [#15751](https://bugs.python.org/issue15751): Thread state corruption during interpreter teardown
- Workaround: Skip `Py_EndInterpreter()` and `Py_Finalize()` when threads detected
- Result: Small memory leak (~4-5 MB per script with threads)
- **Trade-off**: No crashes, but incomplete cleanup

**Single-Interpreter Mode (Recommended):**
- All scripts share one interpreter
- Threading and asyncio work perfectly
- Clean shutdown with proper cleanup
- **Note**: Scripts can see each other's globals (use unique naming!)

**When to Use Single Interpreter Mode:**

✅ **Use when:**
- You need complex threading or daemon threads
- You trust all loaded scripts (single developer environment)
- You need asyncio event loops to run continuously
- Memory leaks from threading workaround are unacceptable

❌ **Don't use when:**
- Loading untrusted user scripts (security risk)
- You need script isolation for stability
- Multiple script authors (namespace conflicts)
- Production multi-tenant environments

**Example - Enabling Single Interpreter Mode:**

```bash
# Configure with single interpreter support
cd verlihub/build
cmake -DPYTHON_USE_SINGLE_INTERPRETER=ON ..
make clean && make -j$(nproc)
```

After rebuild, Python scripts will share global namespace:

```python
# script1.py
shared_config = {'debug': True}

def OnUserLogin(nick):
    print(f"Debug mode: {shared_config['debug']}")
    return 1
```

```python
# script2.py - can access script1's variables!
def OnParsedMsgChat(nick, message):
    if message == "!debug":
        shared_config['debug'] = not shared_config['debug']  # Modifies script1's data
    return 1
```

**Memory Usage:**
- Sub-interpreter mode with threading: ~4-5 MB leak per script
- Single interpreter mode: No leaks, proper cleanup

---

## Testing

### Running Tests

```bash
# Build tests
cd build
cmake ..
make

# Run all Python plugin tests
ctest -R Python -V

# Run specific test suites
./plugins/python/test_vh_module
./plugins/python/test_dynamic_registration
./plugins/python/test_python_wrapper_stress
```

### Test Coverage

The plugin includes comprehensive test suites covering all functionality:

**Sub-Interpreter Mode** (default - 10 tests total):
1. **PythonWrapperTests** (3 tests) - Basic hook invocation, script loading/unloading, memory leak testing
2. **PythonWrapperStressTests** (1 test) - 1M hook invocations, memory stability, performance validation
3. **PythonPluginIntegrationTests** (3 tests) - StressTreatMsg (1M messages), ThreadedDataProcessing, AsyncDataProcessing
4. **PythonAdvancedTypesTests** (9 tests) - Complex type marshaling, dict/list/set/tuple handling, PyObject passthrough
5. **VHModuleTests** (5 tests) - Module import, type marshaling, encoding/decoding utilities
6. **DynamicRegistrationTests** (4 tests) - Function registration/unregistration, multiple function management, error handling
7. **JsonMarshalTests** (10 tests) - JSON serialization/deserialization, complex nested structures
8. **JsonIntegrationTests** (6 tests) - End-to-end JSON marshaling with callbacks
9. **SetsTuplesTests** (8 tests) - Python set and tuple marshaling
10. **HubApiStressTests** (5 tests) - 600 rapid commands, 5000 concurrent messages, threading stability

**Single-Interpreter Mode** (with `-DPYTHON_USE_SINGLE_INTERPRETER=ON` - 11 tests total):
- All of the above sub-interpreter tests (10 tests)
- **SingleInterpreterTests** (1 additional test) - Validates shared namespace behavior

**Test Results:**
- ✅ **Sub-Interpreter Mode: 10/10 tests passing (100%)**
- ✅ **Single-Interpreter Mode: 11/11 tests passing (100%)**

**All tests pass in both configurations.** Tests that validate isolation-specific behavior (e.g., `MultipleScriptsHaveIsolatedModules`) are automatically disabled in single-interpreter mode, while integration tests use unique function names to work correctly in both modes.

### Writing Tests

Example test for a new feature:

```cpp
#include <gtest/gtest.h>
#include "wrapper.h"

TEST(MyFeatureTest, BasicFunctionality) {
    // Initialize Python
    w_Tcallback callbacks[W_MAX_CALLBACKS];
    w_Begin(callbacks);
    
    // Test your feature
    int result = my_new_function();
    EXPECT_EQ(result, expected_value);
    
    // Cleanup
    w_End();
}
```

---

## Development

### Building from Source

```bash
# Prerequisites
sudo apt-get install cmake g++ python3-dev libmysqlclient-dev libicu-dev rapidjson-dev

# For running tests (optional)
sudo apt-get install libgtest-dev libcurl4-openssl-dev

# Clone repository
git clone https://github.com/verlihub/verlihub.git
cd verlihub

# Build (default: tests enabled)
mkdir build && cd build
cmake ..
make -j$(nproc)

# Build without tests (if you don't need them)
cmake -DBUILD_PYTHON_TESTS=OFF ..
make -j$(nproc)

# Install
sudo make install
```

#### Build Options

**Single Interpreter Mode** (enables full Python ecosystem compatibility):
```bash
cmake -DPYTHON_USE_SINGLE_INTERPRETER=ON ..
```
Allows FastAPI, numpy, torch, and other modern packages. Trade-off: scripts share global namespace.

**Disable Python Tests** (skip test dependencies):
```bash
cmake -DBUILD_PYTHON_TESTS=OFF ..
```
Tests are enabled by default. Disable to avoid needing GTest/GMock/libcurl development packages.

#### Build Dependencies Explained

**Required for compilation:**
- `cmake` - Build system generator
- `python3-dev` - Python development headers (Python.h)
- `libmysqlclient-dev` - MySQL client library headers
- `libicu-dev` - ICU library for character encoding conversion (supports legacy encodings like CP1251, ISO-8859-1, etc.)
- `rapidjson-dev` - RapidJSON headers for high-performance JSON marshaling between C++ and Python

**Optional for tests** (only needed if `BUILD_PYTHON_TESTS=ON`, which is default):
- `libgtest-dev` - Google Test framework for unit/integration testing
- `libcurl4-openssl-dev` - CURL library with development headers (required by test_hub_api_stress for HTTP API testing)

### Adding New Callbacks

To add a new function to the vh module:

**Step 1**: Add callback ID to `wrapper.h`:

```cpp
enum {
    W_OnNewConn,
    // ... existing callbacks ...
    W_YourNewCallback,  // Add here
    W_MAX_CALLBACKS
};
```

**Step 2**: Implement the wrapper function in `wrapper.cpp`:

```cpp
static PyObject* vh_YourNewFunction(PyObject* self, PyObject* args) {
    const char *param1;
    long param2;
    
    // Parse Python arguments
    if (!PyArg_ParseTuple(args, "sl", &param1, &param2)) {
        return NULL;
    }
    
    // Get script ID
    int script_id = vh_GetScriptID(self);
    if (script_id < 0) Py_RETURN_NONE;
    
    // Pack arguments for C++ callback
    w_Targs *packed = w_pack("sl", param1, param2);
    if (!packed) Py_RETURN_NONE;
    
    // Call C++ callback
    PyThreadState *save = PyEval_SaveThread();
    w_Targs *result = w_Python->callbacks[W_YourNewCallback](W_YourNewCallback, packed);
    PyEval_RestoreThread(save);
    free(packed);
    
    // Convert result
    if (!result) Py_RETURN_NONE;
    
    long ret_value;
    if (w_unpack(result, "l", &ret_value)) {
        free(result);
        return PyLong_FromLong(ret_value);
    }
    
    free(result);
    Py_RETURN_NONE;
}
```

**Step 3**: Add to method table in `wrapper.cpp`:

```cpp
static PyMethodDef vh_methods[] = {
    // ... existing methods ...
    {"YourNewFunction", vh_YourNewFunction, METH_VARARGS, "Description of function"},
    {NULL, NULL, 0, NULL}
};
```

**Step 4**: Implement C++ callback in the main plugin (`cpipython.cpp`):

```cpp
w_Targs* _YourNewCallback(int id, w_Targs* args) {
    char *param1;
    long param2;
    
    if (!w_unpack(args, "sl", &param1, &param2)) {
        return NULL;
    }
    
    // Your C++ implementation
    long result = DoSomething(param1, param2);
    
    return w_pack("l", result);
}
```

**Step 5**: Register callback in `cpipython.cpp`:

```cpp
void cpiPython::OnLoad(cServerDC *server) {
    // ... existing registrations ...
    lib_registerCallback(W_YourNewCallback, _YourNewCallback);
}
```

**Step 6**: Add test in `tests/test_vh_module.cpp`:

```cpp
TEST_F(VHModuleTest, YourNewFunctionWorks) {
    // Create a test script file
    int id = w_ReserveID();
    std::string script_path = CreateTempFile("test_new_function");
    FILE* f = fopen(script_path.c_str(), "w");
    ASSERT_NE(f, nullptr);
    fprintf(f, "import vh\n");
    fprintf(f, "result = vh.YourNewFunction('test', 42)\n");
    fprintf(f, "assert result == expected_value\n");
    fclose(f);
    
    // Load and execute the script
    w_Targs* load_args = w_pack("lssssls", 
        (long)id, 
        strdup(script_path.c_str()),
        strdup("TestBot"),
        strdup("OpChat"),
        strdup("/tmp"),
        (long)1234567890,
        strdup("test_config"));
    
    int result = w_Load(load_args);
    w_free_args(load_args);
    
    EXPECT_EQ(result, id);
    
    // Cleanup
    w_Unload(id);
}
```

### Debugging

**Enable Debug Logging:**

```cpp
// In wrapper.cpp
w_LogLevel(3);  // 0=none, 1=errors, 2=warnings, 3=info, 4=debug
```

**GDB Debugging:**

```bash
# Run hub under debugger
gdb verlihub
(gdb) break wrapper.cpp:123
(gdb) run
```

**Python Exception Tracing:**

```cpp
if (PyErr_Occurred()) {
    PyErr_Print();  // Print to stderr
    PyErr_Clear();  // Clear error state
}
```

**Memory Leak Detection:**

```bash
# Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./plugins/python/test_vh_module

# AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
make
./plugins/python/test_vh_module
```

---

## Testing & Development Status

### Test Suite - All Tests Passing! ✅

**Current Status (December 16, 2025):**

✅ **Sub-Interpreter Mode: 10/10 tests passing (100%)**  
✅ **Single-Interpreter Mode: 11/11 tests passing (100%)**

The Python plugin includes comprehensive tests in `plugins/python/tests/`:

1. **PythonWrapperTests** - Low-level C wrapper tests
   - ✅ Basic hook invocation, script loading/unloading, memory management
   - Passes in both modes

2. **PythonWrapperStressTests** - High-volume stress testing
   - ✅ 1M hook invocations, memory stability validation
   - Passes in both modes

3. **PythonPluginIntegrationTests** - Full integration scenarios
   - ✅ StressTreatMsg: 1M messages, 250K callbacks, throughput ~33K msg/sec
   - ✅ ThreadedDataProcessing: Multi-threaded event processing with unique function names
   - ✅ AsyncDataProcessing: Asyncio event loop with background threads
   - ✅ Memory leak testing: <1KB growth over 1M messages
   - Passes in both modes (integration tests use unique function names to avoid namespace collisions)

4. **SingleInterpreterTests** - Shared namespace validation
   - ✅ Tests script data sharing in single-interpreter mode
   - Only runs in single-interpreter mode (disabled in sub-interpreter mode)

5. **VHModuleTests** - Module functionality
   - ✅ Module import, type marshaling, encoding/decoding
   - Isolation test disabled in single-interpreter mode, others pass in both

6. **DynamicRegistrationTests** - Dynamic function registration
   - ✅ Function registration/unregistration, error handling
   - Isolation test disabled in single-interpreter mode, others pass in both

7. **JsonMarshalTests** - JSON serialization/deserialization
   - ✅ Complex nested structures, all JSON types
   - Passes in both modes

8. **JsonIntegrationTests** - End-to-end JSON marshaling
   - ✅ Full round-trip testing with callbacks
   - Passes in both modes

9. **SetsTuplesTests** - Python set and tuple handling
   - ✅ Set/tuple marshaling, nested structures
   - Passes in both modes

10. **HubApiStressTests** - Real-world threading scenarios
    - ✅ 600 rapid commands, 5000 concurrent messages
    - ✅ Threading stability, no deadlocks or crashes
    - Passes in both modes

### Build Modes - Both Production Ready! ✅

The plugin supports two interpreter modes, **both fully tested and production-ready**:

**Sub-Interpreter Mode (Default - Better Isolation)**
```bash
cmake ..
make -j$(nproc)
ctest  # 10/10 tests pass
```

**Features:**
- ✅ **Script isolation** - Each script has its own Python interpreter
- ✅ **Security** - Scripts cannot interfere with each other's global variables
- ✅ **Independent reload** - Reload one script without affecting others
- ✅ **10/10 tests passing**
- ⚠️ **Package limitations** - Some modern packages don't support sub-interpreters (FastAPI, numpy, etc.)
- ⚠️ **Memory overhead** - Small leak (~4-5MB) when scripts use threading/asyncio (Python bug #15751 workaround)

**Single Interpreter Mode (Better Compatibility)**
```bash
cmake -DPYTHON_USE_SINGLE_INTERPRETER=ON ..
make -j$(nproc)
ctest  # 11/11 tests pass
```

**Features:**
- ✅ **Full package compatibility** - All Python packages work (FastAPI, numpy, pandas, torch, etc.)
- ✅ **Threading/asyncio** - Full support for modern async frameworks
- ✅ **Better performance** - No interpreter switching overhead
- ✅ **11/11 tests passing**
- ⚠️ **Shared namespace** - Scripts share global state (use unique prefixes for variables)
- ⚠️ **No isolation** - One script's crash can affect others

**Comparison Table:**

| Feature | Sub-Interpreter (Default) | Single Interpreter |
|---------|--------------------------|-------------------|
| Test Status | ✅ 10/10 passing (100%) | ✅ 11/11 passing (100%) |
| Production Ready | ✅ Yes | ✅ Yes |
| Script Isolation | ✅ Full isolation | ⚠️ Shared namespace |
| Package Support | ⚠️ Limited (pure Python only) | ✅ All packages |
| Threading/Asyncio | ✅ Works (small memory leak) | ✅ Full support |
| FastAPI/numpy/ML | ❌ Not supported | ✅ Fully supported |
| Memory Management | ⚠️ Small leak with threads | ✅ Clean |
| Use Case | Multi-user dev environments | Production with modern packages |

### Running Tests

**Sub-Interpreter Mode (Default):**
```bash
cd build
cmake ..
make -j$(nproc)
ctest  # All 10 tests pass

# Or run specific tests:
./plugins/python/test_python_plugin_integration
./plugins/python/test_hub_api_stress
```

**Single Interpreter Mode:**
```bash
cd build
cmake -DPYTHON_USE_SINGLE_INTERPRETER=ON ..
make -j$(nproc)
ctest  # All 11 tests pass

# Or run specific tests:
./plugins/python/test_single_interpreter
./plugins/python/test_python_plugin_integration
```

**Expected Results:**
- ✅ Sub-interpreter mode: **10/10 tests passing (100%)**
- ✅ Single-interpreter mode: **11/11 tests passing (100%)**
- ✅ No crashes, no segfaults, clean shutdown in both modes
- ✅ All integration tests work correctly with unique function names

---

## Troubleshooting

### Script Not Loading

**Problem**: Script fails to load with "ImportError"

**Solutions**:
- Check Python version: `python3 --version` (must be 3.12+)
- Verify script syntax: `python3 -m py_compile script.py`
- Check import paths: Add `import sys; print(sys.path)` to script

### vh Module Not Found

**Problem**: `ModuleNotFoundError: No module named 'vh'`

**Solution**: The vh module is only available inside Verlihub's Python environment. It cannot be imported from a standalone Python interpreter.

### Hook Not Being Called

**Problem**: Defined hook function never executes

**Solutions**:
- Check function name matches exactly (case-sensitive)
- Ensure return value (must return int: 0 or 1)
- Check hub configuration (some events may be disabled)
- Verify script is loaded: `!pylist`

### Segmentation Fault

**Problem**: Hub crashes when calling Python

**Solutions**:
- Check GIL management (acquire before Python calls, release during C++)
- Verify reference counting (Py_INCREF/Py_DECREF balance)
- Check for NULL pointers before dereferencing
- Run under valgrind to detect memory errors

### Deadlock on Signal (SIGTERM, SIGHUP, etc.)

**Problem**: Hub hangs/deadlocks when receiving signals, backtrace shows `PyGILState_Ensure()` waiting

**Root Cause**: Signal handlers must NOT call Python code. When a signal interrupts Python execution, the GIL is already held. Calling `PyGILState_Ensure()` from the signal handler creates a deadlock.

**Solutions**:
1. **Recommended**: Ensure Verlihub core does not call plugin hooks from signal handlers
2. **Workaround**: The Python plugin now automatically skips hook calls during signal context to prevent deadlocks
3. **For Plugin Developers**: If you control the signal handler, call `w_SetSignalContext(1)` on entry and `w_SetSignalContext(0)` on exit:
   ```cpp
   void my_signal_handler(int sig) {
       w_SetSignalContext(1);  // Mark signal context
       // ... signal handling (don't call Python!)  ...
       w_SetSignalContext(0);  // Clear signal context
   }
   ```

**Why This Happens**:
- Signal handler interrupts running Python code (GIL held by thread A)
- Signal handler tries to call Python hook
- `PyGILState_Ensure()` in hook tries to acquire GIL
- Deadlock: thread A has GIL and is interrupted, signal handler waits for GIL that will never be released

**Best Practice**: Use deferred signal handling - set a flag in the signal handler and check it in the main loop, then call plugin hooks from the main loop, not from the signal handler itself.

### Performance Issues

**Problem**: Hub slows down with Python scripts

**Solutions**:
- Minimize work in OnTimer (called every second)
- Use OnTimer modulo for periodic tasks: `if msec % 60000 < 1000:`
- Avoid expensive operations in high-frequency hooks (OnParsedMsgChat)
- Profile Python code: `import cProfile`

### Script Conflicts

**Problem**: Multiple scripts interfere with each other

**Solution**: Scripts run in isolated sub-interpreters and cannot directly interfere. If you need shared state, use vh.SetConfig/GetConfig or database.

---

## License

Verlihub is licensed under the GNU General Public License v3.0 or later.

See [http://www.gnu.org/licenses/](http://www.gnu.org/licenses/) for details.

---

**For more information:**
- GitHub: https://github.com/verlihub/verlihub
- Wiki: https://github.com/verlihub/verlihub/wiki
- Issues: https://github.com/verlihub/verlihub/issues
