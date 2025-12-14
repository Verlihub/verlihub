/*
	Phase 3 & 4: Advanced Type Marshaling Tests
	Tests list, dict, and PyObject* marshaling between C++ and Python
	
	Copyright (C) 2025 Verlihub Team, info at verlihub dot net
*/

#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include "../wrapper.h"

class AdvancedTypesTest : public ::testing::Test {
protected:
	static int script_id;
	static std::string script_path;
	static bool initialized;

	static void SetUpTestSuite() {
		// Load test script once for all tests
		script_path = std::string(__FILE__);
		size_t pos = script_path.find_last_of("/\\");
		if (pos != std::string::npos) {
			script_path = script_path.substr(0, pos + 1);
		}
		script_path += "test_script_advanced_types.py";
		
		// Initialize wrapper
		w_Tcallback callbacks[W_MAX_HOOKS] = {nullptr};
		ASSERT_EQ(w_Begin(callbacks), 1) << "Failed to initialize Python";
		
		// Reserve ID and load script
		script_id = w_ReserveID();
		ASSERT_GE(script_id, 0) << "Failed to reserve script ID";
		
		w_Targs *load_args = w_pack("lssssls", 
			(long)script_id,
			script_path.c_str(),
			"TestBot",
			"OpChat",
			"/tmp",
			(long)0,
			"test_advanced");
		
		ASSERT_NE(load_args, nullptr) << "Failed to pack load arguments";
		int load_result = w_Load(load_args);
		free(load_args);
		
		ASSERT_EQ(load_result, 0) << "Failed to load Python script: " << script_path;
		initialized = true;
	}

	static void TearDownTestSuite() {
		if (script_id >= 0 && initialized) {
			w_Unload(script_id);
		}
		w_End();
	}
};

// Initialize static members
int AdvancedTypesTest::script_id = -1;
std::string AdvancedTypesTest::script_path;
bool AdvancedTypesTest::initialized = false;

// ===== Phase 3: List Marshaling Tests =====

TEST_F(AdvancedTypesTest, PythonReturnsStringList) {
	// Call Python function that returns list of strings
	w_Targs *result = w_CallFunction(script_id, "return_string_list", nullptr);
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	ASSERT_NE(result->format, nullptr) << "Result format is NULL";
	ASSERT_EQ(result->format[0], 'L') << "Expected 'L' format, got: " << result->format;
	
	// Unpack the list
	char **str_list;
	ASSERT_TRUE(w_unpack(result, "L", &str_list)) << "Failed to unpack list";
	ASSERT_NE(str_list, nullptr) << "List is NULL";
	
	// Verify list contents
	EXPECT_STREQ(str_list[0], "user1");
	EXPECT_STREQ(str_list[1], "user2");
	EXPECT_STREQ(str_list[2], "user3");
	EXPECT_STREQ(str_list[3], "admin");
	EXPECT_STREQ(str_list[4], "moderator");
	EXPECT_EQ(str_list[5], nullptr) << "List should be NULL-terminated";
	
	// Cleanup
	for (int i = 0; str_list[i] != nullptr; i++) {
		free(str_list[i]);
	}
	free(str_list);
	free(result);
}

TEST_F(AdvancedTypesTest, CppSendsListToPython) {
	// Create list in C++
	char *users[] = {
		strdup("alice"),
		strdup("bob"),
		strdup("charlie"),
		nullptr
	};
	
	w_Targs *args = w_pack("L", users);
	ASSERT_NE(args, nullptr) << "Failed to pack list";
	
	// Send to Python function that processes list
	w_Targs *result = w_CallFunction(script_id, "process_string_list", args);
	
	free(args);
	for (int i = 0; users[i] != nullptr; i++) {
		free(users[i]);
	}
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	
	// Python should return count (3)
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count)) << "Failed to unpack count";
	EXPECT_EQ(count, 3) << "Expected 3 users";
	
	free(result);
}

// ===== Phase 3: Dict/JSON Marshaling Tests =====

TEST_F(AdvancedTypesTest, PythonReturnsDict) {
	// Call Python function that returns dict
	w_Targs *result = w_CallFunction(script_id, "return_dict", nullptr);
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	ASSERT_NE(result->format, nullptr) << "Result format is NULL";
	ASSERT_EQ(result->format[0], 'D') << "Expected 'D' format, got: " << result->format;
	
	// Unpack as JSON string
	char *json_str;
	ASSERT_TRUE(w_unpack(result, "D", &json_str)) << "Failed to unpack dict";
	ASSERT_NE(json_str, nullptr) << "JSON string is NULL";
	
	// Verify JSON contains expected keys
	EXPECT_NE(strstr(json_str, "total_users"), nullptr) << "Missing 'total_users' key";
	EXPECT_NE(strstr(json_str, "hub_name"), nullptr) << "Missing 'hub_name' key";
	EXPECT_NE(strstr(json_str, "TestHub"), nullptr) << "Missing hub name value";
	
	std::cout << "Returned JSON: " << json_str << std::endl;
	
	free(json_str);
	free(result);
}

