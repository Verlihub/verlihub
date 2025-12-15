/*
	Copyright (C) 2025 Verlihub Team, info at verlihub dot net

	Verlihub is free software; You can redistribute it
	and modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation, either version 3 of the license, or at
	your option any later version.

	JSON marshaling utilities for Python-C++ interop
	Uses RapidJSON for high-performance JSON parsing
*/

#ifndef JSON_MARSHAL_H
#define JSON_MARSHAL_H

#ifdef HAVE_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#endif

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>  // For int64_t

namespace nVerliHub {
namespace nPythonPlugin {

// JSON types that can be marshaled
enum class JsonType {
	NULL_TYPE,
	BOOL,
	INT,
	DOUBLE,
	STRING,
	ARRAY,
	OBJECT,
	SET,      // Python set → std::set
	TUPLE     // Python tuple → std::vector (immutable list)
};

// Generic JSON value wrapper for C++
struct JsonValue {
	JsonType type;
	
	// Primitive values (use explicit 64-bit types for Python compatibility)
	bool bool_val;
	int64_t int_val;      // Always 64-bit for Python int
	double double_val;    // Always 64-bit (double precision) for Python float
	std::string string_val;
	
	// Container values
	std::vector<JsonValue> array_val;
	std::map<std::string, JsonValue> object_val;
	std::set<JsonValue> set_val;      // For Python sets
	std::vector<JsonValue> tuple_val; // For Python tuples (immutable, but stored as vector)
	
	// Comparison operator for std::set storage
	bool operator<(const JsonValue& other) const {
		if (type != other.type) return type < other.type;
		switch (type) {
			case JsonType::NULL_TYPE: return false;
			case JsonType::BOOL: return bool_val < other.bool_val;
			case JsonType::INT: return int_val < other.int_val;
			case JsonType::DOUBLE: return double_val < other.double_val;
			case JsonType::STRING: return string_val < other.string_val;
			case JsonType::ARRAY: return array_val < other.array_val;
			case JsonType::SET: return set_val < other.set_val;
			case JsonType::TUPLE: return tuple_val < other.tuple_val;
			case JsonType::OBJECT: return object_val < other.object_val;
		}
		return false;
	}
	
	JsonValue() : type(JsonType::NULL_TYPE), bool_val(false), int_val(0), double_val(0.0) {}
	
	// Convenience constructors
	explicit JsonValue(bool v) : type(JsonType::BOOL), bool_val(v), int_val(0), double_val(0.0) {}
	explicit JsonValue(int64_t v) : type(JsonType::INT), bool_val(false), int_val(v), double_val(0.0) {}
	explicit JsonValue(int v) : type(JsonType::INT), bool_val(false), int_val((int64_t)v), double_val(0.0) {}
	explicit JsonValue(double v) : type(JsonType::DOUBLE), bool_val(false), int_val(0), double_val(v) {}
	explicit JsonValue(const std::string& v) : type(JsonType::STRING), bool_val(false), int_val(0), double_val(0.0), string_val(v) {}
	explicit JsonValue(const char* v) : type(JsonType::STRING), bool_val(false), int_val(0), double_val(0.0), string_val(v) {}
	
	bool isNull() const { return type == JsonType::NULL_TYPE; }
	bool isBool() const { return type == JsonType::BOOL; }
	bool isInt() const { return type == JsonType::INT; }
	bool isDouble() const { return type == JsonType::DOUBLE; }
	bool isString() const { return type == JsonType::STRING; }
	bool isArray() const { return type == JsonType::ARRAY; }
	bool isObject() const { return type == JsonType::OBJECT; }
	bool isSet() const { return type == JsonType::SET; }
	bool isTuple() const { return type == JsonType::TUPLE; }
};

#ifdef HAVE_RAPIDJSON

// Parse JSON string into JsonValue structure
bool parseJson(const std::string& json_str, JsonValue& out_value);

// Convert JsonValue back to JSON string
std::string toJsonString(const JsonValue& value, bool pretty = false);

// Helper to convert string list to JSON array
std::string stringListToJson(const std::vector<std::string>& string_list);

// Helper to parse JSON array back to string list
std::vector<std::string> jsonToStringList(const std::string& json_str);

// Helper to convert map<string,string> to JSON object
std::string stringMapToJson(const std::map<std::string, std::string>& map);

// Helper to parse JSON object to map<string,string>
bool jsonToStringMap(const std::string& json_str, std::map<std::string, std::string>& out_map);

#endif // HAVE_RAPIDJSON

} // namespace nPythonPlugin
} // namespace nVerliHub

#endif // JSON_MARSHAL_H
