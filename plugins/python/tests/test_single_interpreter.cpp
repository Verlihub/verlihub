/*
	Copyright (C) 2025 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	Verlihub is distributed in the hope that it will be
	useful, but without any warranty, without even the
	implied warranty of merchantability or fitness for
	a particular purpose. See the GNU General Public
	License for more details.

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include <gtest/gtest.h>
#include "../wrapper.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>

// This test is only compiled when PYTHON_SINGLE_INTERPRETER is defined
#ifdef PYTHON_SINGLE_INTERPRETER

class SingleInterpreterTest : public ::testing::Test {
protected:
	w_Tcallback callbacks[W_MAX_CALLBACKS];

	void SetUp() override {
		memset(callbacks, 0, sizeof(callbacks));
		ASSERT_TRUE(w_Begin(callbacks));
	}

	void TearDown() override {
		w_End();
	}
};

TEST_F(SingleInterpreterTest, ScriptsShareGlobalNamespace) {
	// Create first script that defines a global variable
	std::string script1_path = std::string(BUILD_DIR) + "/test_single_interp_script1.py";
	FILE* f1 = fopen(script1_path.c_str(), "w");
	ASSERT_NE(f1, nullptr);
	fprintf(f1, R"python(
# Script 1: Define a global variable
shared_data = {'message': 'Hello from script 1', 'counter': 42}

def get_shared_data():
    return shared_data

def OnUserLogin(nick):
    return 1
)python");
	fclose(f1);

	// Create second script that reads the global variable
	std::string script2_path = std::string(BUILD_DIR) + "/test_single_interp_script2.py";
	FILE* f2 = fopen(script2_path.c_str(), "w");
	ASSERT_NE(f2, nullptr);
	fprintf(f2, R"python(
# Script 2: Access the global variable from script 1
def check_shared_data():
    try:
        # In single interpreter mode, this should work!
        return shared_data['message']
    except NameError:
        return "NOT_SHARED"

def modify_shared_data():
    # Modify the shared variable
    shared_data['counter'] += 1
    return shared_data['counter']

def OnUserLogin(nick):
    return 1
)python");
	fclose(f2);

	// Load both scripts
	int id1 = w_ReserveID();
	w_Targs* args1 = w_pack("lssssls", id1, strdup(script1_path.c_str()), strdup("TestBot1"), strdup("OpChat"), strdup("."), (long)0, strdup("config"));
	ASSERT_EQ(id1, w_Load(args1)) << "Failed to load script 1";
	w_free_args(args1);
	
	int id2 = w_ReserveID();
	w_Targs* args2 = w_pack("lssssls", id2, strdup(script2_path.c_str()), strdup("TestBot2"), strdup("OpChat"), strdup("."), (long)0, strdup("config"));
	ASSERT_EQ(id2, w_Load(args2)) << "Failed to load script 2";
	w_free_args(args2);

	// Call function from script 1 to verify it works
	w_Targs* result1 = w_CallFunction(id1, "get_shared_data", w_pack(""));
	ASSERT_NE(result1, nullptr) << "Failed to call get_shared_data from script 1";
	free(result1);

	// Call function from script 2 to check if it can see script 1's data
	w_Targs* result2 = w_CallFunction(id2, "check_shared_data", w_pack(""));
	ASSERT_NE(result2, nullptr) << "Failed to call check_shared_data from script 2";
	
	char* message = nullptr;
	ASSERT_TRUE(w_unpack(result2, "s", &message));
	
	// In single interpreter mode, script 2 should see script 1's data
	std::string msg(message);
	EXPECT_EQ(msg, "Hello from script 1") 
		<< "Scripts should share global namespace in single interpreter mode";
	
	free(result2);

	// Verify script 2 can modify shared data
	w_Targs* result3 = w_CallFunction(id2, "modify_shared_data", w_pack(""));
	ASSERT_NE(result3, nullptr);
	
	long counter = 0;
	ASSERT_TRUE(w_unpack(result3, "l", &counter));
	EXPECT_EQ(counter, 43) << "Script 2 should be able to modify shared data";
	
	free(result3);

	// Clean up
	w_Unload(id1);
	w_Unload(id2);
	unlink(script1_path.c_str());
	unlink(script2_path.c_str());
}

TEST_F(SingleInterpreterTest, ScriptsShareImportedModules) {
	// Create first script that imports a module
	std::string script1_path = std::string(BUILD_DIR) + "/test_single_interp_imports1.py";
	FILE* f1 = fopen(script1_path.c_str(), "w");
	ASSERT_NE(f1, nullptr);
	fprintf(f1, R"python(
import json
import sys

# Set a custom attribute on the json module
json._script1_marker = "imported_by_script1"

def check_json_imported():
    return 'json' in sys.modules

def OnUserLogin(nick):
    return 1
)python");
	fclose(f1);

	// Create second script that checks if module is already imported
	std::string script2_path = std::string(BUILD_DIR) + "/test_single_interp_imports2.py";
	FILE* f2 = fopen(script2_path.c_str(), "w");
	ASSERT_NE(f2, nullptr);
	fprintf(f2, R"python(
import sys

def check_json_already_loaded():
    # In single interpreter mode, json should already be in sys.modules
    return 'json' in sys.modules

def check_json_marker():
    # Can we see the marker set by script1?
    try:
        import json
        return hasattr(json, '_script1_marker')
    except:
        return False

def OnUserLogin(nick):
    return 1
)python");
	fclose(f2);

	// Load first script
	int id1 = w_ReserveID();
	w_Targs* args1 = w_pack("lssssls", id1, strdup(script1_path.c_str()), strdup("TestBot"), strdup("OpChat"), strdup("."), (long)0, strdup("config"));
	ASSERT_EQ(id1, w_Load(args1));
	w_free_args(args1);

	// Verify json is imported
	w_Targs* result1 = w_CallFunction(id1, "check_json_imported", w_pack(""));
	ASSERT_NE(result1, nullptr);
	long is_imported = 0;
	ASSERT_TRUE(w_unpack(result1, "l", &is_imported));
	EXPECT_EQ(is_imported, 1);
	free(result1);

	// Load second script
	int id2 = w_ReserveID();
	w_Targs* args2 = w_pack("lssssls", id2, strdup(script2_path.c_str()), strdup("TestBot"), strdup("OpChat"), strdup("."), (long)0, strdup("config"));
	ASSERT_EQ(id2, w_Load(args2));
	w_free_args(args2);

	// Check if script2 can see that json is already loaded
	w_Targs* result2 = w_CallFunction(id2, "check_json_already_loaded", w_pack(""));
	ASSERT_NE(result2, nullptr);
	long already_loaded = 0;
	ASSERT_TRUE(w_unpack(result2, "l", &already_loaded));
	EXPECT_EQ(already_loaded, 1) 
		<< "In single interpreter mode, modules should be shared";
	free(result2);

	// Check if script2 can see the marker from script1
	w_Targs* result3 = w_CallFunction(id2, "check_json_marker", w_pack(""));
	ASSERT_NE(result3, nullptr);
	long has_marker = 0;
	ASSERT_TRUE(w_unpack(result3, "l", &has_marker));
	EXPECT_EQ(has_marker, 1)
		<< "In single interpreter mode, module modifications should be visible";
	free(result3);

	// Clean up
	w_Unload(id1);
	w_Unload(id2);
	unlink(script1_path.c_str());
	unlink(script2_path.c_str());
}

TEST_F(SingleInterpreterTest, ThreadingWorksAndCleansUp) {
	// Create a script with threading
	std::string script_path = std::string(BUILD_DIR) + "/test_single_interp_threading.py";
	FILE* f = fopen(script_path.c_str(), "w");
	ASSERT_NE(f, nullptr);
	fprintf(f, R"python(
import threading
import time

result = {'thread_ran': False}

def worker():
    time.sleep(0.1)
    result['thread_ran'] = True

def start_thread():
    t = threading.Thread(target=worker)
    t.start()
    t.join()  # Wait for thread to complete
    return result['thread_ran']

def OnUserLogin(nick):
    return 1
)python");
	fclose(f);

	// Load script
	int id = w_ReserveID();
	w_Targs* args = w_pack("lssssls", id, strdup(script_path.c_str()), strdup("TestBot"), strdup("OpChat"), strdup("."), (long)0, strdup("config"));
	ASSERT_EQ(id, w_Load(args));
	w_free_args(args);

	// Run the threading test
	w_Targs* result = w_CallFunction(id, "start_thread", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	long thread_ran = 0;
	ASSERT_TRUE(w_unpack(result, "l", &thread_ran));
	EXPECT_EQ(thread_ran, 1) << "Thread should execute successfully";
	free(result);

	// Unload - in single interpreter mode, this should NOT crash
	// even though threading was used
	w_Unload(id);
	
	unlink(script_path.c_str());
	
	// If we got here without crashing, the test passed
	SUCCEED() << "Threading cleanup succeeded in single interpreter mode";
}

#endif // PYTHON_SINGLE_INTERPRETER
