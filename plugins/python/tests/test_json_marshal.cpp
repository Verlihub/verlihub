/*
	Unit tests for JSON marshaling with RapidJSON
	Tests serialization and deserialization of complex data structures
*/

#include <gtest/gtest.h>
#include "../json_marshal.h"
#include <cstring>

using namespace nVerliHub::nPythonPlugin;

#ifdef HAVE_RAPIDJSON

class JsonMarshalTest : public ::testing::Test {
protected:
	void SetUp() override {
	}
	
	void TearDown() override {
	}
};

// ===== Basic Type Tests =====

TEST_F(JsonMarshalTest, ParseNull) {
	JsonValue value;
	ASSERT_TRUE(parseJson("null", value));
	EXPECT_TRUE(value.isNull());
}

TEST_F(JsonMarshalTest, ParseBool) {
	JsonValue value_true, value_false;
	ASSERT_TRUE(parseJson("true", value_true));
	ASSERT_TRUE(parseJson("false", value_false));
	
	EXPECT_TRUE(value_true.isBool());
	EXPECT_EQ(value_true.bool_val, true);
	
	EXPECT_TRUE(value_false.isBool());
	EXPECT_EQ(value_false.bool_val, false);
}

TEST_F(JsonMarshalTest, ParseInteger) {
	JsonValue value;
	ASSERT_TRUE(parseJson("42", value));
	EXPECT_TRUE(value.isInt());
	EXPECT_EQ(value.int_val, 42);
	
	ASSERT_TRUE(parseJson("-123", value));
	EXPECT_TRUE(value.isInt());
	EXPECT_EQ(value.int_val, -123);
}

TEST_F(JsonMarshalTest, ParseDouble) {
	JsonValue value;
	ASSERT_TRUE(parseJson("3.14159", value));
	EXPECT_TRUE(value.isDouble());
	EXPECT_NEAR(value.double_val, 3.14159, 0.00001);
}

TEST_F(JsonMarshalTest, ParseString) {
	JsonValue value;
	ASSERT_TRUE(parseJson("\"Hello World\"", value));
	EXPECT_TRUE(value.isString());
	EXPECT_EQ(value.string_val, "Hello World");
}

// ===== Array Tests =====

TEST_F(JsonMarshalTest, ParseEmptyArray) {
	JsonValue value;
	ASSERT_TRUE(parseJson("[]", value));
	EXPECT_TRUE(value.isArray());
	EXPECT_EQ(value.array_val.size(), 0);
}

TEST_F(JsonMarshalTest, ParseStringArray) {
	JsonValue value;
	ASSERT_TRUE(parseJson("[\"apple\", \"banana\", \"cherry\"]", value));
	EXPECT_TRUE(value.isArray());
	ASSERT_EQ(value.array_val.size(), 3);
	
	EXPECT_TRUE(value.array_val[0].isString());
	EXPECT_EQ(value.array_val[0].string_val, "apple");
	EXPECT_EQ(value.array_val[1].string_val, "banana");
	EXPECT_EQ(value.array_val[2].string_val, "cherry");
}

TEST_F(JsonMarshalTest, ParseNumberArray) {
	JsonValue value;
	ASSERT_TRUE(parseJson("[1, 2, 3, 4, 5]", value));
	EXPECT_TRUE(value.isArray());
	ASSERT_EQ(value.array_val.size(), 5);
	
	for (int i = 0; i < 5; i++) {
		EXPECT_TRUE(value.array_val[i].isInt());
		EXPECT_EQ(value.array_val[i].int_val, i + 1);
	}
}

TEST_F(JsonMarshalTest, ParseMixedArray) {
	JsonValue value;
	ASSERT_TRUE(parseJson("[1, \"two\", 3.0, true, null]", value));
	EXPECT_TRUE(value.isArray());
	ASSERT_EQ(value.array_val.size(), 5);
	
	EXPECT_TRUE(value.array_val[0].isInt());
	EXPECT_TRUE(value.array_val[1].isString());
	EXPECT_TRUE(value.array_val[2].isDouble());
	EXPECT_TRUE(value.array_val[3].isBool());
	EXPECT_TRUE(value.array_val[4].isNull());
}

// ===== Object Tests =====

TEST_F(JsonMarshalTest, ParseEmptyObject) {
	JsonValue value;
	ASSERT_TRUE(parseJson("{}", value));
	EXPECT_TRUE(value.isObject());
	EXPECT_EQ(value.object_val.size(), 0);
}

