# GetIPCity Crash Debugging Notes

## Current Status
The GetIPCity function is causing crashes with "munmap_chunk(): invalid pointer" error.

## Changes Made

### 1. Added Debug Logging to _GetIPCity (cpipython.cpp:1764-1795)
The function now logs:
- Input parameters (ip, db)
- Result from GetIPCity API call
- Memory address of strdup'd result
- Any exceptions that occur

### 2. Enhanced Unit Test (test_hub_api_stress.cpp)
Created `DirectGetIPCityCall` test that:
- Calls GetIPCity 500+ times with various IPs
- Tests string manipulation to trigger memory issues
- Tests edge cases (invalid IP, None, empty)
- **NOTE**: Test is DISABLED by default (requires MySQL)

## Memory Management Flow

1. **Python calls** `vh.GetIPCity(ip, "")`
2. **Wrapper (wrapper.cpp:1165)** `vh_GetIPCity` → `vh_CallString(W_GetIPCity, args, "ss")`
3. **vh_ParseArgs** allocates input strings with `strdup()` (line 916)
4. **Callback** _GetIPCity is invoked with w_Targs containing allocated strings
5. **_GetIPCity** calls `GetIPCity(ip, db)` script API, gets std::string result
6. **_GetIPCity** returns `lib_pack("s", strdup(citystr.c_str()))`
7. **vh_CallString** frees input args (lines 1002-1006)
8. **vh_CallString** calls `w_free_args(res)` to free return value (line 1030)
9. **w_free_args** calls `free()` on the strdup'd string (wrapper.cpp:442-470)

This flow SHOULD be correct - strdup allocates with malloc, free() releases it.

## Possible Root Causes

### Theory 1: Thread Safety
GetIPCity might be called from multiple threads simultaneously:
- FastAPI runs in uvicorn with threading
- Python GIL might not protect C++ code
- GetIPCity accesses mMaxMindDB which might not be thread-safe

### Theory 2: Heap Corruption
Something else is corrupting the heap before free() is called:
- Buffer overflow in MaxMindDB library
- Invalid pointer arithmetic somewhere
- Double-free of a different object

### Theory 3: Empty String Edge Case
When GetIPCity returns empty string (""), the flow is:
- `citystr = ""` (empty std::string)
- `strdup(citystr.c_str())` → strdup("") → allocates 1 byte for `\0`
- Should be valid, but check if wrapper handles empty strings correctly

### Theory 4: Exception During Cleanup
If an exception is thrown after strdup but before lib_pack:
- The strdup'd memory would leak
- But this wouldn't cause "invalid pointer" unless something else frees it

## Debugging Steps

### Step 1: Check Debug Logs
After restarting Verlihub, check logs for:
```
PY: _GetIPCity called with ip='...', db='...'
PY: _GetIPCity result: '...' (len=...)
PY: _GetIPCity returning strdup'd string at 0x...
```

If crash occurs, last log entry will show which call failed.

### Step 2: Look for Patterns
- Does it crash on specific IPs?
- Does it crash after N calls?
- Does it crash only during high load?
- Does it crash only when FastAPI is running?

### Step 3: Valgrind Analysis (if reproducible)
```bash
valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all \
  /usr/local/bin/verlihub
```

### Step 4: GDB Core Dump Analysis
If core dumps are enabled:
```bash
gdb /usr/local/bin/verlihub core
bt full
frame N
print *variable
```

## Implementation Comparison

### GetIPCC (WORKS) - Single Argument
```cpp
w_Targs *_GetIPCC(int id, w_Targs *args) {
    const char *ip;
    if (!cpiPython::lib_unpack(args, "s", &ip)) return NULL;
    if (!ip) return NULL;
    string ccstr = GetIPCC(ip);
    return cpiPython::lib_pack("s", strdup(ccstr.c_str()));
}
```

### GetIPCity (CRASHES) - Two Arguments
```cpp
w_Targs *_GetIPCity(int id, w_Targs *args) {
    const char *ip, *db;
    if (!cpiPython::lib_unpack(args, "ss", &ip, &db)) return NULL;
    if (!ip) return NULL;
    if (!db) db = "";
    string citystr = GetIPCity(ip, db);
    return cpiPython::lib_pack("s", strdup(citystr.c_str()));
}
```

**Key Difference**: GetIPCity unpacks TWO strings ("ss") vs GetIPCC unpacks ONE ("s").

## Hypothesis
The wrapper's `vh_ParseArgs` allocates TWO strings for GetIPCity.
Both are freed in the cleanup loop (lines 1002-1006).
If the second string (`db`) is passed as empty string from Python,
it gets allocated as `strdup("")` by vh_ParseArgs (line 916).
Then in _GetIPCity, if db is NULL, we set `db = ""` (line 1774).

**WAIT** - This might be the bug! If vh_ParseArgs allocates db as strdup(""),
and we then overwrite the pointer with `db = ""` (string literal),
we've lost the allocated pointer and it will never be freed!

## Potential Fix
```cpp
w_Targs *_GetIPCity(int id, w_Targs *args) {
    const char *ip, *db;
    if (!cpiPython::lib_unpack(args, "ss", &ip, &db)) return NULL;
    if (!ip) return NULL;
    
    // DON'T reassign db if it's NULL - vh_ParseArgs already handled it
    // If Python passed None, db will be NULL
    // If Python passed "", db will be strdup("") 
    const char *db_safe = (db && db[0]) ? db : "";  // Use empty literal only for API call
    
    string citystr = GetIPCity(ip, db_safe);
    return cpiPython::lib_pack("s", strdup(citystr.c_str()));
}
```

Actually no - lib_unpack should not modify what's in args. Let me check what lib_unpack does...

Actually, reviewing the code again, I don't think that's the issue. The `db` pointer from lib_unpack points into the args structure, and reassigning the local variable doesn't affect the args structure that will be freed.

## Next Steps
1. ✅ Rebuild with debug logging
2. ✅ Install updated plugin  
3. ⏳ Restart Verlihub and monitor logs
4. ⏳ Reproduce crash and analyze logs
5. ⏳ If needed, add more targeted logging or try valgrind

## Test Command
To run the unit test (requires MySQL):
```bash
cd /home/tfx/src/verlihub-py3-build-6
./plugins/python/test_hub_api_stress --gtest_filter="*DirectGetIPCityCall" --gtest_also_run_disabled_tests
```
