# Python Plugin TODO

## Critical Issues

### Signal Handler Safety - REQUIRES CORE FIX

**Status**: Workaround implemented, core fix needed  
**Priority**: HIGH  
**Issue**: Deadlock when signals (SIGTERM, SIGHUP, etc.) trigger plugin hook calls

#### Problem Description

The Verlihub core currently calls plugin hooks directly from signal handlers. This causes deadlocks in the Python plugin because:

1. Signal handler interrupts Python code execution (GIL held by interrupted thread)
2. Signal handler calls plugin hooks → `cpiPython::CallAll()` → `w_CallHook()`
3. `w_CallHook()` tries to acquire GIL via `PyGILState_Ensure()`
4. **DEADLOCK**: Interrupted thread has GIL and is paused, signal handler waits forever for GIL

This is a fundamental limitation of Python's threading model - the GIL cannot be safely acquired from signal handlers.

#### Current Workaround

The Python plugin now has defensive code to detect and skip hook calls from signal context:

```cpp
// In wrapper.cpp
static volatile sig_atomic_t in_signal_handler = 0;

void w_SetSignalContext(int is_signal) {
    in_signal_handler = is_signal ? 1 : 0;
}

w_Targs *w_CallHook(int id, int num, w_Targs *params) {
    if (in_signal_handler) {
        log("[Python] WARNING: Skipping hook call from signal handler\n");
        return NULL;
    }
    // ... rest of hook call
}
```

**Limitations of Workaround**:
- Plugins don't receive shutdown/cleanup notifications when signaled
- `UnLoad()` hooks may not be called on SIGTERM
- Scripts can't perform graceful shutdown (save state, send messages, etc.)
- Relies on external code calling `w_SetSignalContext()` - core doesn't use it yet
- Only prevents deadlock, doesn't provide the functionality users expect

#### Required Core Fix

The Verlihub core needs to implement **deferred signal handling** pattern:

**Current (UNSAFE) code pattern** (approximate location: `src/cserverdc.cpp` or main):
```cpp
void mySigServHandler(int sig) {
    // WRONG: Calling plugins directly from signal handler
    if (sig == SIGTERM || sig == SIGINT) {
        // This calls Python and can deadlock!
        server->mCallBacks.CallAll(eSIGNAL_TERM);
    }
}
```

**Correct (SAFE) code pattern**:
```cpp
// Global flag (sig_atomic_t is signal-safe)
static volatile sig_atomic_t pending_signal = 0;

void mySigServHandler(int sig) {
    // CORRECT: Just set a flag, don't call anything complex
    if (sig == SIGTERM || sig == SIGINT) {
        pending_signal = SIGTERM;
    }
    // Signal handler exits immediately - no deadlock possible
}

// In main event loop (cServerDC::run() or similar)
void cAsyncSocketServer::run() {
    while (running) {
        // Check signal flag from main thread
        if (pending_signal) {
            int sig = pending_signal;
            pending_signal = 0;  // Clear flag
            
            // NOW it's safe to call plugins - we're in main thread
            if (sig == SIGTERM) {
                mCallBacks.CallAll(eSIGNAL_TERM);
                stop();  // Initiate shutdown
            }
        }
        
        // ... rest of event loop
        TimeStep();
    }
}
```

**Benefits**:
- ✅ No deadlock - plugin hooks called from main thread
- ✅ Plugins can perform cleanup (save state, flush buffers, etc.)
- ✅ Python scripts receive `UnLoad()` notification
- ✅ Standard signal handling pattern used by most servers
- ✅ Works with all plugin types, not just Python

#### Files to Modify in Core

1. **src/cserverdc.cpp** (or wherever signal handlers are defined)
   - Add `volatile sig_atomic_t pending_signal` global
   - Modify signal handlers to just set flag
   
2. **src/casyncconn.cpp** or **src/cserverdc.cpp** (main event loop)
   - Add signal flag check in main loop
   - Call plugin callbacks from there instead

3. **Signal handler registration** (likely in main.cpp or cServerDC constructor)
   - Keep existing signal() or sigaction() calls
   - Just change what the handler does

#### Testing the Fix

Once core is fixed, test with:

```bash
# Start hub with Python plugin loaded
verlihub

# Send SIGTERM
kill -TERM <pid>

# Verify in logs:
# - Python scripts receive UnLoad() calls
# - No deadlock
# - Clean shutdown
```

#### References

- Python documentation: https://docs.python.org/3/library/signal.html
  > "Python signal handlers are always executed in the main Python thread of the main interpreter, 
  > even if the signal was received in another thread. This means that signals can't be used as a 
  > means of inter-thread communication."
  
- POSIX signal safety: https://man7.org/linux/man-pages/man7/signal-safety.7.html
  > Signal handlers must only call async-signal-safe functions

---

## Future Enhancements

### Performance Optimizations

- [ ] Implement PyObject caching for frequently called hooks
- [ ] Pool PyTuple objects for argument passing
- [ ] Consider using Python's buffer protocol for large data transfers

### Feature Additions

- [ ] Add support for Python async/await in hooks (using asyncio)
- [ ] Provide streaming API for large search results
- [ ] Add Python profiling hooks for performance analysis
- [ ] Support for Python type hints in dynamic registration

### Code Quality

- [ ] Add more comprehensive error messages for common mistakes
- [ ] Create migration guide for scripts from old Python 2 plugin
- [ ] Add performance benchmarks suite
- [ ] Document memory usage patterns and best practices

### Testing

- [ ] Add stress tests for signal handling (once core is fixed)
- [ ] Test GIL management under heavy concurrent load
- [ ] Add tests for edge cases in type marshaling
- [ ] Create automated tests for all vh module functions

---

## Completed

- ✅ Python 3.12+ support with sub-interpreters
- ✅ Full bidirectional C++/Python communication
- ✅ JSON marshaling with RapidJSON
- ✅ Thread-safe implementation
- ✅ Dynamic function registration
- ✅ Comprehensive test suite
- ✅ Signal handler defensive measures (workaround)
- ✅ Documentation of signal safety issues