TEST_F(JsonMarshalTest, ParseSimpleObject) {
	JsonValue value;
	ASSERT_TRUE(parseJson("{\"name\": \"John\", \"age\": 30}", value));
	EXPECT_TRUE(value.isObject());
	ASSERT_EQ(value.object_val.size(), 2);
	
	EXPECT_TRUE(value.object_val["name"].isString());
	EXPECT_EQ(value.object_val["name"].string_val, "John");
	
	EXPECT_TRUE(value.object_val["age"].isInt());
	EXPECT_EQ(value.object_val["age"].int_val, 30);
}

TEST_F(JsonMarshalTest, ParseNestedObject) {
	const char* json = R"({
		"user": {
			"name": "Alice",
			"address": {
				"city": "NYC",
				"zip": 10001
			}
		}
	})";
	
	JsonValue value;
	ASSERT_TRUE(parseJson(json, value));
	EXPECT_TRUE(value.isObject());
	
	EXPECT_TRUE(value.object_val["user"].isObject());
	EXPECT_EQ(value.object_val["user"].object_val["name"].string_val, "Alice");
	
	auto& address = value.object_val["user"].object_val["address"];
	EXPECT_TRUE(address.isObject());
	EXPECT_EQ(address.object_val["city"].string_val, "NYC");
	EXPECT_EQ(address.object_val["zip"].int_val, 10001);
}

// ===== Serialization Tests =====

TEST_F(JsonMarshalTest, SerializeNull) {
	JsonValue value;
	std::string json = toJsonString(value);
	EXPECT_EQ(json, "null");
}

TEST_F(JsonMarshalTest, SerializeBool) {
	JsonValue value_true(true);
	JsonValue value_false(false);
	
	EXPECT_EQ(toJsonString(value_true), "true");
	EXPECT_EQ(toJsonString(value_false), "false");
}

TEST_F(JsonMarshalTest, SerializeInt) {
	JsonValue value((int64_t)42);
	EXPECT_EQ(toJsonString(value), "42");
}

TEST_F(JsonMarshalTest, SerializeDouble) {
	JsonValue value(3.14);
	std::string json = toJsonString(value);
	EXPECT_NE(json.find("3.14"), std::string::npos);
}

TEST_F(JsonMarshalTest, SerializeString) {
	JsonValue value("Hello");
	EXPECT_EQ(toJsonString(value), "\"Hello\"");
}

TEST_F(JsonMarshalTest, SerializeArray) {
	JsonValue arr;
	arr.type = JsonType::ARRAY;
	arr.array_val.push_back(JsonValue((int64_t)1));
	arr.array_val.push_back(JsonValue("two"));
	arr.array_val.push_back(JsonValue(3.0));
	
	std::string json = toJsonString(arr);
	// Should be something like: [1,"two",3.0]
	EXPECT_NE(json.find("["), std::string::npos);
	EXPECT_NE(json.find("\"two\""), std::string::npos);
	EXPECT_NE(json.find("]"), std::string::npos);
}

TEST_F(JsonMarshalTest, SerializeObject) {
	JsonValue obj;
	obj.type = JsonType::OBJECT;
	obj.object_val["name"] = JsonValue("Bob");
	obj.object_val["age"] = JsonValue((int64_t)25);
	
	std::string json = toJsonString(obj);
	EXPECT_NE(json.find("\"name\""), std::string::npos);
	EXPECT_NE(json.find("\"Bob\""), std::string::npos);
	EXPECT_NE(json.find("\"age\""), std::string::npos);
	EXPECT_NE(json.find("25"), std::string::npos);
}

// ===== Round-trip Tests =====

TEST_F(JsonMarshalTest, RoundTripSimple) {
	const char* original = "{\"x\":10,\"y\":20}";
	JsonValue value;
	ASSERT_TRUE(parseJson(original, value));
	
	std::string serialized = toJsonString(value);
	JsonValue value2;
	ASSERT_TRUE(parseJson(serialized, value2));
	
	EXPECT_EQ(value2.object_val["x"].int_val, 10);
	EXPECT_EQ(value2.object_val["y"].int_val, 20);
}

TEST_F(JsonMarshalTest, RoundTripComplex) {
	const char* original = R"({
		"users": ["Alice", "Bob", "Charlie"],
		"config": {
			"max": 100,
			"enabled": true
		}
	})";
	
	JsonValue value;
	ASSERT_TRUE(parseJson(original, value));
	
	std::string serialized = toJsonString(value);
	JsonValue value2;
	ASSERT_TRUE(parseJson(serialized, value2));
	
	EXPECT_EQ(value2.object_val["users"].array_val.size(), 3);
	EXPECT_EQ(value2.object_val["users"].array_val[0].string_val, "Alice");
	EXPECT_EQ(value2.object_val["config"].object_val["max"].int_val, 100);
	EXPECT_EQ(value2.object_val["config"].object_val["enabled"].bool_val, true);
}

// ===== Helper Function Tests =====

