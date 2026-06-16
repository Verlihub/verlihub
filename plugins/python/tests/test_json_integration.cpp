// Test JSON marshaling integration with wrapper communication
// Tests complex nested data structures passed between C++ and Python

#include <gtest/gtest.h>
#include "wrapper.h"
#include "json_marshal.h"
#include <Python.h>
#include <string>
#include <cstring>

using namespace nVerliHub::nPythonPlugin;

#ifdef HAVE_RAPIDJSON
// Forward declarations for wrapper functions (defined in wrapper.cpp)
namespace nVerliHub {
namespace nPythonPlugin {
	bool PyObjectToJsonValue(PyObject* obj, JsonValue& out_val);
	PyObject* JsonValueToPyObject(const JsonValue& value);
	char* PyObjectToJsonString(PyObject* obj);
	PyObject* JsonStringToPyObject(const char* json_str);
}
}
#endif

class JsonIntegrationTest : public ::testing::Test {
protected:
	void SetUp() override {
		if (!Py_IsInitialized()) {
			Py_Initialize();
		}
	}
	
	void TearDown() override {
		// Don't finalize Python - keep it running for other tests
	}
};

// Test: Complex nested dict C++ -> Python -> C++
TEST_F(JsonIntegrationTest, NestedDictRoundTrip) {
	// Create complex nested structure in JSON
	const char *json_input = R"({
		"user": {
			"name": "TestUser",
			"level": 42,
			"score": 98.5,
			"active": true,
			"tags": ["vip", "moderator", "verified"],
			"stats": {
				"messages": 1234,
				"uptime": 56789.12,
				"flags": [1, 2, 3, 5, 8, 13]
			}
		},
		"settings": {
			"theme": "dark",
			"notifications": false,
			"limits": [10, 100, 1000]
		}
	})";
	
	// Parse JSON to JsonValue
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	// Convert to JSON string
	std::string json_str = nVerliHub::nPythonPlugin::toJsonString(val);
	ASSERT_FALSE(json_str.empty());
	
	// Pack as 'D' format
	w_Targs *args = w_pack("D", strdup(json_str.c_str()));
	ASSERT_NE(args, nullptr);
	ASSERT_STREQ(args->format, "D");
	
	// Verify we can unpack it
	char *unpacked_json;
	w_unpack(args, "D", &unpacked_json);
	ASSERT_NE(unpacked_json, nullptr);
	
	// Parse unpacked JSON
	JsonValue val2;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(unpacked_json, val2));
	ASSERT_EQ(val2.type, JsonType::OBJECT);
	
	// Verify nested structure
	auto user_it = val2.object_val.find("user");
	ASSERT_NE(user_it, val2.object_val.end());
	ASSERT_EQ(user_it->second.type, JsonType::OBJECT);
	
	auto name_it = user_it->second.object_val.find("name");
	ASSERT_NE(name_it, user_it->second.object_val.end());
	ASSERT_EQ(name_it->second.type, JsonType::STRING);
	ASSERT_EQ(name_it->second.string_val, "TestUser");
	
	auto level_it = user_it->second.object_val.find("level");
	ASSERT_NE(level_it, user_it->second.object_val.end());
	ASSERT_EQ(level_it->second.type, JsonType::INT);
	ASSERT_EQ(level_it->second.int_val, 42);
	
	auto score_it = user_it->second.object_val.find("score");
	ASSERT_NE(score_it, user_it->second.object_val.end());
	ASSERT_EQ(score_it->second.type, JsonType::DOUBLE);
	ASSERT_DOUBLE_EQ(score_it->second.double_val, 98.5);
	
	// Cleanup
	w_free_args(args);
}

