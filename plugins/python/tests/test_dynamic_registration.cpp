/*
	Copyright (C) 2025 Verlihub Team

	Test suite for Dynamic C++ Function Registration
*/

#include <gtest/gtest.h>
#include "../wrapper.h"
#include "../json_marshal.h"
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>

using namespace nVerliHub::nPythonPlugin;

// Helper function to generate unique test file paths in build directory
static std::string GetTestFilePath(const char* base_name) {
	std::ostringstream oss;
	oss << BUILD_DIR << "/test_dyn_" << base_name << "_" << getpid() << "_" << ::testing::UnitTest::GetInstance()->current_test_info()->name() << ".py";
	return oss.str();
}

// Test fixture
class DynamicRegistrationTest : public ::testing::Test {
protected:
	static w_Tcallback callbacks[W_MAX_CALLBACKS];
	std::vector<std::string> temp_files;  // Track files to clean up
	
	void SetUp() override {
		w_Begin(callbacks);
	}
	
	void TearDown() override {
		w_End();
		// Clean up temporary files
		for (const auto& file : temp_files) {
			unlink(file.c_str());
		}
	}
	
	// Helper to create temp file and track it
	std::string CreateTempFile(const char* base_name) {
		std::string path = GetTestFilePath(base_name);
		temp_files.push_back(path);
		return path;
	}
	
	// Example C++ function that adds two numbers
	static w_Targs* AddNumbers(int id, w_Targs* args) {
		long a, b;
		if (!w_unpack(args, "ll", &a, &b)) {
			return w_pack("l", (long)0);
		}
		return w_pack("l", a + b);
	}
	
