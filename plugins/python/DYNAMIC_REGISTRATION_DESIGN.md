# Dynamic C++ Function Registration Design (Fourth Dimension)

## Critical Discovery: The vh Module Was Removed

**Update December 14, 2025**: After investigating the wiki documentation that shows `import vh` examples, we discovered:

### The Mystery Solved

1. **The vh module DID exist** - It was implemented in the original Python 2.x plugin (pre-2016)
2. **It was completely removed** - During the Python 3 migration (commit 663e50b, Sept 30, 2025), **1692 lines were deleted** from wrapper.cpp
3. **The wiki is outdated** - The [API documentation](https://github.com/verlihub/verlihub/wiki/api-python-methods) was last edited in 2016 and documents the old Python 2 API
4. **Current state**: Python scripts can only define hooks (OnParsedMsgChat, etc.) but **cannot call C++ functions back**

### What Was Lost

The original Python 2 implementation had:
- **55+ Python-callable functions** via `PyMethodDef w_vh_methods[]`
- **Complete vh module** using `Py_InitModule("vh", w_vh_methods)`
- **All the functions documented in the wiki**: SetConfig, GetConfig, SendToAll, KickUser, etc.
- **Helper functions** like `BasicCall()` and `Call()` that wrapped the callback invocations

Example from the old code:
```cpp
static PyObject *__SetConfig(PyObject *self, PyObject *args) {
    return pybool(BasicCall(W_SetConfig, args, "sss"));
}

static PyMethodDef w_vh_methods[] = {
    {"SetConfig",    __SetConfig,    METH_VARARGS},
    {"GetConfig",    __GetConfig,    METH_VARARGS},
    {"SendToAll",    __SendToAll,    METH_VARARGS},
    // ... 52 more functions
    {NULL, NULL}
};
```

### Why It Was Removed

The Python 3 migration removed the vh module because:
- **API incompatibility**: Python 2's `Py_InitModule()` was replaced by `PyModule_Create()` in Python 3
- **Major refactoring**: The commit message says "first stab at python3 support" - it was a quick port
- **Incomplete migration**: The hooks (OnParsedMsgChat, etc.) were migrated but the callback system was not

## Executive Summary

**Status**: ✅ RESTORATION FEASIBLE (not new feature, but restoration)  
**Complexity**: Medium (2-3 days to restore + modernize)  
**Risk Level**: Low (restoring proven architecture with modern Python 3 API)  
**Breaking Changes**: None (backward compatible, adds missing functionality)

## Current Architecture Analysis

### Existing Bidirectional Communication

**Dimension 1: C++ → Python (Event Hooks)**
- C++ calls `w_CallHook(id, enum_id, params)` 
- wrapper.cpp discovers hooks in Python script via `PyObject_GetAttrString("OnNewConn")`
- Python function executed in sub-interpreter with GIL management
- **Status**: ✅ Complete

**Dimension 2: Python → C++ (Pre-registered Callbacks)**
- C++ registers callbacks in array: `callbacklist[W_GetUserClass] = &_GetUserClass`
- Callbacks stored in `w_Python->callbacks` array during `w_Begin()`
- Python scripts... **CRITICAL GAP IDENTIFIED**
- **Current Limitation**: Python scripts do NOT have a `vh` module to call these functions

**Dimension 3: C++ → Python (Arbitrary Functions)**
- C++ calls `w_CallFunction(id, "function_name", params)`
- Uses `PyObject_GetAttrString()` for dynamic lookup
- Supports all marshaling types (l, s, d, p, L, D, O)
- **Status**: ✅ Complete

**Dimension 4: Python → C++ (Arbitrary Functions)** ← **REQUESTED**
- **Current**: Does not exist
- **Requested**: Python can call C++ functions registered at runtime by name
- **Example**: `RegisterFunction("get_stats", callback)` → `python_script.get_stats()`

## Architecture Discovery - The Missing Piece

### What We Found

After extensive analysis and git archaeology, we discovered:

1. **The vh Module Was Deleted**: During Python 3 migration (Sept 2025), the entire vh module implementation was removed from wrapper.cpp (1692 lines deleted)

2. **It Used to Work**: The original Python 2 plugin had full bidirectional communication via `Py_InitModule("vh", w_vh_methods)` with 55+ callable functions

3. **Callbacks Still Registered**: The C++ code still registers 55+ callbacks in `callbacklist[]` and stores them in `w_Python->callbacks`, but Python can no longer invoke them

4. **Documentation Is Historical**: The wiki and README examples document the OLD Python 2 API that no longer exists in the Python 3 version

### Current State Summary

```
┌─────────────────────────────────────────────────────────────┐
│ Current Communication Paths                                  │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  C++ Plugin (cpipython)                                      │
│     │                                                         │
│     ├─→ w_CallHook(W_OnParsedMsgChat, args) ─────────┐      │
│     │                                          [D1]    │      │
│     │                                                  ▼      │
│     │                                    Python Script        │
│     │                                    def OnParsedMsgChat  │
│     │                                                  │      │
│     ◄───────────────────────────────────── returns ───┘      │
│     │                                                         │
│     ├─→ w_CallFunction("get_stats", args) ──────────┐       │
│     │                                         [D3]    │       │
│     │                                                 ▼       │
│     │                                    Python Script        │
│     │                                    def get_stats()      │
│     │                                                 │       │
│     ◄──────────────────────────────────── returns ───┘       │
│                                                               │
│  ❌ Missing: Python calling C++ callbacks                    │
│     Python needs to call: SendToAll(), GetUserClass(), etc.  │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

## Proposed Solution: Restore vh Module with Python 3 API

### Design Philosophy

**This is not a new feature** - it's **restoring functionality** that existed in Python 2 but was lost during migration. We'll modernize it for Python 3.

### Implementation Strategy

#### Option A: Python 3 C API Module (Recommended - Based on Original Code)

Restore the Python C extension module `vh` using Python 3's modern API, based on the original Python 2 implementation.

Create a Python C extension module `vh` that exposes all 55+ callbacks as Python-callable functions.

**Pros:**
- Standard Python approach
- Type-safe with proper error handling
- Zero Python-side performance overhead
- Integrates with Python introspection (help(), dir())
- Can be documented with docstrings

**Cons:**
- Requires ~500-1000 lines of boilerplate code
- Must write PyMethodDef array for each function

**Implementation Sketch (Python 3 version of original code):**

```cpp
// In wrapper.cpp - Python 3 version

// Modern wrapper using existing w_pack/w_unpack infrastructure
static PyObject* vh_SetConfig(PyObject* self, PyObject* args) {
    const char *conf, *var, *val;
    if (!PyArg_ParseTuple(args, "sss", &conf, &var, &val))
        return NULL;
    
    // Invoke callback via existing table (like old BasicCall)
    w_Targs* packed = w_pack("sss", (char*)conf, (char*)var, (char*)val);
    w_Targs* result = w_Python->callbacks[W_SetConfig](W_SetConfig, packed);
    
    // Convert result to bool
    long ret = 0;
    w_unpack(result, "l", &ret);
    w_free(packed);
    w_free(result);
    
    if (ret) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject* vh_GetConfig(PyObject* self, PyObject* args) {
    const char *conf, *var;
    if (!PyArg_ParseTuple(args, "ss", &conf, &var))
        return NULL;
    
    w_Targs* packed = w_pack("ss", (char*)conf, (char*)var);
    w_Targs* result = w_Python->callbacks[W_GetConfig](W_GetConfig, packed);
    
    // Convert result to string
    char* res = NULL;
    w_unpack(result, "s", &res);
    
    PyObject* py_result = NULL;
    if (res && res[0]) {
        py_result = PyUnicode_FromString(res);
    } else {
        Py_INCREF(Py_None);
        py_result = Py_None;
    }
    
    w_free(packed);
    w_free(result);
    return py_result;
}

// Python 3 method table
static PyMethodDef vh_methods[] = {
    {"SendToOpChat",       vh_SendToOpChat,       METH_VARARGS, "Send message to op chat"},
    {"SendToAll",          vh_SendToAll,          METH_VARARGS, "Send message to all users"},
    {"SetConfig",          vh_SetConfig,          METH_VARARGS, "Set configuration value"},
    {"GetConfig",          vh_GetConfig,          METH_VARARGS, "Get configuration value"},
    {"GetUserClass",       vh_GetUserClass,       METH_VARARGS, "Get user class"},
    {"KickUser",           vh_KickUser,           METH_VARARGS, "Kick a user"},
    // ... 49 more entries (all from old w_vh_methods[])
    {NULL, NULL, 0, NULL}
};

// Python 3 module definition (replaces Py_InitModule)
static struct PyModuleDef vh_module = {
    PyModuleDef_HEAD_INIT,
    "vh",
    "Verlihub C++ callback interface (Python 3)",
    -1,
    vh_methods
};

// Python 3 module initializer
PyMODINIT_FUNC PyInit_vh(void) {
    PyObject* m = PyModule_Create(&vh_module);
    if (m == NULL)
        return NULL;
    
    // Add constants (from old code)
    PyModule_AddIntConstant(m, "myid", (long)/* script id */);
    PyModule_AddStringConstant(m, "botname", /* script->botname */);
    
    // Add enums (from old code)
    PyModule_AddIntMacro(m, eCR_KICKED);
    PyModule_AddIntMacro(m, eCR_TIMEOUT);
    // ... more constants
    
    return m;
}

// In w_Load(), register the module (Python 3 way)
int w_Load(w_Targs *args) {
    // ... existing setup code ...
    
    PyGILState_STATE sub_gil = PyGILState_Ensure();
    
    // Add vh module to the interpreter
    PyObject* vh_mod = PyInit_vh();
    if (vh_mod) {
        PyObject* modules = PyImport_GetModuleDict();
        PyDict_SetItemString(modules, "vh", vh_mod);
        Py_DECREF(vh_mod);
    }
    
    // ... rest of load code ...
}
```

#### Option B: Template Wrapper Generator (More Elegant)

Use C++ templates to auto-generate the module with less boilerplate.

**Pros:**
- DRY principle - define once
- Type-safe at compile time
- Easier to maintain (add callback in one place)
- Can leverage existing `WrapStringToLong` pattern

**Cons:**
- More complex metaprogramming
- Harder to debug template errors

**Implementation Sketch:**

```cpp
// Template wrapper that generates PyMethodDef entry
template<int CallbackID, typename... Args>
struct VHMethod {
    static PyObject* call(PyObject* self, PyObject* args) {
        // Extract arguments using template parameter pack
        // Invoke w_Python->callbacks[CallbackID]
        // Return PyObject
    }
    
    static const char* name() {
        return w_CallName(CallbackID);
    }
};

// Generate method table at compile time
constexpr PyMethodDef vh_methods[] = {
    MAKE_METHOD(W_SendDataToAll, "s"),
    MAKE_METHOD(W_GetUserClass, "s"),
    // ...
    {NULL, NULL, 0, NULL}
};
```

#### Option C: Dynamic Registration (True Fourth Dimension)

Implement runtime function registration as originally requested.

**Pros:**
- Maximum flexibility
- Scripts can register custom C++ callbacks at runtime
- True symmetry with dimension 3

**Cons:**
- More complex (needs std::map<string, callback>)
- Thread safety concerns with registration
- Lifetime management of registered functions
- Not needed if vh module provides all static callbacks

### Recommended Approach

**Phase 1 (Immediate - 2-3 days)**:
1. **Restore vh module** using Python 3 API (based on original Python 2 code)
2. Port all 55+ functions from old `w_vh_methods[]` to new `vh_methods[]`
3. Modernize `Py_InitModule` → `PyModule_Create` pattern
4. Restore helper functions (BasicCall, Call patterns) adapted for w_pack/w_unpack
5. Update README to mark examples as "working again"
6. Add unit tests validating the restored functionality

**Phase 2 (Future - Optional)**:
1. If runtime registration is truly needed, add Option C
2. Implement `vh.register_function(name, callback)` API  
3. Extend module to support dynamic lookups beyond the 55 static callbacks

## Technical Specifications

### Type Marshaling

The existing `w_pack/w_unpack` infrastructure supports:

| Format | C++ Type | Python Type | Usage |
|--------|----------|-------------|-------|
| `'l'` | `long` | `int` | Integers, IDs, counts |
| `'s'` | `char*` | `str` | Strings, nicknames |
| `'d'` | `double` | `float` | Decimals, percentages |
| `'p'` | `void*` | `int` (as capsule) | Pointers (advanced) |
| `'L'` | `char**` | `list[str]` | String lists |
| `'D'` | `char*` (JSON) | `dict` | Dictionaries |
| `'O'` | `PyObject*` | `object` | Arbitrary Python objects |

### Example API Conversion

```python
# Python script using vh module (AFTER implementation)
import vh

def OnParsedMsgChat(nick, message):
    if message.startswith("!stats"):
        # Call C++ function to get stats
        user_class = vh.GetUserClass(nick)
        user_count = vh.GetUsersCount()
        
        # Call C++ function to send response
        vh.SendToAll(f"User {nick} (class {user_class}) requested stats. Online: {user_count}")
    return 1
```

Corresponding C++ wrapper:

```cpp
static PyObject* vh_GetUserClass(PyObject* self, PyObject* args) {
    const char* nick = NULL;
    if (!PyArg_ParseTuple(args, "s", &nick))
        return NULL;
    
    w_Targs* packed = w_pack("s", (char*)nick);
    w_Targs* result = w_Python->callbacks[W_GetUserClass](W_GetUserClass, packed);
    
    long user_class = 0;
    w_unpack(result, "l", &user_class);
    
    w_free(packed);
    w_free(result);
    
    return PyLong_FromLong(user_class);
}

static PyObject* vh_GetUsersCount(PyObject* self, PyObject* args) {
    w_Targs* packed = w_pack("");  // No arguments
    w_Targs* result = w_Python->callbacks[W_GetUsersCount](W_GetUsersCount, packed);
    
    long count = 0;
    w_unpack(result, "l", &count);
    
    w_free(packed);
    w_free(result);
    
    return PyLong_FromLong(count);
}
```

## Constraints & Compliance

### User Requirements

✅ **Stay within Python plugin** - All changes in `plugins/python/`  
✅ **Type-safe** - Uses existing w_pack/w_unpack marshaling  
✅ **No external dependencies** - Only Python.h (already used)  
✅ **Don't modify main application** - Zero changes to `src/`  

### Performance Impact

- **Module initialization**: One-time cost at startup (~100μs)
- **Function call overhead**: ~2-5μs per callback invocation
- **Memory overhead**: PyMethodDef array ~5KB
- **GIL management**: Already handled by existing infrastructure

### Backward Compatibility

✅ **Existing scripts unaffected** - Hook-only scripts continue working  
✅ **No API breakage** - All existing callbacks remain enum-based  
✅ **Graceful degradation** - Scripts without `import vh` still work  

## Implementation Checklist

### Core Implementation (wrapper.cpp)

- [ ] Define PyMethodDef array with all 55+ callbacks
- [ ] Implement vh_* wrapper functions for each callback
- [ ] Create PyModuleDef for vh module
- [ ] Implement PyInit_vh() module initializer
- [ ] Register module in w_Begin() with PyImport_AppendInittab()
- [ ] Handle GIL state in all wrapper functions
- [ ] Add error handling and type checking

### Testing (tests/)

- [ ] Unit test: Import vh module in Python script
- [ ] Unit test: Call vh.SendToAll() and verify callback invoked
- [ ] Unit test: Call vh.GetUserClass() and verify return value
- [ ] Unit test: Test all 7 marshaling types (l, s, d, p, L, D, O)
- [ ] Integration test: Script using multiple vh.* calls
- [ ] Stress test: 10K vh.* calls for performance validation
- [ ] Error test: Invalid arguments, NULL pointers, GIL deadlocks

### Documentation (README.md)

- [ ] Update "Potential Fourth Dimension" section → "vh Module API"
- [ ] Change examples from hypothetical to actual working code
- [ ] Document all 55 available vh.* functions
- [ ] Add type signatures for each function
- [ ] Include error handling examples
- [ ] Update bidirectional communication validation section

### Build System (CMakeLists.txt)

- [ ] Ensure libvh_python_wrapper.so exports PyInit_vh symbol
- [ ] Add test_vh_module.cpp unit test
- [ ] Link test executables properly

## Risk Assessment

### Low Risk

- ✅ Uses proven Python C API patterns
- ✅ Builds on existing w_pack/w_unpack infrastructure
- ✅ GIL management already working
- ✅ Sub-interpreter isolation maintained
- ✅ No threading issues (callbacks execute in main thread)

### Medium Risk

- ⚠️ **Boilerplate code volume**: 55 functions × 20 lines = ~1100 lines
  - Mitigation: Use code generation or templates
  
- ⚠️ **Type conversion edge cases**: NULL strings, out-of-range integers
  - Mitigation: Follow cpython best practices, add defensive checks

### Mitigated Risks

- ~~Memory leaks~~ → Use existing w_free() infrastructure
- ~~GIL deadlocks~~ → Follow established PyGILState_Ensure/Release pattern
- ~~Script isolation breaking~~ → Module is per-interpreter, not global

## Alternative Considered (Rejected)

### ctypes-based Approach

Python scripts could use ctypes to call C functions directly:

```python
import ctypes
vh = ctypes.CDLL("libvh_python_wrapper.so")
vh.w_Python.callbacks[11](...)  # Unsafe, fragile
```

**Rejected because:**
- Not type-safe
- Exposes internal implementation details
- No error handling
- Breaks on ABI changes
- Violates "type safe way" requirement

## Conclusion

**Restoring the vh module is feasible, historically proven, and necessary**. It:

1. **Restores lost functionality** from Python 2 version
2. **Completes the bidirectional communication loop** (dimension 2)
3. **Makes wiki documentation work again** (examples from 2016 will function)
4. **Uses proven architecture** (55 functions worked for years in Python 2)
5. **Maintains all constraints** (plugin-only, type-safe, no external deps)
6. **Low risk** (restoring proven code with modern API)

### Historical Context

- **2003-2016**: Python 2 plugin with full vh module support (55+ functions)
- **Mar 2016**: Wiki documentation last updated (documents working API)
- **Sept 2025**: Python 3 migration removed vh module (incomplete port)
- **Oct-Dec 2025**: Advanced type marshaling added, but vh module still missing
- **Dec 2025**: Discovery that dimension 2 was lost, not unimplemented

### Next Steps

1. ✅ **Document the discovery** (this file)
2. ⏭️ Extract all 55 function implementations from commit 663e50b^
3. ⏭️ Port each function from Py_InitModule to PyModule_Create
4. ⏭️ Modernize BasicCall/Call helpers to use w_pack/w_unpack
5. ⏭️ Add comprehensive unit tests (all 55 functions)
6. ⏭️ Update README: "Bidirectional API Restored" section
7. ⏭️ (Optional) Later add dynamic registration if needed

**Estimated Effort**: 2-3 days for complete restoration + testing + documentation

---

*Document created: 2025-12-14*  
*Discovery: vh module existed in Python 2, removed in Python 3 migration*  
*Git archaeology: commit 663e50b (Python 3 migration, -1692 lines)*  
*Original implementation: PyMethodDef w_vh_methods[] with 55 functions*
