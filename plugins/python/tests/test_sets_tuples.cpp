/*
	Copyright (C) 2025 Verlihub Team

	Test suite for Python Sets and Tuples marshaling
	Tests bidirectional conversion of sets and tuples between C++ and Python
*/

#include <gtest/gtest.h>
#include "../wrapper.h"
#include "../json_marshal.h"
#include <string>
#include <cstring>
#include <fstream>

using namespace nVerliHub::nPythonPlugin;

class SetsTuplesTest : public ::testing::Test {
protected:
	static int script_id;
	static std::string script_path;
	static bool initialized;

	static void SetUpTestSuite() {
		// Load test script once for all tests
		script_path = std::string(BUILD_DIR) + "/test_script_sets_tuples.py";
		
		// Initialize wrapper
		w_Tcallback callbacks[W_MAX_HOOKS] = {nullptr};
		ASSERT_EQ(w_Begin(callbacks), 1) << "Failed to initialize Python";
		
		// Create test script
		std::ofstream script(script_path);
		script << R"python(
#!/usr/bin/env python3
import vh

# Track function calls
call_counts = {}

def count_call(func_name):
    global call_counts
    call_counts[func_name] = call_counts.get(func_name, 0) + 1

# Set tests
def return_simple_set():
    count_call('return_simple_set')
    return {1, 2, 3, 4, 5}

def return_string_set():
    count_call('return_string_set')
    return {'apple', 'banana', 'cherry'}

def return_mixed_set():
    count_call('return_mixed_set')
    # Sets must have hashable items - strings and numbers
    return {1, 'two', 3.0, 'four'}

def process_set(items):
    count_call('process_set')
    # Python receives JSON array, convert to set-like operations
    if isinstance(items, (list, tuple, set)):
        return len(items)
    return 0

# Tuple tests
def return_simple_tuple():
    count_call('return_simple_tuple')
    return (1, 2, 3, 4, 5)

def return_string_tuple():
    count_call('return_string_tuple')
    return ('alpha', 'beta', 'gamma')

def return_mixed_tuple():
    count_call('return_mixed_tuple')
    return (1, 'two', 3.0, True, None)

def return_nested_tuple():
    count_call('return_nested_tuple')
    return (1, (2, 3), (4, (5, 6)))

def process_tuple(items):
    count_call('process_tuple')
    if isinstance(items, (list, tuple)) and len(items) >= 2:
        return items[0] + items[-1]  # First + last element
    return 0

# Combined tests
def return_set_and_tuple():
    count_call('return_set_and_tuple')
    return ({'a', 'b', 'c'}, ('x', 'y', 'z'))

def process_mixed_containers(my_list, my_tuple, my_set):
    count_call('process_mixed_containers')
    return {
        'list_len': len(my_list),
        'tuple_len': len(my_tuple),
        'set_len': len(my_set),
        'tuple_first': my_tuple[0] if my_tuple else None,
        'set_has_alpha': 'alpha' in my_set
    }

# Stats
def get_call_stats():
    return call_counts
)python";
		script.close();
		
		// Reserve ID and load script
		script_id = w_ReserveID();
		ASSERT_GE(script_id, 0) << "Failed to reserve script ID";
		
		// Load script
		w_Targs* load_args = w_pack("lssssls", 
			(long)script_id, 
			strdup(script_path.c_str()),
			strdup("TestBot"), 
			strdup("OpChat"), 
			strdup("."),
			(long)123, 
			strdup("test_config"));
		ASSERT_NE(load_args, nullptr) << "Failed to pack load arguments";
		
		int load_result = w_Load(load_args);
		w_free_args(load_args);
		
		ASSERT_EQ(script_id, load_result) << "Failed to load test script";
		initialized = true;
	}

	static void TearDownTestSuite() {
		if (initialized && script_id >= 0) {
			w_Unload(script_id);
		}
		w_End();
		
		// Clean up test script file
		if (!script_path.empty()) {
			unlink(script_path.c_str());
		}
	}
};

