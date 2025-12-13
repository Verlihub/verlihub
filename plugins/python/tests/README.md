# Python Plugin Testing

## Quick Start

```bash
cd /mnt/ramdrive/verlihub-build

# Run wrapper tests (recommended)
./plugins/python/test_python_wrapper
./plugins/python/test_python_wrapper_stress
```

## Test Types

1. **test_python_wrapper** - GIL wrapper unit tests
   - Tests basic GIL acquire/release
   - No dependencies
   - Runs in ~0.06s

2. **test_python_wrapper_stress** - GIL stress test ⭐
   - **100K iterations to catch race conditions**
   - Exercises same GIL code paths as production
   - Tests concurrent callback load
   - Runs in ~17s
   - **This is the main test for GIL race conditions**

3. **test_python_plugin_integration** - Full server integration (DISABLED)
   - Requires full Verlihub server initialization
   - Server constructor blocks on socket binding
   - Not practical for automated testing
   - Use wrapper stress test instead

## Running Tests

```bash
cd /mnt/ramdrive/verlihub-build

# Quick unit test
./plugins/python/test_python_wrapper

# Stress test (recommended for GIL debugging)
./plugins/python/test_python_wrapper_stress
```

## Why Wrapper Stress Test is Sufficient

The `test_python_wrapper_stress` test:

## Why Wrapper Stress Test is Sufficient

The `test_python_wrapper_stress` test:
- Calls Python functions 100,000 times
- Each call acquires/releases the GIL
- Tests the exact same code path as production callbacks
- Exercises `w_Py_INCREF`, `w_Py_DECREF`, `w_CallFunction`
- Runs without full server overhead
- **Catches GIL race conditions reliably**

This is the same code that runs when Verlihub calls Python callbacks for messages, connections, etc. If there's a GIL threading issue, this test will find it.

## Debugging GIL Issues

If you encounter crashes or hangs:

```bash
# Run under GDB
gdb --args ./plugins/python/test_python_wrapper_stress

# Run with Python debug mode
PYTHONDEVMODE=1 ./plugins/python/test_python_wrapper_stress

# Check for memory issues
valgrind --leak-check=full ./plugins/python/test_python_wrapper_stress
```

## More Information

See [TESTING_NOTES.md](TESTING_NOTES.md) for detailed technical documentation.
