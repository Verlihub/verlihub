# Python Plugin Testing Notes

## Test Suite Overview

The Python plugin has three types of tests:

1. **test_python_wrapper** - Unit test for GIL wrapper
   - Tests basic GIL acquire/release
   - No Verlihub dependencies
   - Runs in ~0.06s

2. **test_python_wrapper_stress** - Stress test for GIL wrapper
   - 100K iterations to catch race conditions
   - Simulates concurrent callback load
   - Runs in ~17s

3. **test_python_plugin_integration** - Full integration test
   - Tests message parsing → Python callbacks
   - **REQUIRES MySQL server**
   - Runs 100K message iterations

## MySQL Requirement

The integration test **requires MySQL**. Here's why:

### Technical Analysis

The plugin is tightly coupled to server infrastructure:

```cpp
void cpiPython::OnLoad(cServerDC *server) {
    mQuery = new cQuery(server->mMySQL);  // Line 92 - requires MySQL
    botname = server->mC.hub_security;    // Line 96 - config from DB
    // ...
}
```

And `cServerDC` constructor connects to MySQL during initialization (line 65 of cserverdc.cpp).

Verlihub auto-creates all required tables dynamically using `CreateTable()` methods. No manual schema initialization is needed.

## Running the Integration Test

### MySQL Setup

**Install MySQL server:**
```bash
sudo apt-get update
sudo apt-get install mysql-server

# Start MySQL service
sudo systemctl start mysql
sudo systemctl enable mysql

# Verify MySQL is running
sudo systemctl status mysql
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

### Running Tests

```bash
cd /mnt/ramdrive/verlihub-build

# Fast wrapper tests (no MySQL needed)
./plugins/python/test_python_wrapper
./plugins/python/test_python_wrapper_stress

# Integration test (requires MySQL)
./plugins/python/test_python_plugin_integration
```

### Environment Variables

You can override MySQL connection settings:

**Environment variables used:**
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

## Testing the GIL Race Condition

The stress tests are specifically designed to catch the GIL race condition that causes crashes "after running for a while":

1. **test_python_wrapper_stress**: 100K wrapper calls without message parsing
2. **test_python_plugin_integration**: 100K messages through full pipeline

Both exercise:
- Rapid GIL acquire/release cycles
- Concurrent callback invocations
- Thread state management

If the GIL wrapper has issues, these tests should expose them.

## Build Configuration

The CMakeLists.txt creates **two libraries** for the plugin:

1. `libpython_pi.so` - MODULE library (loadable plugin)
   - Used by Verlihub at runtime
   - Cannot be linked into tests

2. `libpython_pi_core.so` - SHARED library (linkable)
   - Used by test executables
   - Contains same code as MODULE library

This dual-library approach solves: "MODULE libraries cannot be linked into executables"

## Future Improvements

### Database Abstraction Layer

To make testing easier without MySQL, we could:

1. **Create database interface:**
```cpp
class IDatabaseConnection {
    virtual bool Query(const std::string& sql) = 0;
    virtual ResultSet GetResult() = 0;
};

class MockDatabase : public IDatabaseConnection {
    // In-memory implementation
};
```

2. **Refactor cMySQL to use interface**
3. **Update 100+ database queries throughout codebase**

**Effort Estimate:** Several weeks of refactoring

**Risk:** High - touches core infrastructure

**Benefit:** Testable without MySQL, better architecture

**Status:** Not prioritized - local MySQL works fine for development

## Summary

- ✅ Wrapper tests work without MySQL
- ⚠️ Integration test **requires MySQL** (architectural limitation)
- ✅ All tests compile and build successfully
- ✅ MySQL auto-creates schema (no manual setup needed)
- ✅ Environment variables support flexible configurations