// Initialize static members
int SetsTuplesTest::script_id = -1;
std::string SetsTuplesTest::script_path;
bool SetsTuplesTest::initialized = false;

// ===== Set Tests =====

TEST_F(SetsTuplesTest, PythonReturnsSimpleSet) {
	w_Targs *result = w_CallFunction(script_id, "return_simple_set", nullptr);
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	ASSERT_NE(result->format, nullptr) << "Result format is NULL";
	ASSERT_EQ(result->format[0], 'D') << "Expected 'D' format (JSON)";
	
	// Unpack JSON
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str)) << "Failed to unpack JSON";
	ASSERT_NE(json_str, nullptr) << "JSON string is NULL";
	
#ifdef HAVE_RAPIDJSON
	// Parse and verify it's an array (sets serialize as arrays)
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val)) << "Failed to parse JSON";
	ASSERT_EQ(val.type, JsonType::ARRAY) << "Expected JSON array";
	ASSERT_EQ(val.array_val.size(), 5) << "Expected 5 elements";
	
	// Verify values (may be in any order since it's a set)
	std::set<int64_t> values;
	for (const auto& elem : val.array_val) {
		ASSERT_EQ(elem.type, JsonType::INT);
		values.insert(elem.int_val);
	}
	EXPECT_EQ(values, std::set<int64_t>({1, 2, 3, 4, 5}));
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, PythonReturnsStringSet) {
	w_Targs *result = w_CallFunction(script_id, "return_string_set", nullptr);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.type, JsonType::ARRAY);
	ASSERT_EQ(val.array_val.size(), 3);
	
	std::set<std::string> values;
	for (const auto& elem : val.array_val) {
		ASSERT_EQ(elem.type, JsonType::STRING);
		values.insert(elem.string_val);
	}
	EXPECT_EQ(values, std::set<std::string>({"apple", "banana", "cherry"}));
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, CppSendsSetToPython) {
	// Send a set as JSON array
	const char *json_set = R"([1, 2, 3, 4])";
	w_Targs *args = w_pack("D", strdup(json_set));
	ASSERT_NE(args, nullptr);
	
	w_Targs *result = w_CallFunction(script_id, "process_set", args);
	w_free_args(args);
	
	ASSERT_NE(result, nullptr);
	
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count));
	EXPECT_EQ(count, 4);
	
	w_free_args(result);
}

// ===== Tuple Tests =====