TEST_F(AdvancedTypesTest, CppSendsDictToPython) {
	// Create JSON string in C++
	const char *json_config = "{\"hub_name\":\"MyHub\",\"max_users\":100}";
	
	w_Targs *args = w_pack("D", json_config);
	ASSERT_NE(args, nullptr) << "Failed to pack dict";
	
	// Send to Python function that processes dict
	w_Targs *result = w_CallFunction(script_id, "process_dict", args);
	
	free(args);
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	
	// Python returns dict with validation results
	char *response_json;
	ASSERT_TRUE(w_unpack(result, "D", &response_json)) << "Failed to unpack response";
	ASSERT_NE(response_json, nullptr) << "Response JSON is NULL";
	
	// Should contain "status": "valid"
	EXPECT_NE(strstr(response_json, "valid"), nullptr) << "Config should be valid";
	
	std::cout << "Validation response: " << response_json << std::endl;
	
	free(response_json);
	free(result);
}

TEST_F(AdvancedTypesTest, ComplexDictRoundTrip) {
	// Create complex JSON
	const char *json_in = "{\"count\":5,\"items\":[\"a\",\"b\",\"c\"]}";
	
	w_Targs *args = w_pack("D", json_in);
	w_Targs *result = w_CallFunction(script_id, "complex_round_trip", args);
	
	free(args);
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	
	char *json_out;
	ASSERT_TRUE(w_unpack(result, "D", &json_out)) << "Failed to unpack result";
	
	// Python should add "processed": true and increment count
	EXPECT_NE(strstr(json_out, "processed"), nullptr) << "Missing 'processed' key";
	EXPECT_NE(strstr(json_out, "true"), nullptr) << "processed should be true";
	EXPECT_NE(strstr(json_out, "count"), nullptr) << "Missing 'count' key";
	
	std::cout << "Round-trip result: " << json_out << std::endl;
	
	free(json_out);
	free(result);
}

// ===== Phase 4: Bidirectional API Tests =====

TEST_F(AdvancedTypesTest, CallPythonWithArgs) {
	// Test calling Python with arguments
	const char *test_nick = "TestUser123";
	w_Targs *args = w_pack("s", test_nick);
	
	w_Targs *result = w_CallFunction(script_id, "get_user_info", args);
	
	free(args);
	
	ASSERT_NE(result, nullptr) << "Function call failed";
	
	// Should return dict with user info
	char *user_info_json;
	ASSERT_TRUE(w_unpack(result, "D", &user_info_json)) << "Failed to unpack user info";
	
	EXPECT_NE(strstr(user_info_json, test_nick), nullptr) << "Nick not in result";
	EXPECT_NE(strstr(user_info_json, "class"), nullptr) << "Missing class field";
	EXPECT_NE(strstr(user_info_json, "share_size"), nullptr) << "Missing share_size field";
	
	std::cout << "User info: " << user_info_json << std::endl;
	
	free(user_info_json);
	free(result);
}

TEST_F(AdvancedTypesTest, GetTestStatistics) {
	// Call all functions first
	w_CallFunction(script_id, "return_string_list", nullptr);
	w_CallFunction(script_id, "return_dict", nullptr);
	
	// Get statistics
	w_Targs *result = w_CallFunction(script_id, "get_test_stats", nullptr);
	
	ASSERT_NE(result, nullptr) << "get_test_stats failed";
	
	char *stats_json;
	ASSERT_TRUE(w_unpack(result, "D", &stats_json)) << "Failed to unpack stats";
	
	EXPECT_NE(strstr(stats_json, "total_calls"), nullptr) << "Missing total_calls";
	EXPECT_NE(strstr(stats_json, "details"), nullptr) << "Missing details";
	
	std::cout << "Test statistics: " << stats_json << std::endl;
	
	free(stats_json);
	free(result);
}

// ===== Stress Tests =====

TEST_F(AdvancedTypesTest, LargeListPerformance) {
	// Create large list (1000 strings)
	const int LIST_SIZE = 1000;
	char **large_list = (char **)malloc((LIST_SIZE + 1) * sizeof(char *));
	
	for (int i = 0; i < LIST_SIZE; i++) {
		char buf[32];
		snprintf(buf, sizeof(buf), "user_%d", i);
		large_list[i] = strdup(buf);
	}
	large_list[LIST_SIZE] = nullptr;
	
	w_Targs *args = w_pack("L", large_list);
	w_Targs *result = w_CallFunction(script_id, "process_string_list", args);
	
	// Cleanup input list
	free(args);
	for (int i = 0; i < LIST_SIZE; i++) {
		free(large_list[i]);
	}
	free(large_list);
	
	ASSERT_NE(result, nullptr) << "Large list processing failed";
	
	long count;
	ASSERT_TRUE(w_unpack(result, "l", &count)) << "Failed to unpack count";
	EXPECT_EQ(count, LIST_SIZE) << "Count mismatch";
	
	free(result);
}

TEST_F(AdvancedTypesTest, ComplexJSONPerformance) {
	// Create complex nested JSON
	const char *complex_json = "{"
		"\"users\":{\"total\":100,\"active\":50},"
		"\"config\":{\"max_upload\":1000000,\"timeouts\":[30,60,120]},"
		"\"metadata\":{\"version\":\"1.6.0\",\"build\":\"release\"}"
	"}";
	
	w_Targs *args = w_pack("D", complex_json);
	w_Targs *result = w_CallFunction(script_id, "complex_round_trip", args);
	
	free(args);
	
	ASSERT_NE(result, nullptr) << "Complex JSON processing failed";
	
	char *result_json;
	ASSERT_TRUE(w_unpack(result, "D", &result_json)) << "Failed to unpack result";
	
	// Should still have original structure plus additions
	EXPECT_NE(strstr(result_json, "users"), nullptr);
	EXPECT_NE(strstr(result_json, "config"), nullptr);
	EXPECT_NE(strstr(result_json, "processed"), nullptr);
	
	free(result_json);
	free(result);
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