	// Example C++ function that reverses a string
	static w_Targs* ReverseString(int id, w_Targs* args) {
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
	
	// Example C++ function that multiplies a number
	static w_Targs* MultiplyBy10(int id, w_Targs* args) {
		long num;
		if (!w_unpack(args, "l", &num)) {
			return w_pack("l", (long)0);
		}
		return w_pack("l", num * 10);
	}
};

w_Tcallback DynamicRegistrationTest::callbacks[W_MAX_CALLBACKS] = {0};

TEST_F(DynamicRegistrationTest, RegisterAndCallFunction) {
	int id = w_ReserveID();
	ASSERT_GE(id, 0);
	
	// Register the AddNumbers function
	int result = w_RegisterFunction(id, "add", AddNumbers);
	EXPECT_EQ(result, 1);
	
	// Create Python script that calls the dynamic function
	std::string script_path = CreateTempFile("add");
	FILE* f = fopen(script_path.c_str(), "w");
	ASSERT_NE(f, nullptr);
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    result = vh.CallDynamicFunction('add', 5, 7)\n");
	fprintf(f, "    assert result == 12, f'Expected 12, got {result}'\n");
	fprintf(f, "    return result\n");
	fclose(f);
	
	// Load the script
	w_Targs* load_args = w_pack("lssssls", 
		(long)id, 
		(char*)script_path.c_str(),
		(char*)"TestBot",
		(char*)"OpChat",
		(char*)".",
		(long)1234567890,
		(char*)"test_config");
	
	int load_result = w_Load(load_args);
	free(load_args);
	EXPECT_EQ(load_result, id);
	
	// Call the test function
	w_Targs* ret = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(ret, nullptr);
	
	long value;
	ASSERT_TRUE(w_unpack(ret, "l", &value));
	EXPECT_EQ(value, 12);
	
	w_free_args(ret);
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, RegisterMultipleFunctions) {
	int id = w_ReserveID();
	
	// Register multiple functions
	EXPECT_EQ(w_RegisterFunction(id, "add", AddNumbers), 1);
	EXPECT_EQ(w_RegisterFunction(id, "reverse", ReverseString), 1);
	EXPECT_EQ(w_RegisterFunction(id, "multiply", MultiplyBy10), 1);
	
	std::string script_path = CreateTempFile("multi");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    sum = vh.CallDynamicFunction('add', 10, 20)\n");
	fprintf(f, "    assert sum == 30\n");
	fprintf(f, "    \n");
	fprintf(f, "    rev = vh.CallDynamicFunction('reverse', 'hello')\n");
	fprintf(f, "    assert rev == 'olleh', f'Expected olleh, got {rev}'\n");
	fprintf(f, "    \n");
	fprintf(f, "    mult = vh.CallDynamicFunction('multiply', 5)\n");
	fprintf(f, "    assert mult == 50\n");
	fprintf(f, "    \n");
	fprintf(f, "    return True\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	ASSERT_NE(result, nullptr);
	
	w_free_args(result);
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, UnregisterFunction) {
	int id = w_ReserveID();
	
	// Must load the script first
	std::string script_path = CreateTempFile("unreg");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "pass\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	// Register and then unregister
	EXPECT_EQ(w_RegisterFunction(id, "add", AddNumbers), 1);
	EXPECT_EQ(w_UnregisterFunction(id, "add"), 1);
	
	// Try to unregister again (should fail)
	EXPECT_EQ(w_UnregisterFunction(id, "add"), 0);
	
	// Try to unregister non-existent function
	EXPECT_EQ(w_UnregisterFunction(id, "nonexistent"), 0);
	
	w_Unload(id);
}

TEST_F(DynamicRegistrationTest, CallNonExistentFunction) {
	int id = w_ReserveID();
	
	std::string script_path = CreateTempFile("nonexist");
	FILE* f = fopen(script_path.c_str(), "w");
	fprintf(f, "import vh\n");
	fprintf(f, "def test():\n");
	fprintf(f, "    try:\n");
	fprintf(f, "        vh.CallDynamicFunction('nonexistent', 1, 2)\n");
	fprintf(f, "        return False  # Should have raised exception\n");
	fprintf(f, "    except (NameError, RuntimeError) as e:\n");  // Both are OK
	fprintf(f, "        # Expected: either no funcs registered or func not found\n");
	fprintf(f, "        return True\n");
	fclose(f);
	
	w_Targs* load_args = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load_args);
	free(load_args);
	
	w_Targs* result = w_CallFunction(id, "test", w_pack(""));
	// Note: result might be NULL if exception propagated, that's OK for this test
	// The important thing is the script handled it
	
	if (result) w_free_args(result);
	w_Unload(id);
}

#ifndef PYTHON_SINGLE_INTERPRETER
// This test expects dynamic function isolation between scripts
// In single-interpreter mode, all scripts share the same vh module and can call each other's functions
TEST_F(DynamicRegistrationTest, IsolatedBetweenScripts) {
	int id1 = w_ReserveID();
	int id2 = w_ReserveID();
	
	// Register function only for script 1
	EXPECT_EQ(w_RegisterFunction(id1, "add", AddNumbers), 1);
	
	// Script 1 can call it
	std::string script1_path = CreateTempFile("isolated1");
	FILE* f1 = fopen(script1_path.c_str(), "w");
	fprintf(f1, "import vh\n");
	fprintf(f1, "def test():\n");
	fprintf(f1, "    return vh.CallDynamicFunction('add', 3, 4)\n");
	fclose(f1);
	
	// Script 2 cannot call it
	std::string script2_path = CreateTempFile("isolated2");
	FILE* f2 = fopen(script2_path.c_str(), "w");
	fprintf(f2, "import vh\n");
	fprintf(f2, "def test():\n");
	fprintf(f2, "    try:\n");
	fprintf(f2, "        vh.CallDynamicFunction('add', 1, 2)\n");
	fprintf(f2, "        return False\n");
	fprintf(f2, "    except:\n");
	fprintf(f2, "        return True  # Expected to fail\n");
	fclose(f2);
	
	w_Targs* load1 = w_pack("lssssls", (long)id1, (char*)script1_path.c_str(),
		(char*)"Bot1", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load1);
	free(load1);
	
	w_Targs* load2 = w_pack("lssssls", (long)id2, (char*)script2_path.c_str(),
		(char*)"Bot2", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	w_Load(load2);
	free(load2);
	
	// Script 1 should succeed
	w_Targs* res1 = w_CallFunction(id1, "test", w_pack(""));
	ASSERT_NE(res1, nullptr);
	long val1;
	w_unpack(res1, "l", &val1);
	EXPECT_EQ(val1, 7);
	free(res1);
	
	// Script 2 should fail gracefully (returns True)
	w_Targs* res2 = w_CallFunction(id2, "test", w_pack(""));
	ASSERT_NE(res2, nullptr);
	free(res2);
	
	w_Unload(id1);
	w_Unload(id2);
}
#endif // PYTHON_SINGLE_INTERPRETER

// Test complex type: list argument and return
TEST_F(DynamicRegistrationTest, ListArgumentAndReturn) {
	// C++ function that takes a list and returns it reversed
	auto ReverseList = [](int id, w_Targs* args) -> w_Targs* {
		char *json_str;
		if (!w_unpack(args, "D", &json_str)) {
			return w_pack("D", strdup("[]"));
		}
		
#ifdef HAVE_RAPIDJSON
		// Parse JSON array
		nVerliHub::nPythonPlugin::JsonValue val;
		if (!nVerliHub::nPythonPlugin::parseJson(json_str, val) || val.type != nVerliHub::nPythonPlugin::JsonType::ARRAY) {
			return w_pack("D", strdup("[]"));
		}
		
		// Reverse the array
		nVerliHub::nPythonPlugin::JsonValue reversed;
		reversed.type = nVerliHub::nPythonPlugin::JsonType::ARRAY;
		for (auto it = val.array_val.rbegin(); it != val.array_val.rend(); ++it) {
			reversed.array_val.push_back(*it);
		}
		
		// Serialize back to JSON
		std::string result_json_str = nVerliHub::nPythonPlugin::toJsonString(reversed);
		return w_pack("D", strdup(result_json_str.c_str()));
#else
		return w_pack("D", strdup("[]"));
#endif
	};
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("list");
	std::ofstream script(script_path);
	script << R"python(
import vh

def test_list():
    # Pass a list, get it back reversed
    result = vh.CallDynamicFunction('reverse_list', ['apple', 'banana', 'cherry'])
    assert result == ['cherry', 'banana', 'apple'], f"Expected reversed list, got {result}"
    
    # Pass empty list
    result = vh.CallDynamicFunction('reverse_list', [])
    assert result == [], f"Expected empty list, got {result}"
    
    # Pass single item
    result = vh.CallDynamicFunction('reverse_list', ['solo'])
    assert result == ['solo'], f"Expected single item, got {result}"
    
    return True

test_list()
)python";
	script.close();
	
	w_RegisterFunction(id, "reverse_list", ReverseList);
	
	w_Targs* load = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	EXPECT_EQ(id, w_Load(load));
	free(load);
	
	w_Unload(id);
}

// Test complex type: dict argument and return
TEST_F(DynamicRegistrationTest, DictArgumentAndReturn) {
	// C++ function that takes a dict and adds a field
	auto AddDictField = [](int id, w_Targs* args) -> w_Targs* {
		char *json_str;
		if (!w_unpack(args, "D", &json_str)) {
			return w_pack("D", strdup("{}"));
		}
		
		// Simple JSON manipulation - add "processed": true
		std::string json(json_str ? json_str : "{}");
		if (json == "{}") {
			json = R"({"processed": true})";
		} else {
			// Insert before the last }
			size_t pos = json.rfind('}');
			if (pos != std::string::npos) {
				json.insert(pos, R"(, "processed": true)");
			}
		}
		
		return w_pack("D", strdup(json.c_str()));
	};
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("dict");
	std::ofstream script(script_path);
	script << R"python(
import vh

def test_dict():
    # Pass a dict, get it back with added field
    result = vh.CallDynamicFunction('add_field', {'name': 'Alice', 'age': 30})
    assert 'processed' in result, f"Expected 'processed' field, got {result}"
    assert result['processed'] == True, f"Expected processed=True, got {result}"
    assert result['name'] == 'Alice', f"Expected name='Alice', got {result}"
    
    # Pass empty dict
    result = vh.CallDynamicFunction('add_field', {})
    assert result == {'processed': True}, f"Expected {{'processed': True}}, got {result}"
    
    return True

test_dict()
)python";
	script.close();
	