TEST_F(SetsTuplesTest, PythonReturnsSimpleTuple) {
	w_Targs *result = w_CallFunction(script_id, "return_simple_tuple", nullptr);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.type, JsonType::ARRAY) << "Tuples serialize as arrays";
	ASSERT_EQ(val.array_val.size(), 5);
	
	// Verify order is preserved (tuples maintain order)
	EXPECT_EQ(val.array_val[0].int_val, 1);
	EXPECT_EQ(val.array_val[1].int_val, 2);
	EXPECT_EQ(val.array_val[2].int_val, 3);
	EXPECT_EQ(val.array_val[3].int_val, 4);
	EXPECT_EQ(val.array_val[4].int_val, 5);
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, PythonReturnsStringTuple) {
	w_Targs *result = w_CallFunction(script_id, "return_string_tuple", nullptr);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.array_val.size(), 3);
	
	EXPECT_EQ(val.array_val[0].string_val, "alpha");
	EXPECT_EQ(val.array_val[1].string_val, "beta");
	EXPECT_EQ(val.array_val[2].string_val, "gamma");
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, PythonReturnsMixedTuple) {
	w_Targs *result = w_CallFunction(script_id, "return_mixed_tuple", nullptr);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.array_val.size(), 5);
	
	EXPECT_EQ(val.array_val[0].type, JsonType::INT);
	EXPECT_EQ(val.array_val[0].int_val, 1);
	
	EXPECT_EQ(val.array_val[1].type, JsonType::STRING);
	EXPECT_EQ(val.array_val[1].string_val, "two");
	
	EXPECT_EQ(val.array_val[2].type, JsonType::DOUBLE);
	EXPECT_DOUBLE_EQ(val.array_val[2].double_val, 3.0);
	
	EXPECT_EQ(val.array_val[3].type, JsonType::BOOL);
	EXPECT_EQ(val.array_val[3].bool_val, true);
	
	EXPECT_EQ(val.array_val[4].type, JsonType::NULL_TYPE);
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, PythonReturnsNestedTuple) {
	w_Targs *result = w_CallFunction(script_id, "return_nested_tuple", nullptr);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.array_val.size(), 3);
	
	// First element: 1
	EXPECT_EQ(val.array_val[0].int_val, 1);
	
	// Second element: (2, 3)
	EXPECT_EQ(val.array_val[1].type, JsonType::ARRAY);
	EXPECT_EQ(val.array_val[1].array_val.size(), 2);
	
	// Third element: (4, (5, 6))
	EXPECT_EQ(val.array_val[2].type, JsonType::ARRAY);
	EXPECT_EQ(val.array_val[2].array_val.size(), 2);
	EXPECT_EQ(val.array_val[2].array_val[1].type, JsonType::ARRAY);
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, CppSendsTupleToPython) {
	// Send a tuple as JSON array (will be received as list in Python, but can be used)
	const char *json_tuple = R"([10, 20])";
	w_Targs *args = w_pack("D", strdup(json_tuple));
	ASSERT_NE(args, nullptr);
	
	w_Targs *result = w_CallFunction(script_id, "process_tuple", args);
	w_free_args(args);
	
	ASSERT_NE(result, nullptr);
	
	long sum;
	ASSERT_TRUE(w_unpack(result, "l", &sum));
	EXPECT_EQ(sum, 30);  // 10 + 20
	
	w_free_args(result);
}

// ===== Combined Tests =====

TEST_F(SetsTuplesTest, MixedContainers) {
	// Prepare arguments: list, tuple, set
	const char *json_list = R"(["item1", "item2", "item3"])";
	const char *json_tuple = R"(["first", "second"])";
	const char *json_set = R"(["alpha", "beta", "gamma"])";
	
	w_Targs *args = w_pack("DDD", 
		strdup(json_list),
		strdup(json_tuple),
		strdup(json_set));
	ASSERT_NE(args, nullptr);
	
	w_Targs *result = w_CallFunction(script_id, "process_mixed_containers", args);
	w_free_args(args);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	EXPECT_EQ(val.object_val["list_len"].int_val, 3);
	EXPECT_EQ(val.object_val["tuple_len"].int_val, 2);
	EXPECT_EQ(val.object_val["set_len"].int_val, 3);
	EXPECT_EQ(val.object_val["tuple_first"].string_val, "first");
	EXPECT_EQ(val.object_val["set_has_alpha"].bool_val, true);
#endif
	
	w_free_args(result);
}

TEST_F(SetsTuplesTest, GetCallStatistics) {
	w_Targs *result = w_CallFunction(script_id, "get_call_stats", nullptr);
	
	ASSERT_NE(result, nullptr);
	ASSERT_EQ(result->format[0], 'D');
	
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str));
	
#ifdef HAVE_RAPIDJSON
	JsonValue val;
	ASSERT_TRUE(parseJson(json_str, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	// Verify that all test functions were called
	EXPECT_GT(val.object_val["return_simple_set"].int_val, 0);
	EXPECT_GT(val.object_val["return_string_set"].int_val, 0);
	EXPECT_GT(val.object_val["return_simple_tuple"].int_val, 0);
	EXPECT_GT(val.object_val["return_nested_tuple"].int_val, 0);
	EXPECT_GT(val.object_val["process_mixed_containers"].int_val, 0);
#endif
	
	w_free_args(result);
}
