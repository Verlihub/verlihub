# Python Environment Configuration for Verlihub

## Current Issues and Solutions

### 1. Standard Library Import Failures (`import json`, `import re`, etc.)

**Problem**: Python scripts cannot import standard library modules because `PySys_SetPath()` in `wrapper.cpp` **replaces** the entire Python path instead of **appending** to it.

**Current Code** (wrapper.cpp:348-358):
```cpp
// Set sys.path to include script_dir and config_dir/scripts
string pypath = script_dir + ":" + string(config_dir) + "/scripts";
char *pypath_c = strdup(pypath.c_str());

wchar_t *wpath = Py_DecodeLocale(pypath_c, NULL);
if (wpath) {
    PySys_SetPath(wpath);  // ❌ This REPLACES sys.path, removing stdlib!
    PyMem_RawFree(wpath);
}
```

**Solution**: Use `PySys_GetObject("path")` and append directories instead:

```cpp
// Get current sys.path
PyObject *sys_path = PySys_GetObject("path"); // Borrowed reference
if (sys_path && PyList_Check(sys_path)) {
    // Prepend script_dir
    PyObject *script_dir_str = PyUnicode_FromString(script_dir.c_str());
    PyList_Insert(sys_path, 0, script_dir_str);
    Py_DECREF(script_dir_str);
    
    // Prepend config_dir/scripts
    string scripts_path = string(config_dir) + "/scripts";
    PyObject *scripts_path_str = PyUnicode_FromString(scripts_path.c_str());
    PyList_Insert(sys_path, 0, scripts_path_str);
    Py_DECREF(scripts_path_str);
}
```

**Benefits**:
- ✅ Standard library accessible (`import json`, `import re`, etc.)
- ✅ Site-packages accessible (pip-installed packages)
- ✅ Script directory still takes precedence (inserted at position 0)

---

### 2. Accessing Python Call Counts from C++

**Current Limitation**: The Python wrapper uses hook IDs (integers) for callbacks, not arbitrary function names. You cannot directly call `print_summary()` or `get_total_calls()` from C++.

**Solutions**:

#### Option A: Add to wrapper API (Recommended for production)
Extend `wrapper.cpp` to support arbitrary function calls:

```cpp
// In wrapper.h
w_Targs *w_CallFunction(int script_id, const char *func_name, w_Targs *params);

// In wrapper.cpp
w_Targs *w_CallFunction(int id, const char *func_name, w_Targs *params) {
    w_TScript *script = w_Scripts[id];
    if (!script) return NULL;

    PyGILState_STATE gstate = PyGILState_Ensure();
    PyThreadState *old_state = PyThreadState_Get();
    PyThreadState_Swap(script->state);

    PyObject *func = PyObject_GetAttrString(script->module, func_name);
    if (!func || !PyCallable_Check(func)) {
        Py_XDECREF(func);
        PyThreadState_Swap(old_state);
        PyGILState_Release(gstate);
        return NULL;
    }

    // Build args tuple (same as w_CallHook)
    size_t arg_count = params ? (params->format ? strlen(params->format) : 0) : 0;
    PyObject *args = PyTuple_New(arg_count);
    
    // ... populate args from params ...
    
    PyObject *res = PyObject_CallObject(func, args);
    
    // ... convert res to w_Targs ...
    
    Py_DECREF(args);
    Py_DECREF(func);
    PyThreadState_Swap(old_state);
    PyGILState_Release(gstate);
    
    return result;
}
```

Then in C++ test:
```cpp
// Call Python function
w_Targs *result = py_plugin->lib_callfunction(script_id, "get_total_calls", NULL);
if (result && result->format && result->format[0] == 'l') {
    long total = result->args[0].l;
    std::cout << "Total callbacks: " << total << std::endl;
}
```

#### Option B: Use PyRun_String (Quick & dirty)
Add a helper to execute arbitrary Python code:

```cpp
void ExecutePythonCode(int script_id, const char *code) {
    // Acquire GIL, swap to script's interpreter
    // PyRun_String(code, Py_file_input, globals, locals);
    // Restore state
}

// Usage:
ExecutePythonCode(script_id, "print_summary()");
```

#### Option C: Current approach (Output parsing)
Keep using stderr output and parse it in tests. This is sufficient for testing but not ideal for production monitoring.

---

### 3. Virtual Environments for Isolated Dependencies

**Why Virtual Environments?**
- Isolate package dependencies per script
- Avoid polluting system Python
- Different scripts can use different package versions
- Easy to reproduce environments

**Implementation Strategy for Verlihub**:

#### Option 1: Pre-created venvs (Simplest)

1. **Create venv per script**:
   ```bash
   # For each script that needs packages
   cd /path/to/verlihub/scripts/
   python3 -m venv my_script_venv
   source my_script_venv/bin/activate
   pip install requests beautifulsoup4 numpy
   deactivate
   ```

2. **Modify wrapper to use venv Python**:
   ```cpp
   // In wrapper.cpp w_Load():
   // Before Py_NewInterpreter(), check for .venv file
   string venv_marker = script_dir + "/.venv";
   struct stat st;
   if (stat(venv_marker.c_str(), &st) == 0) {
       // Use venv's site-packages
       string venv_site_packages = script_dir + "/.venv/lib/python3.12/site-packages";
       // Add to sys.path
   }
   ```