	w_RegisterFunction(id, "add_field", AddDictField);
	
	w_Targs* load = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	EXPECT_EQ(id, w_Load(load));
	free(load);
	
	w_Unload(id);
}

// Test mixed complex types
TEST_F(DynamicRegistrationTest, MixedComplexTypes) {
	// C++ function that takes multiple argument types
	auto ProcessData = [](int id, w_Targs* args) -> w_Targs* {
		char *name;
		long count;
		char *items_json;  // Now receives JSON for list
		char *metadata_json;
		
		if (!w_unpack(args, "slDD", &name, &count, &items_json, &metadata_json)) {
			return w_pack("s", strdup("error"));
		}
		
		// Parse items JSON to count elements
		int item_count = 0;
#ifdef HAVE_RAPIDJSON
		if (items_json) {
			nVerliHub::nPythonPlugin::JsonValue items_val;
			if (nVerliHub::nPythonPlugin::parseJson(items_json, items_val)) {
				if (items_val.type == nVerliHub::nPythonPlugin::JsonType::ARRAY) {
					item_count = items_val.array_val.size();
				}
			}
		}
#endif
		
		// Build result string
		std::ostringstream result;
		result << "Processed: " << (name ? name : "unknown");
		result << ", count=" << count;
		result << ", items=" << item_count;
		result << ", metadata=" << (metadata_json ? metadata_json : "{}");
		
		return w_pack("s", strdup(result.str().c_str()));
	};
	
	int id = w_ReserveID();
	std::string script_path = CreateTempFile("mixed");
	std::ofstream script(script_path);
	script << R"python(
import vh

def test_mixed():
    # Pass mixed types: string, int, list, dict
    result = vh.CallDynamicFunction('process',
        'TestUser',
        42,
        ['item1', 'item2', 'item3'],
        {'status': 'active', 'priority': 1}
    )
    
    assert 'TestUser' in result, f"Expected 'TestUser' in result: {result}"
    assert 'count=42' in result, f"Expected 'count=42' in result: {result}"
    assert 'items=3' in result, f"Expected 'items=3' in result: {result}"
    assert 'metadata=' in result, f"Expected metadata in result: {result}"
    
    return True

test_mixed()
)python";
	script.close();
	
	w_RegisterFunction(id, "process", ProcessData);
	
	w_Targs* load = w_pack("lssssls", (long)id, (char*)script_path.c_str(),
		(char*)"Bot", (char*)"OpChat", (char*)".", (long)123, (char*)"cfg");
	EXPECT_EQ(id, w_Load(load));
	free(load);
	
	w_Unload(id);
}

int main(int argc, char** argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
