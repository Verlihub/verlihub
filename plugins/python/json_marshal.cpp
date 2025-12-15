/*
	Copyright (C) 2025 Verlihub Team, info at verlihub dot net

	JSON marshaling utilities implementation
*/

#include "json_marshal.h"
#include <cstring>
#include <sstream>

#ifdef HAVE_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

namespace nVerliHub {
namespace nPythonPlugin {

using namespace rapidjson;

// Forward declaration
static bool parseValue(const Value& json_val, JsonValue& out_val);

// Parse RapidJSON Value into our JsonValue
static bool parseValue(const Value& json_val, JsonValue& out_val) {
	if (json_val.IsNull()) {
		out_val.type = JsonType::NULL_TYPE;
		return true;
	}
	
	if (json_val.IsBool()) {
		out_val.type = JsonType::BOOL;
		out_val.bool_val = json_val.GetBool();
		return true;
	}
	
	if (json_val.IsInt64()) {
		out_val.type = JsonType::INT;
		out_val.int_val = json_val.GetInt64();
		return true;
	}
	
	if (json_val.IsDouble()) {
		out_val.type = JsonType::DOUBLE;
		out_val.double_val = json_val.GetDouble();
		return true;
	}
	
	if (json_val.IsString()) {
		out_val.type = JsonType::STRING;
		out_val.string_val = json_val.GetString();
		return true;
	}
	
	if (json_val.IsArray()) {
		out_val.type = JsonType::ARRAY;
		out_val.array_val.clear();
		out_val.array_val.reserve(json_val.Size());
		
		for (SizeType i = 0; i < json_val.Size(); i++) {
			JsonValue elem;
			if (!parseValue(json_val[i], elem)) {
				return false;
			}
			out_val.array_val.push_back(elem);
		}
		return true;
	}
	
	if (json_val.IsObject()) {
		out_val.type = JsonType::OBJECT;
		out_val.object_val.clear();
		
		for (Value::ConstMemberIterator itr = json_val.MemberBegin(); 
		     itr != json_val.MemberEnd(); ++itr) {
			JsonValue elem;
			if (!parseValue(itr->value, elem)) {
				return false;
			}
			out_val.object_val[itr->name.GetString()] = elem;
		}
		return true;
	}
	
	return false;
}

// Public API: Parse JSON string
bool parseJson(const std::string& json_str, JsonValue& out_value) {
	if (json_str.empty()) return false;
	
	Document doc;
	doc.Parse(json_str.c_str());
	
	if (doc.HasParseError()) {
		// Log parse error if needed
		return false;
	}
	
	return parseValue(doc, out_value);
}

// Forward declaration
static void writeValue(Writer<StringBuffer>& writer, const JsonValue& value);
static void writeValue(PrettyWriter<StringBuffer>& writer, const JsonValue& value);

// Template helper to write JsonValue to RapidJSON writer
template<typename WriterType>
static void writeValueImpl(WriterType& writer, const JsonValue& value) {
	switch (value.type) {
		case JsonType::NULL_TYPE:
			writer.Null();
			break;
		case JsonType::BOOL:
			writer.Bool(value.bool_val);
			break;
		case JsonType::INT:
			writer.Int64(value.int_val);
			break;
		case JsonType::DOUBLE:
			writer.Double(value.double_val);
			break;
		case JsonType::STRING:
			writer.String(value.string_val.c_str());
			break;
		case JsonType::ARRAY:
			writer.StartArray();
			for (const auto& elem : value.array_val) {
				writeValueImpl(writer, elem);
			}
			writer.EndArray();
			break;
		case JsonType::TUPLE:
			// Tuples serialize as arrays (order preserved)
			writer.StartArray();
			for (const auto& elem : value.tuple_val) {
				writeValueImpl(writer, elem);
			}
			writer.EndArray();
			break;
		case JsonType::SET:
			// Sets serialize as arrays (order not guaranteed)
			writer.StartArray();
			for (const auto& elem : value.set_val) {
				writeValueImpl(writer, elem);
			}
			writer.EndArray();
			break;
		case JsonType::OBJECT:
			writer.StartObject();
			for (const auto& pair : value.object_val) {
				writer.Key(pair.first.c_str());
				writeValueImpl(writer, pair.second);
			}
			writer.EndObject();
			break;
	}
}

static void writeValue(Writer<StringBuffer>& writer, const JsonValue& value) {
	writeValueImpl(writer, value);
}

static void writeValue(PrettyWriter<StringBuffer>& writer, const JsonValue& value) {
	writeValueImpl(writer, value);
}

// Public API: Convert JsonValue to JSON string
std::string toJsonString(const JsonValue& value, bool pretty) {
	StringBuffer buffer;
	
	if (pretty) {
		PrettyWriter<StringBuffer> writer(buffer);
		writeValue(writer, value);
	} else {
		Writer<StringBuffer> writer(buffer);
		writeValue(writer, value);
	}
	
	return std::string(buffer.GetString());
}

// Helper: Convert string list to JSON array
std::string stringListToJson(const std::vector<std::string>& string_list) {
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	
	writer.StartArray();
	for (const auto& str : string_list) {
		writer.String(str.c_str());
	}
	writer.EndArray();
	
	return std::string(buffer.GetString());
}

// Helper: Parse JSON array to string list
std::vector<std::string> jsonToStringList(const std::string& json_str) {
	if (json_str.empty()) return {};
	
	Document doc;
	doc.Parse(json_str.c_str());
	
	if (doc.HasParseError() || !doc.IsArray()) {
		return {};
	}
	
	std::vector<std::string> result;
	result.reserve(doc.Size());
	
	for (size_t i = 0; i < doc.Size(); i++) {
		if (doc[i].IsString()) {
			result.push_back(doc[i].GetString());
		} else {
			// Convert non-string to string representation
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);
			doc[i].Accept(writer);
			result.push_back(buffer.GetString());
		}
	}
	
	return result;
}

// Helper: Convert map to JSON object
std::string stringMapToJson(const std::map<std::string, std::string>& map) {
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	
	writer.StartObject();
	for (const auto& pair : map) {
		writer.Key(pair.first.c_str());
		writer.String(pair.second.c_str());
	}
	writer.EndObject();
	
	return std::string(buffer.GetString());
}

// Helper: Parse JSON object to map
bool jsonToStringMap(const std::string& json_str, std::map<std::string, std::string>& out_map) {
	if (json_str.empty()) return false;
	
	Document doc;
	doc.Parse(json_str.c_str());
	
	if (doc.HasParseError() || !doc.IsObject()) {
		return false;
	}
	
	out_map.clear();
	for (Value::ConstMemberIterator itr = doc.MemberBegin(); 
	     itr != doc.MemberEnd(); ++itr) {
		if (itr->value.IsString()) {
			out_map[itr->name.GetString()] = itr->value.GetString();
		} else {
			// Convert non-string to string representation
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);
			itr->value.Accept(writer);
			out_map[itr->name.GetString()] = buffer.GetString();
		}
	}
	
	return true;
}

} // namespace nPythonPlugin
} // namespace nVerliHub

#endif // HAVE_RAPIDJSON