// Test: Complex nested list with mixed types
TEST_F(JsonIntegrationTest, NestedListMixedTypes) {
	const char *json_input = R"([
		"string_value",
		42,
		3.14159,
		true,
		false,
		null,
		["nested", "list", 123],
		{"nested": "dict", "value": 999}
	])";
	
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::ARRAY);
	ASSERT_EQ(val.array_val.size(), 8);
	
	// Check each type
	ASSERT_EQ(val.array_val[0].type, JsonType::STRING);
	ASSERT_EQ(val.array_val[0].string_val, "string_value");
	
	ASSERT_EQ(val.array_val[1].type, JsonType::INT);
	ASSERT_EQ(val.array_val[1].int_val, 42);
	
	ASSERT_EQ(val.array_val[2].type, JsonType::DOUBLE);
	ASSERT_NEAR(val.array_val[2].double_val, 3.14159, 0.00001);
	
	ASSERT_EQ(val.array_val[3].type, JsonType::BOOL);
	ASSERT_TRUE(val.array_val[3].bool_val);
	
	ASSERT_EQ(val.array_val[4].type, JsonType::BOOL);
	ASSERT_FALSE(val.array_val[4].bool_val);
	
	ASSERT_EQ(val.array_val[5].type, JsonType::NULL_TYPE);
	
	ASSERT_EQ(val.array_val[6].type, JsonType::ARRAY);
	ASSERT_EQ(val.array_val[6].array_val.size(), 3);
	
	ASSERT_EQ(val.array_val[7].type, JsonType::OBJECT);
	ASSERT_EQ(val.array_val[7].object_val.size(), 2);
	
	// Pack and verify
	std::string json_str = nVerliHub::nPythonPlugin::toJsonString(val);
	w_Targs *args = w_pack("D", strdup(json_str.c_str()));
	ASSERT_NE(args, nullptr);
	
	char *unpacked_json;
	w_unpack(args, "D", &unpacked_json);
	ASSERT_NE(unpacked_json, nullptr);
	
	JsonValue val2;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(unpacked_json, val2));
	ASSERT_EQ(val2.type, JsonType::ARRAY);
	ASSERT_EQ(val2.array_val.size(), 8);
	
	w_free_args(args);
}

// Test: PyObject to JsonValue and back (requires Python)
#ifdef HAVE_RAPIDJSON
TEST_F(JsonIntegrationTest, PyObjectBidirectionalConversion) {
	PyGILState_STATE gstate = PyGILState_Ensure();
	
	// Create complex Python structure
	PyObject *py_dict = PyDict_New();
	PyDict_SetItemString(py_dict, "name", PyUnicode_FromString("Alice"));
	PyDict_SetItemString(py_dict, "age", PyLong_FromLong(30));
	PyDict_SetItemString(py_dict, "score", PyFloat_FromDouble(95.5));
	PyDict_SetItemString(py_dict, "active", Py_True);
	
	// Add nested list
	PyObject *py_list = PyList_New(0);
	PyList_Append(py_list, PyLong_FromLong(1));
	PyList_Append(py_list, PyLong_FromLong(2));
	PyList_Append(py_list, PyLong_FromLong(3));
	PyDict_SetItemString(py_dict, "numbers", py_list);
	
	// Add nested dict
	PyObject *py_nested = PyDict_New();
	PyDict_SetItemString(py_nested, "key1", PyUnicode_FromString("value1"));
	PyDict_SetItemString(py_nested, "key2", PyLong_FromLong(123));
	PyDict_SetItemString(py_dict, "nested", py_nested);
	
	// Convert to JsonValue
	JsonValue val;
	ASSERT_TRUE(PyObjectToJsonValue(py_dict, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	// Verify structure
	ASSERT_EQ(val.object_val["name"].string_val, "Alice");
	ASSERT_EQ(val.object_val["age"].int_val, 30);
	ASSERT_DOUBLE_EQ(val.object_val["score"].double_val, 95.5);
	ASSERT_TRUE(val.object_val["active"].bool_val);
	
	ASSERT_EQ(val.object_val["numbers"].type, JsonType::ARRAY);
	ASSERT_EQ(val.object_val["numbers"].array_val.size(), 3);
	ASSERT_EQ(val.object_val["numbers"].array_val[0].int_val, 1);
	
	ASSERT_EQ(val.object_val["nested"].type, JsonType::OBJECT);
	ASSERT_EQ(val.object_val["nested"].object_val["key1"].string_val, "value1");
	ASSERT_EQ(val.object_val["nested"].object_val["key2"].int_val, 123);
	
	// Convert back to PyObject
	PyObject *py_dict2 = JsonValueToPyObject(val);
	ASSERT_NE(py_dict2, nullptr);
	ASSERT_TRUE(PyDict_Check(py_dict2));
	
	// Verify round-trip
	PyObject *py_name = PyDict_GetItemString(py_dict2, "name");
	ASSERT_NE(py_name, nullptr);
	ASSERT_TRUE(PyUnicode_Check(py_name));
	ASSERT_STREQ(PyUnicode_AsUTF8(py_name), "Alice");
	
	PyObject *py_age = PyDict_GetItemString(py_dict2, "age");
	ASSERT_NE(py_age, nullptr);
	ASSERT_TRUE(PyLong_Check(py_age));
	ASSERT_EQ(PyLong_AsLong(py_age), 30);
	
	PyObject *py_numbers = PyDict_GetItemString(py_dict2, "numbers");
	ASSERT_NE(py_numbers, nullptr);
	ASSERT_TRUE(PyList_Check(py_numbers));
	ASSERT_EQ(PyList_Size(py_numbers), 3);
	
	Py_DECREF(py_dict);
	Py_DECREF(py_dict2);
	
	PyGILState_Release(gstate);
}

// Test: PyObject to JSON string and back
TEST_F(JsonIntegrationTest, PyObjectJsonStringConversion) {
	PyGILState_STATE gstate = PyGILState_Ensure();
	
	// Create Python list with mixed types
	PyObject *py_list = PyList_New(0);
	PyList_Append(py_list, PyUnicode_FromString("hello"));
	PyList_Append(py_list, PyLong_FromLong(42));
	PyList_Append(py_list, PyFloat_FromDouble(3.14));
	PyList_Append(py_list, Py_True);
	PyList_Append(py_list, Py_None);
	
	// Convert to JSON string
	char *json_str = PyObjectToJsonString(py_list);
	ASSERT_NE(json_str, nullptr);
	
	// Parse back
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_str, val));
	ASSERT_EQ(val.type, JsonType::ARRAY);
	ASSERT_EQ(val.array_val.size(), 5);
	ASSERT_EQ(val.array_val[0].string_val, "hello");
	ASSERT_EQ(val.array_val[1].int_val, 42);
	ASSERT_DOUBLE_EQ(val.array_val[2].double_val, 3.14);
	ASSERT_TRUE(val.array_val[3].bool_val);
	ASSERT_EQ(val.array_val[4].type, JsonType::NULL_TYPE);
	
	// Convert JSON string back to PyObject
	PyObject *py_list2 = JsonStringToPyObject(json_str);
	ASSERT_NE(py_list2, nullptr);
	ASSERT_TRUE(PyList_Check(py_list2));
	ASSERT_EQ(PyList_Size(py_list2), 5);
	
	free(json_str);
	Py_DECREF(py_list);
	Py_DECREF(py_list2);
	
	PyGILState_Release(gstate);
}