3. **Script structure**:
   ```
   scripts/
   ├── monitoring_script/
   │   ├── .venv/              # Virtual environment
   │   │   └── lib/python3.12/site-packages/
   │   ├── requirements.txt    # pip freeze > requirements.txt
   │   └── monitoring.py       # The actual script
   ```

#### Option 2: Dynamic venv activation (More complex)

Modify the wrapper to detect and use venvs automatically:

```cpp
string FindVenvPython(const string &script_dir) {
    // Check for .venv/bin/python3
    string venv_python = script_dir + "/.venv/bin/python3";
    struct stat st;
    if (stat(venv_python.c_str(), &st) == 0) {
        return venv_python;
    }
    
    // Check for venv/bin/python3
    venv_python = script_dir + "/venv/bin/python3";
    if (stat(venv_python.c_str(), &st) == 0) {
        return venv_python;
    }
    
    return "";  // No venv found
}

// In w_Load():
string venv_python = FindVenvPython(script_dir);
if (!venv_python.empty()) {
    // Set VIRTUAL_ENV and update sys.path
    setenv("VIRTUAL_ENV", (script_dir + "/.venv").c_str(), 1);
    
    // Add site-packages to sys.path
    string site_packages = script_dir + "/.venv/lib/python3.12/site-packages";
    PyObject *sys_path = PySys_GetObject("path");
    if (sys_path && PyList_Check(sys_path)) {
        PyObject *sp = PyUnicode_FromString(site_packages.c_str());
        PyList_Insert(sys_path, 0, sp);
        Py_DECREF(sp);
    }
}
```

#### Option 3: Per-script requirements.txt

Most flexible approach:

```cpp
// In w_Load(), after creating sub-interpreter:
string requirements_file = script_dir + "/requirements.txt";
struct stat st;
if (stat(requirements_file.c_str(), &st) == 0) {
    // Check if packages are installed
    string marker_file = script_dir + "/.packages_installed";
    if (stat(marker_file.c_str(), &st) != 0) {
        // Install packages using pip
        log("PY: Installing dependencies from requirements.txt\n");
        
        // Execute: python3 -m pip install --user -r requirements.txt
        string cmd = "python3 -m pip install --user -r " + requirements_file;
        int ret = system(cmd.c_str());
        
        if (ret == 0) {
            // Create marker file
            ofstream marker(marker_file);
            marker << "Installed at: " << time(NULL) << "\n";
            marker.close();
        }
    }
}
```

**Recommended Approach**:

For Verlihub, I recommend **Option 1** (pre-created venvs) because:
1. ✅ No runtime overhead
2. ✅ Admin controls packages explicitly
3. ✅ No security concerns (scripts can't install arbitrary packages)
4. ✅ Reproducible environments
5. ✅ Easy to document

**Setup Instructions**:

```bash
# 1. Create script directory with venv
cd /path/to/verlihub_config/scripts/
mkdir my_monitoring_script
cd my_monitoring_script

# 2. Create virtual environment
python3 -m venv .venv

# 3. Install dependencies
source .venv/bin/activate
pip install requests beautifulsoup4
pip freeze > requirements.txt
deactivate

# 4. Create script
cat > monitoring.py << 'EOF'
import requests
import json
from bs4 import BeautifulSoup

def OnTimer():
    # Fetch external API
    response = requests.get('https://api.example.com/status')
    data = response.json()
    # ... process data ...
    return 1
EOF

# 5. Modify wrapper.cpp to add .venv/lib/pythonX.Y/site-packages to sys.path
```

---

## Testing the Fixes

### Fix 1: Test stdlib access
```python
# test_script.py
import sys
import json  # Should work now!
import re    # Should work now!

print(f"Python version: {sys.version}", file=sys.stderr)
print(f"sys.path: {sys.path}", file=sys.stderr)

def OnParsedMsgChat(*args):
    data = {"message": "test", "count": 42}
    json_str = json.dumps(data)  # Should work!
    return 1
```

### Fix 2: Test call count retrieval
```cpp
// In integration test
w_Targs *result = /* call get_total_calls */;
EXPECT_GT(result->args[0].l, 0) << "Expected callbacks to be invoked";
```

### Fix 3: Test venv isolation
```bash
# Create test venv
cd scripts/test_venv_script/
python3 -m venv .venv
source .venv/bin/activate
pip install requests==2.31.0  # Specific version
deactivate

# Script uses this specific version, not system version
```

---

## Summary

| Issue | Current State | Solution | Complexity |
|-------|--------------|----------|------------|
| Import stdlib | ❌ Broken | Fix PySys_SetPath | Easy |
| Call count access | ⚠️ Output only | Add wrapper API | Medium |
| Virtual envs | ❌ Not supported | Add venv detection | Medium |

**Next Steps**:
1. Fix `wrapper.cpp` to use `PySys_GetObject("path")` instead of `PySys_SetPath()`
2. Add `w_CallFunction()` to wrapper API for arbitrary function calls
3. Implement venv support (Option 1: pre-created venvs)