TEST_F(JsonMarshalTest, StringListToJsonHelper) {
	std::vector<std::string> list = {"user1", "user2", "user3"};
	
	std::string json = stringListToJson(list);
	EXPECT_EQ(json, "[\"user1\",\"user2\",\"user3\"]");
}

TEST_F(JsonMarshalTest, JsonToStringListHelper) {
	const char* json = "[\"apple\", \"banana\", \"cherry\"]";
	auto list = jsonToStringList(json);
	
	ASSERT_EQ(list.size(), 3);
	EXPECT_EQ(list[0], "apple");
	EXPECT_EQ(list[1], "banana");
	EXPECT_EQ(list[2], "cherry");
}

TEST_F(JsonMarshalTest, StringListRoundTrip) {
	std::vector<std::string> original = {"alpha", "beta", "gamma"};
	
	std::string json = stringListToJson(original);
	auto restored = jsonToStringList(json);
	
	ASSERT_EQ(restored.size(), 3);
	EXPECT_EQ(restored[0], "alpha");
	EXPECT_EQ(restored[1], "beta");
	EXPECT_EQ(restored[2], "gamma");
}

TEST_F(JsonMarshalTest, StringMapToJsonHelper) {
	std::map<std::string, std::string> map;
	map["name"] = "TestHub";
	map["version"] = "1.6.0";
	map["max_users"] = "100";
	
	std::string json = stringMapToJson(map);
	
	// Should contain all keys and values
	EXPECT_NE(json.find("\"name\""), std::string::npos);
	EXPECT_NE(json.find("\"TestHub\""), std::string::npos);
	EXPECT_NE(json.find("\"version\""), std::string::npos);
	EXPECT_NE(json.find("\"1.6.0\""), std::string::npos);
}

TEST_F(JsonMarshalTest, JsonToStringMapHelper) {
	const char* json = "{\"hub\":\"MyHub\",\"port\":\"411\",\"ssl\":\"true\"}";
	std::map<std::string, std::string> map;
	
	ASSERT_TRUE(jsonToStringMap(json, map));
	EXPECT_EQ(map.size(), 3);
	EXPECT_EQ(map["hub"], "MyHub");
	EXPECT_EQ(map["port"], "411");
	EXPECT_EQ(map["ssl"], "true");
}

TEST_F(JsonMarshalTest, StringMapRoundTrip) {
	std::map<std::string, std::string> original;
	original["key1"] = "value1";
	original["key2"] = "value2";
	original["key3"] = "value3";
	
	std::string json = stringMapToJson(original);
	std::map<std::string, std::string> restored;
	ASSERT_TRUE(jsonToStringMap(json.c_str(), restored));
	
	EXPECT_EQ(restored.size(), 3);
	EXPECT_EQ(restored["key1"], "value1");
	EXPECT_EQ(restored["key2"], "value2");
	EXPECT_EQ(restored["key3"], "value3");
}

// ===== Error Handling Tests =====

TEST_F(JsonMarshalTest, ParseInvalidJson) {
	JsonValue value;
	EXPECT_FALSE(parseJson("{invalid}", value));
	EXPECT_FALSE(parseJson("[1, 2,]", value)); // Trailing comma
	EXPECT_FALSE(parseJson("", value));
}

TEST_F(JsonMarshalTest, JsonToStringListInvalidJson) {
	auto list = jsonToStringList("{\"not\":\"an array\"}");
	EXPECT_TRUE(list.empty());
	
	list = jsonToStringList("[broken");
	EXPECT_TRUE(list.empty());
}

TEST_F(JsonMarshalTest, JsonToStringMapInvalidJson) {
	std::map<std::string, std::string> map;
	EXPECT_FALSE(jsonToStringMap("[\"not\",\"an object\"]", map));
	EXPECT_FALSE(jsonToStringMap("{broken", map));
}

// ===== Unicode and Special Characters =====

TEST_F(JsonMarshalTest, ParseUnicodeString) {
	JsonValue value;
	ASSERT_TRUE(parseJson("\"Hello 世界 🌍\"", value));
	EXPECT_TRUE(value.isString());
	EXPECT_EQ(value.string_val, "Hello 世界 🌍");
}

TEST_F(JsonMarshalTest, ParseEscapedCharacters) {
	JsonValue value;
	ASSERT_TRUE(parseJson("\"Line 1\\nLine 2\\tTabbed\"", value));
	EXPECT_TRUE(value.isString());
	EXPECT_NE(value.string_val.find('\n'), std::string::npos);
	EXPECT_NE(value.string_val.find('\t'), std::string::npos);
}

#else

// Fallback test when RapidJSON is not available
TEST(JsonMarshalTest, RapidJsonNotAvailable) {
	GTEST_SKIP() << "RapidJSON not available, skipping tests";
}

#endif // HAVE_RAPIDJSON

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