// Test: Deeply nested structure (5 levels)
TEST_F(JsonIntegrationTest, DeeplyNestedStructure) {
	const char *json_input = R"({
		"level1": {
			"level2": {
				"level3": {
					"level4": {
						"level5": {
							"value": "deep",
							"number": 12345
						}
					}
				}
			}
		}
	})";
	
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	// Navigate to level 5
	auto& l1 = val.object_val["level1"];
	ASSERT_EQ(l1.type, JsonType::OBJECT);
	
	auto& l2 = l1.object_val["level2"];
	ASSERT_EQ(l2.type, JsonType::OBJECT);
	
	auto& l3 = l2.object_val["level3"];
	ASSERT_EQ(l3.type, JsonType::OBJECT);
	
	auto& l4 = l3.object_val["level4"];
	ASSERT_EQ(l4.type, JsonType::OBJECT);
	
	auto& l5 = l4.object_val["level5"];
	ASSERT_EQ(l5.type, JsonType::OBJECT);
	
	ASSERT_EQ(l5.object_val["value"].string_val, "deep");
	ASSERT_EQ(l5.object_val["number"].int_val, 12345);
	
	// Round-trip through string
	std::string json_str = nVerliHub::nPythonPlugin::toJsonString(val);
	JsonValue val2 ;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_str.c_str(), val2));
	
	auto& l5_2 = val2.object_val["level1"].object_val["level2"]
		.object_val["level3"].object_val["level4"].object_val["level5"];
	ASSERT_EQ(l5_2.object_val["value"].string_val, "deep");
	ASSERT_EQ(l5_2.object_val["number"].int_val, 12345);
}

// Test: Large array performance
TEST_F(JsonIntegrationTest, LargeArrayPerformance) {
	// Create array with 1000 elements
	JsonValue val;
	val.type = JsonType::ARRAY;
	for (int i = 0; i < 1000; i++) {
		JsonValue item;
		item.type = JsonType::OBJECT;
		item.object_val["id"] = JsonValue((int64_t)i);
		item.object_val["name"] = JsonValue("item_" + std::to_string(i));
		item.object_val["value"] = JsonValue((double)i * 1.5);
		val.array_val.push_back(item);
	}
	
	// Serialize
	std::string json_str = nVerliHub::nPythonPlugin::toJsonString(val);
	ASSERT_FALSE(json_str.empty());
	
	// Deserialize
	JsonValue val2 ;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_str.c_str(), val2));
	ASSERT_EQ(val2.type, JsonType::ARRAY);
	ASSERT_EQ(val2.array_val.size(), 1000);
	
	// Spot check a few elements
	ASSERT_EQ(val2.array_val[0].object_val["id"].int_val, 0);
	ASSERT_EQ(val2.array_val[0].object_val["name"].string_val, "item_0");
	
	ASSERT_EQ(val2.array_val[500].object_val["id"].int_val, 500);
	ASSERT_EQ(val2.array_val[500].object_val["name"].string_val, "item_500");
	
	ASSERT_EQ(val2.array_val[999].object_val["id"].int_val, 999);
	ASSERT_EQ(val2.array_val[999].object_val["name"].string_val, "item_999");
}

// Test: Unicode strings
TEST_F(JsonIntegrationTest, UnicodeStrings) {
	const char *json_input = R"({
		"english": "Hello World",
		"chinese": "你好世界",
		"emoji": "🎉🚀✨",
		"russian": "Привет мир",
		"arabic": "مرحبا بالعالم"
	})";
	
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	// Verify all strings present
	ASSERT_EQ(val.object_val.size(), 5);
	ASSERT_EQ(val.object_val["english"].string_val, "Hello World");
	ASSERT_FALSE(val.object_val["chinese"].string_val.empty());
	ASSERT_FALSE(val.object_val["emoji"].string_val.empty());
	ASSERT_FALSE(val.object_val["russian"].string_val.empty());
	ASSERT_FALSE(val.object_val["arabic"].string_val.empty());
	
	// Round-trip
	std::string json_str = nVerliHub::nPythonPlugin::toJsonString(val);
	JsonValue val2 ;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_str.c_str(), val2));
	ASSERT_EQ(val2.object_val["english"].string_val, "Hello World");
}

// Test: Special characters and escaping
TEST_F(JsonIntegrationTest, SpecialCharactersEscaping) {
	const char *json_input = R"({
		"quotes": "He said \"hello\"",
		"backslash": "C:\\path\\to\\file",
		"newline": "line1\nline2",
		"tab": "col1\tcol2",
		"unicode": "\u0041\u0042\u0043"
	})";
	
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	ASSERT_EQ(val.object_val["quotes"].string_val, "He said \"hello\"");
	ASSERT_EQ(val.object_val["backslash"].string_val, "C:\\path\\to\\file");
	ASSERT_EQ(val.object_val["newline"].string_val, "line1\nline2");
	ASSERT_EQ(val.object_val["tab"].string_val, "col1\tcol2");
	ASSERT_EQ(val.object_val["unicode"].string_val, "ABC");
}

// Test: 64-bit integer values (Python compatibility)
TEST_F(JsonIntegrationTest, LargeIntegerValues) {
	const char *json_input = R"({
		"small": 42,
		"large_positive": 9223372036854775807,
		"large_negative": -9223372036854775808
	})";
	
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	ASSERT_EQ(val.object_val["small"].int_val, 42);
	ASSERT_EQ(val.object_val["large_positive"].int_val, 9223372036854775807LL);
	// Note: JSON spec doesn't guarantee exact parsing of INT64_MIN
	// RapidJSON may convert very large numbers to double
}

// Test: Mixed list and dict nesting
TEST_F(JsonIntegrationTest, MixedListDictNesting) {
	const char *json_input = R"({
		"users": [
			{"name": "Alice", "scores": [95, 87, 92]},
			{"name": "Bob", "scores": [78, 85, 90]},
			{"name": "Charlie", "scores": [88, 92, 95]}
		],
		"metadata": {
			"total": 3,
			"average_scores": [87.0, 88.0, 92.3]
		}
	})";
	
	JsonValue val;
	ASSERT_TRUE(nVerliHub::nPythonPlugin::parseJson(json_input, val));
	ASSERT_EQ(val.type, JsonType::OBJECT);
	
	auto& users = val.object_val["users"];
	ASSERT_EQ(users.type, JsonType::ARRAY);
	ASSERT_EQ(users.array_val.size(), 3);
	
	// Check first user
	auto& alice = users.array_val[0];
	ASSERT_EQ(alice.object_val["name"].string_val, "Alice");
	ASSERT_EQ(alice.object_val["scores"].array_val.size(), 3);
	ASSERT_EQ(alice.object_val["scores"].array_val[0].int_val, 95);
	
	// Check metadata
	auto& metadata = val.object_val["metadata"];
	ASSERT_EQ(metadata.object_val["total"].int_val, 3);
	ASSERT_EQ(metadata.object_val["average_scores"].array_val.size(), 3);
}
#endif // HAVE_RAPIDJSON

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
