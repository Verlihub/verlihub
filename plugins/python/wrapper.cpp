/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2025 Verlihub Team, info at verlihub dot net

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

#include "wrapper.h"
#include "json_marshal.h"
#include "src/cserverdc.h"
#include "src/cban.h"
#include "cpipython.h"  // For cpiPython::me->server

#include <libgen.h>  // for basename, dirname
#include <cstring>   // for strrchr
#include <time.h>    // for nanosleep
#include <signal.h>  // for sig_atomic_t
#include <pthread.h> // for pthread_mutex_trylock
#include <errno.h>   // for EBUSY

using namespace std;
using namespace nVerliHub::nEnums;
using namespace nVerliHub::nPythonPlugin;

vector<w_TScript *> w_Scripts;
static vector<bool> w_Scripts_had_threads;  // Track which scripts used threading (persists after w_Unload)

static int log_level = 0;

#define pybool(a) ((a) ? ({ Py_INCREF(Py_True); Py_True; }) : ({ Py_INCREF(Py_False); Py_False; }))
#define pybool2(a) ((a) ? ({ free(a); Py_INCREF(Py_True); Py_True; }) : ({ Py_INCREF(Py_False); Py_False; }))

// logging levels:
// 0 - plugin critical errors, interpreter errors, bad call arguments
// 1 - callback / hook logging - only their status
// 2 - all function parameters and return values are printed
#define log(...)                      { printf( __VA_ARGS__ ); fflush(stdout); }
#define log1(...)  if (log_level > 0) { printf( __VA_ARGS__ ); fflush(stdout); }
#define log2(...)  if (log_level > 1) { printf( __VA_ARGS__ ); fflush(stdout); }
#define log3(...)  if (log_level > 2) { printf( __VA_ARGS__ ); fflush(stdout); }
#define log4(...)  if (log_level > 3) { printf( __VA_ARGS__ ); fflush(stdout); }
#define log5(...)  if (log_level > 4) { printf( __VA_ARGS__ ); fflush(stdout); }
#define log6(...)  if (log_level > 5) { printf( __VA_ARGS__ ); fflush(stdout); }

void w_LogLevel(int level) { log_level = level; }

//==============================================================================
// Encoding Conversion Helpers (Hub encoding <-> UTF-8 for Python)
//==============================================================================

// Note: Verlihub's cICUConvert::Convert() converts FROM UTF-8 TO hub_encoding
// and cICUConvert::ConvertReverse() converts FROM hub_encoding TO UTF-8.
// This provides full round-trip conversion for the Python/C++ boundary.

// Helper: Convert string from UTF-8 (from Python) to hub encoding
static std::string Utf8ToHub(const std::string& utf8_str) {
	if (utf8_str.empty())
		return utf8_str;
	
	// Check if we have a server and ICU converter
	if (!cpiPython::me || !cpiPython::me->server || !cpiPython::me->server->mICUConvert)
		return utf8_str; // Return as-is if no converter available
	
	std::string hub_enc = cpiPython::me->server->mC.hub_encoding;
	if (hub_enc.empty() || hub_enc == "UTF-8" || hub_enc == "utf-8")
		return utf8_str; // Already UTF-8
	
	// Use Verlihub's existing ICU converter: Convert(UTF-8 input) -> hub encoding output
	std::string converted;
	if (cpiPython::me->server->mICUConvert->Convert(utf8_str.c_str(), utf8_str.length(), converted)) {
		return converted;
	}
	
	// Fallback to original string if conversion fails
	return utf8_str;
}

// Helper: Convert string from hub encoding to UTF-8 (for returning to Python)
static std::string HubToUtf8(const std::string& hub_str) {
	if (hub_str.empty())
		return hub_str;
	
	// Check if we have a server and ICU converter
	if (!cpiPython::me || !cpiPython::me->server || !cpiPython::me->server->mICUConvert)
		return hub_str; // Return as-is if no converter available
	
	std::string hub_enc = cpiPython::me->server->mC.hub_encoding;
	log3("PY: HubToUtf8: hub_encoding='%s', input_len=%zu\n", hub_enc.c_str(), hub_str.length());
	
	if (hub_enc.empty() || hub_enc == "UTF-8" || hub_enc == "utf-8")
		return hub_str; // Already UTF-8
	
	// Use Verlihub's ICU converter: ConvertReverse(hub encoding input) -> UTF-8 output
	std::string converted;
	if (cpiPython::me->server->mICUConvert->ConvertReverse(hub_str.c_str(), hub_str.length(), converted)) {
		return converted;
	}

	// Fallback to original string if conversion fails
	return hub_str;
}

// Function is similar to Python's [_start:_end] slice
char *w_SubStr(const char *s, int _start, int _end)
{
	int start = _start, end = _end, len = strlen(s), len2;
	char *s2;

	if (start < 0)
		start = 0;

	if (start >= len)
		return strdup("");

	if (end < 0)
		end = len + end;

	if (end == 0) // Different than in Python: here [0:0] will return the whole string
		end = len;

	if (end <= start)
		return strdup("");

	if (end > len)
		end = len;

	len2 = end - start;
	s2 = (char*)malloc(len2 + 1);

	if (!s2)
		return strdup("");

	s2[len2] = 0;
	s2 = strncpy(s2, &s[start], len2);
	return s2;
}

int w_IdentStr(const char *s1, const char *s2, int n)
{
	int len1 = strlen(s1), len2 = strlen(s2);

	if (n > 0 && n < len1)
		len1 = n;

	if (n > 0 && n < len2)
		len2 = n;

	if (len1 != len2)
		return 0;

	for (int i = 0; i < len1; i++) {
		if (s1[i] != s2[i])
			return 0;
	}

	return 1;
}

int w_FindStr(const char *s, const char *key, int start)
{
	if (start < 0)
		start = 0;

	int len = strlen(s), len1 = strlen(key), len2;

	if ((len1 > len) || (len1 == 0) || (len == 0))
		return -1;

	len2 = len - len1 + 1;

	for (int i = start; i < len2; i++) {
		if ((s[i] == key[0]) && (w_IdentStr(&s[i], key, len1)))
			return i;
	}

	return -1;
}


w_TScript *w_Python = NULL;


// Call accepts only the following in_format PyArg_ParseTuple format characters:
// "s" (string or Unicode object) [const char *] -- Call treats "s" like "z" so object can be string or None!
// "z" (string or None) [const char *]
// "l" (integer) [long int]
// "d" (float) [double]
// "O" (object) [PyObject *]
// "|" remaining arguments are optional
// ":" end of format string
// ";" end of format string

w_Targs *w_vapack(const char *format, va_list ap)
{
	w_Targs *a;
	unsigned int flen = strlen(format);

	for (unsigned int i = 0; i < flen; i++) {
		switch (format[i]) {
			case 'l':
			case 's':
			case 'd':
			case 'p':
			case 'L':  //  List of strings
			case 'D':  //  Dictionary (as JSON string)
			case 'O':  //  PyObject* (advanced)
				break;
			default:
				log1("PY: pack: format string supports 'lsdpLDO' and not '%c'\n", format[i]);
				return NULL;
		}
	}

	// Allocate w_Targs with flexible array member
	// Need sizeof(w_Targs) for the header + space for 'flen' elements
	a = (w_Targs*)calloc(1, sizeof(w_Targs) + flen * sizeof(w_Telement));

	if (!a)
		return NULL;

	// Make a copy of the format string since it might be a string literal
	// This copy will be freed by w_free_args()
	a->format = strdup(format);

	for (unsigned int i = 0; i < flen; i++) {
		switch (format[i]) {
			case 'l':
				a->args[i].type = 'l';
				a->args[i].l = va_arg(ap, long);
				break;
			case 's':
				a->args[i].type = 's';
				a->args[i].s = va_arg(ap, char *);
				break;
			case 'd':
				a->args[i].type = 'd';
				a->args[i].d = va_arg(ap, double);
				break;
			case 'p':
				a->args[i].type = 'p';
				a->args[i].p = va_arg(ap, void *);
				break;
			case 'L':  //  List of strings (NULL-terminated char**)
				a->args[i].type = 'L';
				a->args[i].L = va_arg(ap, char **);
				break;
			case 'D':  //  Dict as JSON string (treated as 's' internally)
				a->args[i].type = 'D';
				a->args[i].s = va_arg(ap, char *);
				break;
			case 'O':  //  PyObject* passthrough
				a->args[i].type = 'O';
				a->args[i].O = va_arg(ap, PyObject *);
				break;
		}
	}

	return a;
}

w_Targs *w_pack(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	w_Targs *a = w_vapack(format, ap);
	va_end(ap);
	return a;
}

int w_unpack(w_Targs *a, const char *format, ...)
{
	if (!a)
		return 0;

	//TODO: make this more generic instead of handling just this one case.
	// Handle optional parameters: "ss|s" means 2 required + 1 optional
	// Check if formats match, considering optional params
	if (a->format && format) {
		const char *pipe_pos = strchr(format, '|');
		if (pipe_pos) {
			// Format has optional params: "ss|s"
			// a->format might be "ss" (2 args) or "sss" (3 args)
			size_t required_len = pipe_pos - format;
			size_t actual_len = strlen(a->format);
			size_t total_len = strlen(format) - 1; // -1 for the '|'
			
			if (actual_len < required_len || actual_len > total_len) {
				log1("PY: unpack: format strings don't match: '%s' vs. '%s'\n", a->format, format);
				return 0;
			}
		} else if (strcmp(a->format, format) != 0) {
			log1("PY: unpack: format strings don't match: '%s' vs. '%s'\n", a->format, format);
			return 0;
		}
	}

	unsigned int flen = strlen(format);
	va_list ap;
	va_start(ap, format);

	// Calculate actual number of args from a->format (without '|')
	unsigned int actual_args = a->format ? strlen(a->format) : 0;
	
	// Track actual arg index (format may have '|' which doesn't consume an arg)
	unsigned int arg_idx = 0;
	for (unsigned int i = 0; i < flen; i++) {
		// Skip the '|' separator for optional params
		if (format[i] == '|') {
			continue;
		}
		
		// Check if we're trying to access an arg that wasn't provided
		if (arg_idx >= actual_args) {
			// Optional param not provided - write NULL/0 to the pointer
			switch (format[i]) {
				case 'l':
					*va_arg(ap, long *) = 0;
					break;
				case 's':
				case 'D':
					*va_arg(ap, char **) = NULL;
					break;
				case 'd':
					*va_arg(ap, double *) = 0.0;
					break;
				case 'p':
					*va_arg(ap, void **) = NULL;
					break;
				case 'L':
					*va_arg(ap, char ***) = NULL;
					break;
				case 'O':
					*va_arg(ap, PyObject **) = NULL;
					break;
			}
			continue;
		}
		
		switch (format[i]) {
			case 'l':
				*va_arg(ap, long *) = a->args[arg_idx].l;
				break;
			case 's':
				*va_arg(ap, char **) = a->args[arg_idx].s;
				break;
			case 'd':
				*va_arg(ap, double *) = a->args[arg_idx].d;
				break;
			case 'p':
				*va_arg(ap, void **) = a->args[arg_idx].p;
				break;
			case 'L':  //  List of strings
				*va_arg(ap, char ***) = a->args[arg_idx].L;
				break;
			case 'D':  //  Dict as JSON (unpacked as string)
				*va_arg(ap, char **) = a->args[arg_idx].s;
				break;
			case 'O':  //  PyObject* passthrough
				*va_arg(ap, PyObject **) = a->args[arg_idx].O;
				break;
			default:
				va_end(ap);
				return 0;
		}
		arg_idx++;
	}

	va_end(ap);
	return 1;
}

const char *w_packprint(w_Targs *a)
{
	if (!a) return "(null)";

	static char buf[1024];
	buf[0] = 0;
	char tmp[256];
	if (a->format) strcat(buf, a->format);
	strcat(buf, " [ ");
	for (unsigned int i = 0; i < strlen(a->format); i++) {
		switch (a->format[i]) {
			case 'l':
				snprintf(tmp, sizeof(tmp), "%ld ", a->args[i].l);
				strcat(buf, tmp);
				break;
			case 's':
				snprintf(tmp, sizeof(tmp), "'%s' ", a->args[i].s);
				strcat(buf, tmp);
				break;
			case 'd':
				snprintf(tmp, sizeof(tmp), "%f ", a->args[i].d);
				strcat(buf, tmp);
				break;
			case 'p':
				snprintf(tmp, sizeof(tmp), "%p ", a->args[i].p);
				strcat(buf, tmp);
				break;
			case 'L':  //  List of strings
				{
					strcat(buf, "[");
					if (a->args[i].L) {
						for (int j = 0; a->args[i].L[j] != NULL; j++) {
							if (j > 0) strcat(buf, ", ");
							snprintf(tmp, sizeof(tmp), "'%s'", a->args[i].L[j]);
							strcat(buf, tmp);
							if (j >= 4) {  // Limit display to avoid overflow
								strcat(buf, ", ...");
								break;
							}
						}
					}
					strcat(buf, "] ");
				}
				break;
			case 'D':  //  Dict as JSON string
				snprintf(tmp, sizeof(tmp), "JSON:'%.50s%s' ", 
					a->args[i].s ? a->args[i].s : "",
					(a->args[i].s && strlen(a->args[i].s) > 50) ? "..." : "");
				strcat(buf, tmp);
				break;
			case 'O':  //  PyObject*
				snprintf(tmp, sizeof(tmp), "PyObject:%p ", a->args[i].O);
				strcat(buf, tmp);
				break;
		}
	}
	strcat(buf, "]");
	return buf;
}

// Free a w_Targs structure and all dynamically allocated memory within it
void w_free_args(w_Targs *a)
{
	if (!a) return;
	
	if (a->format) {
		for (size_t i = 0; i < strlen(a->format); i++) {
			switch (a->format[i]) {
				case 's':  // String
				case 'D':  // Dict (as JSON string)
					if (a->args[i].s) {
						free(a->args[i].s);
						a->args[i].s = NULL;
					}
					break;
				case 'L':  // List of strings
					if (a->args[i].L) {
						for (int j = 0; a->args[i].L[j] != NULL; j++) {
							free(a->args[i].L[j]);
						}
						free(a->args[i].L);
						a->args[i].L = NULL;
					}
					break;
				// Other types (l, d, p, O) don't need cleanup
			}
		}
		free((char*)a->format);
	}
	free(a);
}

#ifdef HAVE_RAPIDJSON
//==============================================================================
// JSON Marshaling Helpers for Python-C++ Bidirectional Communication
//==============================================================================

namespace nVerliHub {
namespace nPythonPlugin {

// Convert PyObject to JsonValue (recursive)
bool PyObjectToJsonValue(PyObject* obj, JsonValue& out_val)
{
	if (obj == Py_None) {
		out_val.type = JsonType::NULL_TYPE;
		return true;
	}
	
	if (PyBool_Check(obj)) {
		out_val.type = JsonType::BOOL;
		out_val.bool_val = (obj == Py_True);
		return true;
	}
	
	if (PyLong_Check(obj)) {
		out_val.type = JsonType::INT;
		out_val.int_val = PyLong_AsLongLong(obj);
		if (PyErr_Occurred()) {
			PyErr_Clear();
			return false;
		}
		return true;
	}
	
	if (PyFloat_Check(obj)) {
		out_val.type = JsonType::DOUBLE;
		out_val.double_val = PyFloat_AsDouble(obj);
		return true;
	}
	
	if (PyUnicode_Check(obj)) {
		out_val.type = JsonType::STRING;
		const char* s = PyUnicode_AsUTF8(obj);
		if (!s) {
			PyErr_Clear();
			return false;
		}
		// Convert from UTF-8 (Python) to hub encoding (C++)
		out_val.string_val = Utf8ToHub(s);
		return true;
	}
	
	if (PyList_Check(obj)) {
		out_val.type = JsonType::ARRAY;
		out_val.array_val.clear();
		
		Py_ssize_t size = PyList_Size(obj);
		out_val.array_val.reserve(size);
		
		for (Py_ssize_t i = 0; i < size; i++) {
			PyObject* item = PyList_GetItem(obj, i);
			JsonValue elem;
			if (!PyObjectToJsonValue(item, elem)) {
				return false;
			}
			out_val.array_val.push_back(elem);
		}
		return true;
	}
	
	if (PyTuple_Check(obj)) {
		out_val.type = JsonType::TUPLE;
		out_val.tuple_val.clear();
		
		Py_ssize_t size = PyTuple_Size(obj);
		out_val.tuple_val.reserve(size);
		
		for (Py_ssize_t i = 0; i < size; i++) {
			PyObject* item = PyTuple_GetItem(obj, i);
			JsonValue elem;
			if (!PyObjectToJsonValue(item, elem)) {
				return false;
			}
			out_val.tuple_val.push_back(elem);
		}
		return true;
	}
	
	if (PySet_Check(obj) || PyFrozenSet_Check(obj)) {
		out_val.type = JsonType::SET;
		out_val.set_val.clear();
		
		PyObject* iterator = PyObject_GetIter(obj);
		if (!iterator) {
			PyErr_Clear();
			return false;
		}
		
		PyObject* item;
		while ((item = PyIter_Next(iterator))) {
			JsonValue elem;
			if (!PyObjectToJsonValue(item, elem)) {
				Py_DECREF(item);
				Py_DECREF(iterator);
				return false;
			}
			out_val.set_val.insert(elem);
			Py_DECREF(item);
		}
		Py_DECREF(iterator);
		
		if (PyErr_Occurred()) {
			PyErr_Clear();
			return false;
		}
		return true;
	}
	
	if (PyDict_Check(obj)) {
		out_val.type = JsonType::OBJECT;
		out_val.object_val.clear();
		
		PyObject *key, *value;
		Py_ssize_t pos = 0;
		
		while (PyDict_Next(obj, &pos, &key, &value)) {
			// Keys must be strings
			if (!PyUnicode_Check(key)) {
				return false;
			}
			
			const char* key_str = PyUnicode_AsUTF8(key);
			if (!key_str) {
				PyErr_Clear();
				return false;
			}
			
			JsonValue elem;
			if (!PyObjectToJsonValue(value, elem)) {
				return false;
			}
			
			out_val.object_val[key_str] = elem;
		}
		return true;
	}
	
	// Unsupported type
	return false;
}

// Convert JsonValue to PyObject (recursive)
PyObject* JsonValueToPyObject(const JsonValue& val)
{
	switch (val.type) {
		case JsonType::NULL_TYPE:
			Py_RETURN_NONE;
		
		case JsonType::BOOL:
			if (val.bool_val) {
				Py_RETURN_TRUE;
			} else {
				Py_RETURN_FALSE;
			}
		
		case JsonType::INT:
			return PyLong_FromLongLong(val.int_val);
		
		case JsonType::DOUBLE:
			return PyFloat_FromDouble(val.double_val);
		
		case JsonType::STRING: {
			// Convert from hub encoding to UTF-8 for Python
			std::string utf8_str = HubToUtf8(val.string_val);
			return PyUnicode_FromString(utf8_str.c_str());
		}
		
		case JsonType::ARRAY: {
			PyObject* list = PyList_New(val.array_val.size());
			if (!list) return nullptr;
			
			for (size_t i = 0; i < val.array_val.size(); i++) {
				PyObject* item = JsonValueToPyObject(val.array_val[i]);
				if (!item) {
					Py_DECREF(list);
					return nullptr;
				}
				PyList_SET_ITEM(list, i, item);  // Steals reference
			}
			return list;
		}
		
		case JsonType::TUPLE: {
			PyObject* tuple = PyTuple_New(val.tuple_val.size());
			if (!tuple) return nullptr;
			
			for (size_t i = 0; i < val.tuple_val.size(); i++) {
				PyObject* item = JsonValueToPyObject(val.tuple_val[i]);
				if (!item) {
					Py_DECREF(tuple);
					return nullptr;
				}
				PyTuple_SET_ITEM(tuple, i, item);  // Steals reference
			}
			return tuple;
		}
		
		case JsonType::SET: {
			PyObject* set = PySet_New(nullptr);
			if (!set) return nullptr;
			
			for (const auto& elem : val.set_val) {
				PyObject* item = JsonValueToPyObject(elem);
				if (!item) {
					Py_DECREF(set);
					return nullptr;
				}
				
				if (PySet_Add(set, item) < 0) {
					Py_DECREF(item);
					Py_DECREF(set);
					return nullptr;
				}
				Py_DECREF(item);
			}
			return set;
		}
		
		case JsonType::OBJECT: {
			PyObject* dict = PyDict_New();
			if (!dict) return nullptr;
			
			for (const auto& pair : val.object_val) {
				PyObject* key = PyUnicode_FromString(pair.first.c_str());
				PyObject* value = JsonValueToPyObject(pair.second);
				
				if (!key || !value) {
					Py_XDECREF(key);
					Py_XDECREF(value);
					Py_DECREF(dict);
					return nullptr;
				}
				
				PyDict_SetItem(dict, key, value);
				Py_DECREF(key);
				Py_DECREF(value);
			}
			return dict;
		}
		
		default:
			Py_RETURN_NONE;
	}
}

// Convert PyObject to JSON string
char* PyObjectToJsonString(PyObject* obj)
{
	JsonValue val;
	if (!PyObjectToJsonValue(obj, val)) {
		return nullptr;
	}
	
	std::string json = toJsonString(val);
	return strdup(json.c_str());
}

// Convert JSON string to PyObject
PyObject* JsonStringToPyObject(const char* json_str)
{
	if (!json_str) {
		Py_RETURN_NONE;
	}
	
	JsonValue val;
	if (!parseJson(json_str, val)) {
		return nullptr;
	}
	
	return JsonValueToPyObject(val);
}

} // namespace nPythonPlugin
} // namespace nVerliHub

#endif // HAVE_RAPIDJSON

//==============================================================================
// vh Module Implementation
//==============================================================================

// Helper: Get script ID from vh.myid attribute
static long vh_GetScriptID()
{
	PyObject *m = PyDict_GetItemString(PyImport_GetModuleDict(), "vh");
	if (!m) return -1;
	
	if (!PyObject_HasAttrString(m, "myid")) return -1;
	
	PyObject *f = PyObject_GetAttrString(m, "myid");
	if (!f) return -1;
	
	if (!PyLong_Check(f)) {
		Py_DECREF(f);
		return -1;
	}
	
	long id = PyLong_AsLong(f);
	Py_DECREF(f);
	
	if ((id > -1) && ((unsigned long)id < w_Scripts.size()) && w_Scripts[id])
		return id;
	
	return -1;
}

// Helper: Parse Python args to w_Targs (Python 3 version)
// Format: l=long, s=string, d=double, |=optional args start
static int vh_ParseArgs(int func, PyObject *args, const char *in_format, w_Targs **out_args)
{
	long id = vh_GetScriptID();
	if (id < 0) {
		PyErr_SetString(PyExc_RuntimeError, "Could not determine script ID");
		return 0;
	}
	
	const char *name = w_Scripts[id]->name;
	
	// Parse format string
	int in_len = strlen(in_format);
	char *pack_format = (char*)malloc(in_len + 1);
	if (!pack_format) {
		PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory");
		return 0;
	}
	
	int required = -1;
	int pos2 = 0;
	
	for (int pos = 0; pos < in_len; pos++) {
		switch (in_format[pos]) {
			case 'l':
			case 's':
			case 'd':
				pack_format[pos2++] = in_format[pos];
				break;
			case '|':
				if (required >= 0) {
					free(pack_format);
					PyErr_SetString(PyExc_ValueError, "Multiple '|' in format string");
					return 0;
				}
				required = pos2;
				break;
			default:
				free(pack_format);
				PyErr_Format(PyExc_ValueError, "Unsupported format character '%c'", in_format[pos]);
				return 0;
		}
	}
	pack_format[pos2] = '\0';
	
	int len = pos2;
	if (required < 0) required = len;
	
	int tlen = PyTuple_Size(args);
	
	if (tlen < required) {
		free(pack_format);
		PyErr_Format(PyExc_TypeError, "Function requires at least %d arguments, got %d", required, tlen);
		return 0;
	}
	
	if (tlen > len) {
		free(pack_format);
		PyErr_Format(PyExc_TypeError, "Function takes at most %d arguments, got %d", len, tlen);
		return 0;
	}
	
	// Allocate w_Targs with flexible array member
	// Need sizeof(w_Targs) for the header + space for 'len' elements
	w_Targs *a = (w_Targs*)calloc(1, sizeof(w_Targs) + len * sizeof(w_Telement));
	if (!a) {
		free(pack_format);
		PyErr_SetString(PyExc_MemoryError, "Failed to allocate argument structure");
		return 0;
	}
	
	a->format = pack_format;
	
	// Parse each argument
	for (int i = 0; i < len; i++) {
		if (i >= tlen) {
			// Optional argument not provided
			switch (pack_format[i]) {
				case 'l':
					a->args[i].type = 'l';
					a->args[i].l = 0;
					break;
				case 's':
					a->args[i].type = 's';
					a->args[i].s = NULL;
					break;
				case 'd':
					a->args[i].type = 'd';
					a->args[i].d = 0.0;
					break;
			}
			continue;
		}
		
		PyObject *p = PyTuple_GetItem(args, i);
		if (!p) {
			free(pack_format);
			free(a);
			PyErr_Format(PyExc_RuntimeError, "Failed to get argument %d", i);
			return 0;
		}
		
		switch (pack_format[i]) {
			case 'l':
				a->args[i].type = 'l';
				if (p == Py_None) {
					a->args[i].l = 0;
				} else if (PyLong_Check(p)) {
					a->args[i].l = PyLong_AsLong(p);
				} else {
					free(pack_format);
					free(a);
					PyErr_Format(PyExc_TypeError, "Argument %d must be int, not %s", i, p->ob_type->tp_name);
					return 0;
				}
				break;
				
			case 's':
				a->args[i].type = 's';
				if (p == Py_None) {
					a->args[i].s = NULL;
				} else if (PyUnicode_Check(p)) {
					const char* utf8_str = PyUnicode_AsUTF8(p);
					if (!utf8_str) {
						free(pack_format);
						free(a);
						return 0;  // PyUnicode_AsUTF8 set exception
					}
					// Convert from UTF-8 to hub encoding
					std::string hub_str = Utf8ToHub(utf8_str);
					// Allocate and copy converted string (will be freed by callback handler)
					a->args[i].s = strdup(hub_str.c_str());
				} else {
					free(pack_format);
					free(a);
					PyErr_Format(PyExc_TypeError, "Argument %d must be str, not %s", i, p->ob_type->tp_name);
					return 0;
				}
				break;
				
			case 'd':
				a->args[i].type = 'd';
				if (p == Py_None) {
					a->args[i].d = 0.0;
				} else if (PyFloat_Check(p)) {
					a->args[i].d = PyFloat_AsDouble(p);
				} else if (PyLong_Check(p)) {
					a->args[i].d = (double)PyLong_AsLong(p);
				} else {
					free(pack_format);
					free(a);
					PyErr_Format(PyExc_TypeError, "Argument %d must be float, not %s", i, p->ob_type->tp_name);
					return 0;
				}
				break;
		}
	}
	
	*out_args = a;
	return 1;
}

// Helper: Call C++ callback and return bool
static PyObject* vh_CallBool(int func, PyObject *args, const char *in_format)
{
	w_Targs *a = NULL;
	if (!vh_ParseArgs(func, args, in_format, &a))
		return NULL;
	
	// Release GIL while calling C++
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[func](func, a);
	
	PyEval_AcquireThread(state);
	
	// Free allocated strings from vh_ParseArgs AFTER callback returns
	for (int i = 0; a->format && a->format[i]; i++) {
		if (a->format[i] == 's' && a->args[i].s) {
			free(a->args[i].s);
		}
	}
	
	free((char*)a->format);
	free(a);
	
	if (!res) {
		Py_RETURN_FALSE;
	}
	
	long ret = 0;
	w_unpack(res, "l", &ret);
	w_free_args(res);
	
	if (ret) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

// Helper: Call C++ callback and return string
static PyObject* vh_CallString(int func, PyObject *args, const char *in_format)
{
	w_Targs *a = NULL;
	if (!vh_ParseArgs(func, args, in_format, &a))
		return NULL;
	
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[func](func, a);
	
	PyEval_AcquireThread(state);
	
	// Free allocated strings from vh_ParseArgs AFTER callback returns
	for (int i = 0; a->format && a->format[i]; i++) {
		if (a->format[i] == 's' && a->args[i].s) {
			free(a->args[i].s);
		}
	}
	
	free((char*)a->format);
	free(a);
	
	if (!res) {
		Py_RETURN_NONE;
	}
	
	char *ret = NULL;
	w_unpack(res, "s", &ret);
	
	PyObject *py_ret;
	if (ret && ret[0]) {
		// Convert from hub encoding to UTF-8 for Python
		// Make a copy since w_free_args will free ret
		std::string ret_copy(ret);
		std::string utf8_str = HubToUtf8(ret_copy);
		py_ret = PyUnicode_FromString(utf8_str.c_str());
	} else {
		Py_INCREF(Py_None);
		py_ret = Py_None;
	}
	
	w_free_args(res);  // Changed from free(res) to properly free strings
	return py_ret;
}

// Helper: Call C++ callback and return long
static PyObject* vh_CallLong(int func, PyObject *args, const char *in_format)
{
	w_Targs *a = NULL;
	if (!vh_ParseArgs(func, args, in_format, &a))
		return NULL;
	
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[func](func, a);
	
	PyEval_AcquireThread(state);
	
	// Free allocated strings from vh_ParseArgs AFTER callback returns
	for (int i = 0; a->format && a->format[i]; i++) {
		if (a->format[i] == 's' && a->args[i].s) {
			free(a->args[i].s);
		}
	}
	
	free((char*)a->format);
	free(a);
	
	if (!res) {
		Py_RETURN_NONE;
	}
	
	long ret = 0;
	w_unpack(res, "l", &ret);
	w_free_args(res);
	
	return PyLong_FromLong(ret);
}

//==============================================================================
// Forward declarations for vh module functions
//==============================================================================
static PyObject* vh_Encode(PyObject *self, PyObject *args);
static PyObject* vh_Decode(PyObject *self, PyObject *args);
static PyObject* vh_CallDynamicFunction(PyObject *self, PyObject *args);

//==============================================================================
// vh Module Functions
//==============================================================================

static PyObject* vh_SendToOpChat(PyObject *self, PyObject *args)       { return vh_CallBool(W_SendToOpChat, args, "s|s"); }
static PyObject* vh_SendToActive(PyObject *self, PyObject *args)       { return vh_CallBool(W_SendToActive, args, "s|l"); }
static PyObject* vh_SendToPassive(PyObject *self, PyObject *args)      { return vh_CallBool(W_SendToPassive, args, "s|l"); }
static PyObject* vh_SendToActiveClass(PyObject *self, PyObject *args)  { return vh_CallBool(W_SendToActiveClass, args, "sll|l"); }
static PyObject* vh_SendToPassiveClass(PyObject *self, PyObject *args) { return vh_CallBool(W_SendToPassiveClass, args, "sll|l"); }
static PyObject* vh_SendDataToUser(PyObject *self, PyObject *args)     { return vh_CallBool(W_SendDataToUser, args, "ss|l"); }
static PyObject* vh_SendDataToAll(PyObject *self, PyObject *args)      { return vh_CallBool(W_SendDataToAll, args, "sll|l"); }
static PyObject* vh_SendPMToAll(PyObject *self, PyObject *args)        { return vh_CallBool(W_SendPMToAll, args, "ssll"); }
static PyObject* vh_CloseConnection(PyObject *self, PyObject *args)    { return vh_CallBool(W_CloseConnection, args, "s|ll"); }

// GetMyINFO - returns tuple of (nick, desc, tag, speed, email, sharesize)
static PyObject* vh_GetMyINFO(PyObject *self, PyObject *args)
{
	w_Targs *a = NULL;
	if (!vh_ParseArgs(W_GetMyINFO, args, "s", &a))
		return NULL;
	
	// Don't free the strings yet - they're needed by the callback!
	// The callback will handle freeing via w_free_args()
	
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[W_GetMyINFO](W_GetMyINFO, a);
	
	// Now free the input args
	w_free_args(a);
	
	PyEval_AcquireThread(state);
	
	if (!res) {
		Py_RETURN_NONE;
	}
	
	// Unpack all 6 strings: nick, desc, tag, speed, email, sharesize
	char *nick = NULL, *desc = NULL, *tag = NULL, *speed = NULL, *email = NULL, *size = NULL;
	w_unpack(res, "ssssss", &nick, &desc, &tag, &speed, &email, &size);
	
	// Create Python tuple
	PyObject *tuple = PyTuple_New(6);
	if (!tuple) {
		w_free_args(res);
		return NULL;
	}
	
	// Helper macro to safely create PyUnicode with encoding conversion
	#define SET_TUPLE_STRING(index, str) do { \
		std::string utf8_str = (str) ? HubToUtf8(str) : ""; \
		PyObject *py_str = PyUnicode_FromString(utf8_str.c_str()); \
		if (!py_str) { \
			Py_DECREF(tuple); \
			w_free_args(res); \
			return NULL; \
		} \
		PyTuple_SetItem(tuple, index, py_str); \
	} while(0)
	
	SET_TUPLE_STRING(0, nick);
	SET_TUPLE_STRING(1, desc);
	SET_TUPLE_STRING(2, tag);
	SET_TUPLE_STRING(3, speed);
	SET_TUPLE_STRING(4, email);
	SET_TUPLE_STRING(5, size);
	
	#undef SET_TUPLE_STRING
	
	w_free_args(res);
	return tuple;
}

static PyObject* vh_SetMyINFO(PyObject *self, PyObject *args)          { return vh_CallBool(W_SetMyINFO, args, "s|sssss"); }
static PyObject* vh_GetUserClass(PyObject *self, PyObject *args)       { return vh_CallLong(W_GetUserClass, args, "s"); }
static PyObject* vh_GetRawNickList(PyObject *self, PyObject *args)     { return vh_CallString(W_GetNickList, args, ""); }
static PyObject* vh_GetRawOpList(PyObject *self, PyObject *args)       { return vh_CallString(W_GetOpList, args, ""); }
static PyObject* vh_GetRawBotList(PyObject *self, PyObject *args)      { return vh_CallString(W_GetBotList, args, ""); }
static PyObject* vh_GetUserHost(PyObject *self, PyObject *args)        { return vh_CallString(W_GetUserHost, args, "s"); }
static PyObject* vh_GetUserIP(PyObject *self, PyObject *args)          { return vh_CallString(W_GetUserIP, args, "s"); }
static PyObject* vh_SetUserIP(PyObject *self, PyObject *args)          { return vh_CallBool(W_SetUserIP, args, "ss"); }
static PyObject* vh_SetMyINFOFlag(PyObject *self, PyObject *args)      { return vh_CallBool(W_SetMyINFOFlag, args, "sl"); }
static PyObject* vh_UnsetMyINFOFlag(PyObject *self, PyObject *args)    { return vh_CallBool(W_UnsetMyINFOFlag, args, "sl"); }
static PyObject* vh_GetUserHubURL(PyObject *self, PyObject *args)      { return vh_CallString(W_GetUserHubURL, args, "s"); }
static PyObject* vh_GetUserExtJSON(PyObject *self, PyObject *args)     { return vh_CallString(W_GetUserExtJSON, args, "s"); }
static PyObject* vh_GetUserCC(PyObject *self, PyObject *args)          { return vh_CallString(W_GetUserCC, args, "s"); }
static PyObject* vh_GetIPCC(PyObject *self, PyObject *args)            { return vh_CallString(W_GetIPCC, args, "s"); }
static PyObject* vh_GetIPCN(PyObject *self, PyObject *args)            { return vh_CallString(W_GetIPCN, args, "s"); }
static PyObject* vh_GetIPASN(PyObject *self, PyObject *args)           { return vh_CallString(W_GetIPASN, args, "s|s"); }

// GetGeoIP returns a dictionary with geographic data
static PyObject* vh_GetGeoIP(PyObject *self, PyObject *args)
{
	// Parse Python arguments (IP required, DB optional)
	const char *ip = NULL, *db = NULL;
	if (!PyArg_ParseTuple(args, "s|s", &ip, &db))
		return NULL;

	// Pack arguments for callback
	w_Targs *call_args = w_pack(db ? "ss" : "s", ip, db);
	if (!call_args)
		Py_RETURN_NONE;

	// Call the C++ callback
	w_Targs *res = w_Python->callbacks[W_GetGeoIP](W_GetGeoIP, call_args);
	if (!res)
		Py_RETURN_NONE;

	// Unpack the complex structure: 'sdsdslslp'
	// Format: latitude(s,d) longitude(s,d) metro_code(s,l) area_code(s,l) data(p)
	char *lat_key, *lon_key, *metro_key, *area_key;
	double latitude, longitude;
	long metro_code, area_code;
	void *data_ptr;

	if (!w_unpack(res, "sdsdslslp", 
	              &lat_key, &latitude,
	              &lon_key, &longitude,
	              &metro_key, &metro_code,
	              &area_key, &area_code,
	              &data_ptr)) {
		free(res);
		Py_RETURN_NONE;
	}

	// Create Python dictionary
	PyObject *dict = PyDict_New();
	if (!dict) {
		free(res);
		return NULL;
	}

	// Add numeric fields
	PyDict_SetItemString(dict, "latitude", PyFloat_FromDouble(latitude));
	PyDict_SetItemString(dict, "longitude", PyFloat_FromDouble(longitude));
	PyDict_SetItemString(dict, "metro_code", PyLong_FromLong(metro_code));
	PyDict_SetItemString(dict, "area_code", PyLong_FromLong(area_code));

	// Extract string fields from vector
	if (data_ptr) {
		std::vector<std::string> *data = static_cast<std::vector<std::string>*>(data_ptr);
		// Vector contains key-value pairs as consecutive strings
		for (size_t i = 0; i + 1 < data->size(); i += 2) {
			const char *key = (*data)[i].c_str();
			const char *value = (*data)[i + 1].c_str();
			PyDict_SetItemString(dict, key, PyUnicode_FromString(HubToUtf8(value).c_str()));
		}
	}

	free(res);
	return dict;
}
static PyObject* vh_AddRegUser(PyObject *self, PyObject *args)         { return vh_CallBool(W_AddRegUser, args, "sl|ss"); }
static PyObject* vh_DelRegUser(PyObject *self, PyObject *args)         { return vh_CallBool(W_DelRegUser, args, "s"); }
static PyObject* vh_SetRegClass(PyObject *self, PyObject *args)        { return vh_CallBool(W_SetRegClass, args, "sl"); }
static PyObject* vh_Ban(PyObject *self, PyObject *args)                { return vh_CallBool(W_Ban, args, "sssll"); }
static PyObject* vh_KickUser(PyObject *self, PyObject *args)           { return vh_CallBool(W_KickUser, args, "sss|sss"); }
static PyObject* vh_DelNickTempBan(PyObject *self, PyObject *args)     { return vh_CallBool(W_DelNickTempBan, args, "s"); }
static PyObject* vh_DelIPTempBan(PyObject *self, PyObject *args)       { return vh_CallBool(W_DelIPTempBan, args, "s"); }
static PyObject* vh_ParseCommand(PyObject *self, PyObject *args)       { return vh_CallBool(W_ParseCommand, args, "ssl"); }
static PyObject* vh_ScriptCommand(PyObject *self, PyObject *args)      { return vh_CallString(W_ScriptCommand, args, "sss"); }
static PyObject* vh_SetConfig(PyObject *self, PyObject *args)          { return vh_CallBool(W_SetConfig, args, "sss"); }
static PyObject* vh_GetConfig(PyObject *self, PyObject *args)          { return vh_CallString(W_GetConfig, args, "ss|s"); }
static PyObject* vh_IsRobotNickBad(PyObject *self, PyObject *args)     { return vh_CallLong(W_IsRobotNickBad, args, "s"); }
static PyObject* vh_AddRobot(PyObject *self, PyObject *args)           { return vh_CallBool(W_AddRobot, args, "slssss"); }
static PyObject* vh_DelRobot(PyObject *self, PyObject *args)           { return vh_CallBool(W_DelRobot, args, "s"); }
static PyObject* vh_SQL(PyObject *self, PyObject *args)                { return vh_CallString(W_SQL, args, "sl"); }
static PyObject* vh_GetServFreq(PyObject *self, PyObject *args)        { return vh_CallLong(W_GetServFreq, args, ""); }
static PyObject* vh_GetUsersCount(PyObject *self, PyObject *args)      { return vh_CallLong(W_GetUsersCount, args, ""); }
static PyObject* vh_GetTotalShareSize(PyObject *self, PyObject *args)  { return vh_CallString(W_GetTotalShareSize, args, ""); }
static PyObject* vh_usermc(PyObject *self, PyObject *args)             { return vh_CallBool(W_usermc, args, "ss|s"); }
static PyObject* vh_pm(PyObject *self, PyObject *args)                 { return vh_CallBool(W_pm, args, "ss|ss"); }
static PyObject* vh_mc(PyObject *self, PyObject *args)                 { return vh_CallBool(W_mc, args, "s"); }
static PyObject* vh_classmc(PyObject *self, PyObject *args)            { return vh_CallBool(W_classmc, args, "sll"); }
static PyObject* vh_Topic(PyObject *self, PyObject *args)              { return vh_CallString(W_Topic, args, "|s"); }
static PyObject* vh_name_and_version(PyObject *self, PyObject *args)   { return vh_CallString(W_name_and_version, args, ""); }
static PyObject* vh_StopHub(PyObject *self, PyObject *args)            { return vh_CallBool(W_StopHub, args, "l"); }

// GetNickList, GetOpList, GetBotList - return Python lists
static PyObject* vh_GetNickList(PyObject *self, PyObject *args)
{
	w_Targs *res = w_Python->callbacks[W_GetNickList](W_GetNickList, w_pack(""));
	if (!res) Py_RETURN_NONE;
	
	char *json_str = NULL;
	w_unpack(res, "D", &json_str);
	
	PyObject *py_list = NULL;
	
#ifdef HAVE_RAPIDJSON
	py_list = json_str ? JsonStringToPyObject(json_str) : PyList_New(0);
	if (!py_list) {
		// Clear any exception set by JsonStringToPyObject before returning fallback
		PyErr_Clear();
		py_list = PyList_New(0);
	}
#else
	// Fallback: parse JSON with Python json module
	py_list = PyList_New(0);
	if (json_str) {
		PyObject *json_module = PyImport_ImportModule("json");
		if (json_module) {
			PyObject *loads_func = PyObject_GetAttrString(json_module, "loads");
			if (loads_func) {
				PyObject *result = PyObject_CallFunction(loads_func, "s", json_str);
				if (result && PyList_Check(result)) {
					Py_DECREF(py_list);
					py_list = result;
				} else {
					Py_XDECREF(result);
				}
				Py_DECREF(loads_func);
			}
			Py_DECREF(json_module);
		}
	}
#endif
	
	// Free res AFTER using json_str (which points into res)
	w_free_args(res);
	return py_list;
}

static PyObject* vh_GetOpList(PyObject *self, PyObject *args)
{
	w_Targs *res = w_Python->callbacks[W_GetOpList](W_GetOpList, w_pack(""));
	if (!res) Py_RETURN_NONE;
	
	char *json_str = NULL;
	w_unpack(res, "D", &json_str);
	
#ifdef HAVE_RAPIDJSON
	PyObject *py_list = json_str ? JsonStringToPyObject(json_str) : PyList_New(0);
	if (!py_list) {
		// Clear any exception set by JsonStringToPyObject before returning fallback
		PyErr_Clear();
		py_list = PyList_New(0);
	}
#else
	// Fallback: parse JSON with Python json module
	PyObject *py_list = PyList_New(0);
	if (json_str) {
		PyObject *json_module = PyImport_ImportModule("json");
		if (json_module) {
			PyObject *loads_func = PyObject_GetAttrString(json_module, "loads");
			if (loads_func) {
				PyObject *result = PyObject_CallFunction(loads_func, "s", json_str);
				if (result && PyList_Check(result)) {
					Py_DECREF(py_list);
					py_list = result;
				} else {
					// Clear exception if parsing failed
					PyErr_Clear();
					Py_XDECREF(result);
				}
				Py_DECREF(loads_func);
			}
			Py_DECREF(json_module);
		}
	}
#endif
	
	w_free_args(res);
	return py_list;
}

static PyObject* vh_GetBotList(PyObject *self, PyObject *args)
{
	w_Targs *res = w_Python->callbacks[W_GetBotList](W_GetBotList, w_pack(""));
	if (!res) Py_RETURN_NONE;
	
	char *json_str = NULL;
	w_unpack(res, "D", &json_str);
	
#ifdef HAVE_RAPIDJSON
	PyObject *py_list = json_str ? JsonStringToPyObject(json_str) : PyList_New(0);
	if (!py_list) {
		// Clear any exception set by JsonStringToPyObject before returning fallback
		PyErr_Clear();
		py_list = PyList_New(0);
	}
#else
	// Fallback: parse JSON with Python json module
	PyObject *py_list = PyList_New(0);
	if (json_str) {
		PyObject *json_module = PyImport_ImportModule("json");
		if (json_module) {
			PyObject *loads_func = PyObject_GetAttrString(json_module, "loads");
			if (loads_func) {
				PyObject *result = PyObject_CallFunction(loads_func, "s", json_str);
				if (result && PyList_Check(result)) {
					Py_DECREF(py_list);
					py_list = result;
				} else {
					// Clear exception if parsing failed
					PyErr_Clear();
					Py_XDECREF(result);
				}
				Py_DECREF(loads_func);
			}
			Py_DECREF(json_module);
		}
	}
#endif
	
	w_free_args(res);
	return py_list;
}

// UserRestrictions - uses keyword arguments
static PyObject* vh_UserRestrictions(PyObject *self, PyObject *args, PyObject *kwargs)
{
	const char *nick;
	const char *nochat = "";
	const char *nopm = "";
	const char *nosearch = "";
	const char *noctm = "";
	
	static char *kwlist[] = {(char*)"nick", (char*)"nochat", (char*)"nopm", (char*)"nosearch", (char*)"noctm", NULL};
	
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ssss", kwlist,
	                                  &nick, &nochat, &nopm, &nosearch, &noctm))
		return NULL;
	
	// Convert all strings from UTF-8 to hub encoding
	std::string nick_hub = Utf8ToHub(nick);
	std::string nochat_hub = Utf8ToHub(nochat);
	std::string nopm_hub = Utf8ToHub(nopm);
	std::string nosearch_hub = Utf8ToHub(nosearch);
	std::string noctm_hub = Utf8ToHub(noctm);
	
	w_Targs *packed = w_pack("sssss", (char*)nick_hub.c_str(), (char*)nochat_hub.c_str(), 
	                          (char*)nopm_hub.c_str(), (char*)nosearch_hub.c_str(), (char*)noctm_hub.c_str());
	
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[W_UserRestrictions](W_UserRestrictions, packed);
	
	// NOTE: Use free(packed) here instead of w_free_args(packed) because the strings
	// in 'packed' are pointers to C++ std::string buffers (.c_str()) that are
	// stack-managed and will be automatically destroyed when they go out of scope.
	// w_free_args() should only be used for w_Targs containing heap-allocated strings
	// (e.g., from strdup() or callback return values).
	free(packed);
	PyEval_AcquireThread(state);
	
	if (!res) Py_RETURN_FALSE;
	
	long ret = 0;
	w_unpack(res, "l", &ret);
	w_free_args(res);
	
	if (ret) Py_RETURN_TRUE;
	Py_RETURN_FALSE;
}

// Encode - Pure Python utility function (no C++ callback needed)
// Converts special DC++ characters to their HTML entities
static PyObject* vh_Encode(PyObject *self, PyObject *args)
{
	const char *input;
	if (!PyArg_ParseTuple(args, "s", &input))
		return NULL;
	
	// Calculate output size (worst case: all chars need encoding)
	size_t len = strlen(input);
	size_t max_output = len * 6 + 1; // Worst case: &#126; = 6 chars
	char *output = (char*)malloc(max_output);
	if (!output) Py_RETURN_NONE;
	
	size_t j = 0;
	for (size_t i = 0; i < len; i++) {
		unsigned char c = input[i];
		switch (c) {
			case 5:   strcpy(&output[j], "&#5;");   j += 4; break;  // ^E
			case '$': strcpy(&output[j], "&#36;");  j += 5; break;
			case '|': strcpy(&output[j], "&#124;"); j += 6; break;
			case '`': strcpy(&output[j], "&#96;");  j += 5; break;
			case '~': strcpy(&output[j], "&#126;"); j += 6; break;
			default:  output[j++] = c; break;
		}
	}
	output[j] = '\0';
	
	PyObject *result = PyUnicode_FromString(output);
	free(output);
	return result;
}

// Decode - Pure Python utility function (no C++ callback needed)
// Converts HTML entities back to DC++ special characters
static PyObject* vh_Decode(PyObject *self, PyObject *args)
{
	const char *input;
	if (!PyArg_ParseTuple(args, "s", &input))
		return NULL;
	
	size_t len = strlen(input);
	char *output = (char*)malloc(len + 1); // Decoded is always <= original
	if (!output) Py_RETURN_NONE;
	
	size_t j = 0;
	for (size_t i = 0; i < len; i++) {
		if (input[i] == '&' && input[i+1] == '#') {
			// Parse &#NNN;
			int code = 0;
			size_t k = i + 2;
			while (k < len && input[k] >= '0' && input[k] <= '9') {
				code = code * 10 + (input[k] - '0');
				k++;
			}
			if (k < len && input[k] == ';') {
				output[j++] = (char)code;
				i = k; // Skip the entity
				continue;
			}
		}
		output[j++] = input[i];
	}
	output[j] = '\0';
	
	PyObject *result = PyUnicode_FromString(output);
	free(output);
	return result;
}

// Python 3 method table
static PyMethodDef vh_methods[] = {
	{"SendToOpChat",       vh_SendToOpChat,       METH_VARARGS, "Send message to operator chat"},
	{"SendToActive",       vh_SendToActive,       METH_VARARGS, "Send message to active users"},
	{"SendToPassive",      vh_SendToPassive,      METH_VARARGS, "Send message to passive users"},
	{"SendToActiveClass",  vh_SendToActiveClass,  METH_VARARGS, "Send message to active users of class range"},
	{"SendToPassiveClass", vh_SendToPassiveClass, METH_VARARGS, "Send message to passive users of class range"},
	{"SendDataToUser",     vh_SendDataToUser,     METH_VARARGS, "Send raw protocol data to user"},
	{"SendDataToAll",      vh_SendDataToAll,      METH_VARARGS, "Send raw protocol data to all users"},
	{"SendPMToAll",        vh_SendPMToAll,        METH_VARARGS, "Send PM to all users"},
	{"CloseConnection",    vh_CloseConnection,    METH_VARARGS, "Close user connection"},
	{"GetMyINFO",          vh_GetMyINFO,          METH_VARARGS, "Get user MyINFO tuple (nick, desc, tag, speed, email, sharesize)"},
	{"SetMyINFO",          vh_SetMyINFO,          METH_VARARGS, "Set user MyINFO"},
	{"GetUserClass",       vh_GetUserClass,       METH_VARARGS, "Get user class (0-10)"},
	{"GetNickList",        vh_GetNickList,        METH_VARARGS, "Get list of all nicknames"},
	{"GetOpList",          vh_GetOpList,          METH_VARARGS, "Get list of operator nicknames"},
	{"GetBotList",         vh_GetBotList,         METH_VARARGS, "Get list of bot nicknames"},
	{"GetRawNickList",     vh_GetRawNickList,     METH_VARARGS, "Get raw $NickList protocol string"},
	{"GetRawOpList",       vh_GetRawOpList,       METH_VARARGS, "Get raw $OpList protocol string"},
	{"GetRawBotList",      vh_GetRawBotList,      METH_VARARGS, "Get raw bot list protocol string"},
	{"GetUserHost",        vh_GetUserHost,        METH_VARARGS, "Get user hostname"},
	{"GetUserIP",          vh_GetUserIP,          METH_VARARGS, "Get user IP address"},
	{"SetUserIP",          vh_SetUserIP,          METH_VARARGS, "Set user IP address"},
	{"SetMyINFOFlag",      vh_SetMyINFOFlag,      METH_VARARGS, "Set MyINFO flag"},
	{"UnsetMyINFOFlag",    vh_UnsetMyINFOFlag,    METH_VARARGS, "Unset MyINFO flag"},
	{"GetUserHubURL",      vh_GetUserHubURL,      METH_VARARGS, "Get user hub URL"},
	{"GetUserExtJSON",     vh_GetUserExtJSON,     METH_VARARGS, "Get user ExtJSON"},
	{"GetUserCC",          vh_GetUserCC,          METH_VARARGS, "Get user country code"},
	{"GetIPCC",            vh_GetIPCC,            METH_VARARGS, "Get country code for IP"},
	{"GetIPCN",            vh_GetIPCN,            METH_VARARGS, "Get country name for IP"},
	{"GetIPASN",           vh_GetIPASN,           METH_VARARGS, "Get ASN for IP"},
	{"GetGeoIP",           vh_GetGeoIP,           METH_VARARGS, "Get GeoIP info"},
	{"AddRegUser",         vh_AddRegUser,         METH_VARARGS, "Register new user"},

	{"DelRegUser",         vh_DelRegUser,         METH_VARARGS, "Delete registered user"},
	{"SetRegClass",        vh_SetRegClass,        METH_VARARGS, "Set registered user class"},
	{"Ban",                vh_Ban,                METH_VARARGS, "Ban user"},
	{"KickUser",           vh_KickUser,           METH_VARARGS, "Kick user"},
	{"DelNickTempBan",     vh_DelNickTempBan,     METH_VARARGS, "Delete temporary nick ban"},
	{"DelIPTempBan",       vh_DelIPTempBan,       METH_VARARGS, "Delete temporary IP ban"},
	{"ParseCommand",       vh_ParseCommand,       METH_VARARGS, "Parse hub command"},
	{"ScriptCommand",      vh_ScriptCommand,      METH_VARARGS, "Execute script command"},
	{"SetConfig",          vh_SetConfig,          METH_VARARGS, "Set configuration value"},
	{"GetConfig",          vh_GetConfig,          METH_VARARGS, "Get configuration value"},
	{"IsRobotNickBad",     vh_IsRobotNickBad,     METH_VARARGS, "Check if robot nick is valid"},
	{"AddRobot",           vh_AddRobot,           METH_VARARGS, "Add bot to hub"},
	{"DelRobot",           vh_DelRobot,           METH_VARARGS, "Remove bot from hub"},
	{"SQL",                vh_SQL,                METH_VARARGS, "Execute SQL query"},
	{"GetServFreq",        vh_GetServFreq,        METH_VARARGS, "Get server frequency"},
	{"GetUsersCount",      vh_GetUsersCount,      METH_VARARGS, "Get online users count"},
	{"GetTotalShareSize",  vh_GetTotalShareSize,  METH_VARARGS, "Get total share size"},
	{"usermc",             vh_usermc,             METH_VARARGS, "Send main chat message to user"},
	{"pm",                 vh_pm,                 METH_VARARGS, "Send private message"},
	{"mc",                 vh_mc,                 METH_VARARGS, "Send main chat message"},
	{"classmc",            vh_classmc,            METH_VARARGS, "Send main chat message to class range"},
	{"UserRestrictions",   (PyCFunction)vh_UserRestrictions, METH_VARARGS | METH_KEYWORDS, "Set user restrictions"},
	{"Topic",              vh_Topic,              METH_VARARGS, "Get/set hub topic"},
	{"name_and_version",   vh_name_and_version,   METH_VARARGS, "Get Verlihub name and version"},
	{"StopHub",            vh_StopHub,            METH_VARARGS, "Stop the hub"},
	{"Encode",             vh_Encode,             METH_VARARGS, "Encode DC++ special characters to HTML entities"},
	{"Decode",             vh_Decode,             METH_VARARGS, "Decode HTML entities back to DC++ special characters"},
	{"CallDynamicFunction", vh_CallDynamicFunction, METH_VARARGS, "Call a dynamically registered C++ function"},
	{NULL, NULL, 0, NULL}
};

// Python 3 module definition
static struct PyModuleDef vh_module = {
	PyModuleDef_HEAD_INIT,
	"vh",
	"Verlihub C++ callback interface - Python 3",
	-1,
	vh_methods
};

// Module initializer (called per sub-interpreter)
static PyObject* vh_CreateModule(long script_id)
{
	PyObject *m = PyModule_Create(&vh_module);
	if (!m) return NULL;
	
	w_TScript *script = w_Scripts[script_id];
	
	// Add script-specific attributes
	PyModule_AddIntConstant(m, "myid", script_id);
	PyModule_AddStringConstant(m, "botname", script->botname ? script->botname : "");
	PyModule_AddStringConstant(m, "opchatname", script->opchatname ? script->opchatname : "");
	PyModule_AddStringConstant(m, "name", script->name ? script->name : "");
	PyModule_AddStringConstant(m, "path", script->path ? script->path : "");
	PyModule_AddStringConstant(m, "config_name", script->config_name ? script->config_name : "");
	PyModule_AddStringConstant(m, "basedir", script->config_dir ? script->config_dir : "");
	
	// Add connection close reason constants (from cserverdc.h)
	PyModule_AddIntConstant(m, "eCR_DEFAULT", 0);
	PyModule_AddIntConstant(m, "eCR_INVALID_USER", 1);
	PyModule_AddIntConstant(m, "eCR_KICKED", 2);
	PyModule_AddIntConstant(m, "eCR_FORCEMOVE", 3);
	PyModule_AddIntConstant(m, "eCR_QUIT", 4);
	PyModule_AddIntConstant(m, "eCR_HUB_LOAD", 5);
	PyModule_AddIntConstant(m, "eCR_TIMEOUT", 6);
	PyModule_AddIntConstant(m, "eCR_TO_ANYACTION", 7);
	PyModule_AddIntConstant(m, "eCR_USERLIMIT", 8);
	PyModule_AddIntConstant(m, "eCR_SHARE_LIMIT", 9);
	PyModule_AddIntConstant(m, "eCR_TAG_NONE", 10);
	PyModule_AddIntConstant(m, "eCR_TAG_INVALID", 11);
	PyModule_AddIntConstant(m, "eCR_PASSWORD", 12);
	PyModule_AddIntConstant(m, "eCR_LOGIN_ERR", 13);
	PyModule_AddIntConstant(m, "eCR_SYNTAX", 14);
	PyModule_AddIntConstant(m, "eCR_INVALID_KEY", 15);
	PyModule_AddIntConstant(m, "eCR_RECONNECT", 16);
	PyModule_AddIntConstant(m, "eCR_CLONE", 17);
	PyModule_AddIntConstant(m, "eCR_SELF", 18);
	PyModule_AddIntConstant(m, "eCR_BADNICK", 19);
	PyModule_AddIntConstant(m, "eCR_NOREDIR", 20);
	
	// Bot validation constants
	PyModule_AddIntConstant(m, "eBOT_OK", 0);
	PyModule_AddIntConstant(m, "eBOT_EXISTS", 1);
	PyModule_AddIntConstant(m, "eBOT_WITHOUT_NICK", 2);
	PyModule_AddIntConstant(m, "eBOT_BAD_CHARS", 3);
	PyModule_AddIntConstant(m, "eBOT_RESERVED_NICK", 4);
	PyModule_AddIntConstant(m, "eBOT_API_ERROR", 5);
	
	// Ban type constants (from cban.h)
	PyModule_AddIntConstant(m, "eBF_NICKIP", 0);
	PyModule_AddIntConstant(m, "eBF_IP", 1);
	PyModule_AddIntConstant(m, "eBF_NICK", 2);
	PyModule_AddIntConstant(m, "eBF_RANGE", 3);
	PyModule_AddIntConstant(m, "eBF_HOST1", 4);
	PyModule_AddIntConstant(m, "eBF_HOST2", 5);
	PyModule_AddIntConstant(m, "eBF_HOST3", 6);
	PyModule_AddIntConstant(m, "eBF_SHARE", 7);
	PyModule_AddIntConstant(m, "eBF_PREFIX", 8);
	PyModule_AddIntConstant(m, "eBF_HOSTR1", 9);
	
	return m;
}

int w_Begin(w_Tcallback *callbacks)
{
	Py_Initialize();
	w_Python = (w_TScript*)calloc(1, sizeof(w_TScript));
	if (!w_Python) return 0;
	w_Python->callbacks = callbacks;
	PyEval_ReleaseThread(PyGILState_GetThisThreadState());
	return 1;
}

int w_End()
{
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: w_End() called - starting Python cleanup\n");
	#endif
	
	// Clean up all remaining interpreters before finalizing Python
	// We need to track the main state before we start ending interpreters
	PyGILState_STATE gil = PyGILState_Ensure();
	PyThreadState *main_state = PyThreadState_Get();
	
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: Acquired GIL, cleaning up %zu script(s)\n", w_Scripts.size());
	#endif
	
	bool any_had_threads = false;
	
	// First pass: check which scripts had threading (check persistent tracking vector)
	for (size_t i = 0; i < w_Scripts_had_threads.size(); ++i) {
		if (w_Scripts_had_threads[i]) {
			any_had_threads = true;
			#ifdef DEBUG_WRAPPER
			fprintf(stderr, "PY: Script %d had threading/asyncio\n", (int)i);
			#endif
		}
	}
	
#ifdef PYTHON_SINGLE_INTERPRETER
	// Single interpreter mode: all scripts shared the same interpreter
	// Just clean up the script objects, not the interpreter itself
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: Single interpreter mode - cleaning up shared interpreter\n");
	#endif
	
	// Remove vh module from sys.modules so next w_Begin() gets fresh callbacks
	PyObject *modules = PyImport_GetModuleDict();
	if (modules && PyDict_Contains(modules, PyUnicode_FromString("vh"))) {
		PyDict_DelItemString(modules, "vh");
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: Removed vh module from sys.modules (will be recreated with fresh callbacks)\n");
		#endif
	}
	
	for (size_t i = 0; i < w_Scripts.size(); ++i) {
		w_TScript *script = w_Scripts[i];
		if (script) {
			// Clean up Python objects
			Py_XDECREF(script->module);
			script->module = NULL;
			
			// Clean up dynamic function registry
			if (script->dynamic_funcs) {
				delete script->dynamic_funcs;
				script->dynamic_funcs = NULL;
			}
			
			free(script->path);
			free(script->name);
			free(script->botname);
			free(script->opchatname);
			free(script->hooks);
			free(script->config_name);
			free(script);
			w_Scripts[i] = NULL;
		}
	}
	
	// In single interpreter mode, we can safely call Py_Finalize even with threading
	// because there are no sub-interpreters to corrupt
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: Calling Py_Finalize() (safe in single interpreter mode)\n");
	#endif
	// Note: Do NOT release GIL before Py_Finalize - Python docs say finalize handles it
	Py_Finalize();
	// GIL is automatically released by Py_Finalize
	
#else
	// Sub-interpreter mode: each script had its own isolated interpreter
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: Sub-interpreter mode - cleaning up %zu sub-interpreters\n", w_Scripts.size());
	#endif
	
	for (size_t i = 0; i < w_Scripts.size(); ++i) {
		w_TScript *script = w_Scripts[i];
		if (script && script->state) {
			// Switch to the subinterpreter to clean up
			PyThreadState_Swap(script->state);
			
			// Clean up Python objects in the subinterpreter
			Py_XDECREF(script->module);
			script->module = NULL;
			
			// Always switch back to main interpreter before cleanup
			PyThreadState_Swap(main_state);
			
			if (script->had_threads) {
				// WORKAROUND: Do NOT call Py_EndInterpreter for scripts that used threading
				// This is a known limitation of Python's sub-interpreter + threading model
				// The interpreter will leak, but it's better than crashing
				// Reference: https://bugs.python.org/issue15751
				#ifdef DEBUG_WRAPPER
				fprintf(stderr, "PY: Skipping Py_EndInterpreter() for sub-interpreter %d to prevent crash\n",
				        (int)i);
				#endif
				// script->state is intentionally not freed - memory leak but no crash
				script->state = NULL;  // Clear the pointer but don't end the interpreter
			} else {
				// Safe to end interpreters that didn't use threading
				#ifdef DEBUG_WRAPPER
				fprintf(stderr, "PY: Calling Py_EndInterpreter() for sub-interpreter %d\n", (int)i);
				#endif
				Py_EndInterpreter(script->state);
				script->state = NULL;
			}
		}
		
		// Clean up C++ resources for this script
		if (script) {
			// Clean up dynamic function registry
			if (script->dynamic_funcs) {
				delete script->dynamic_funcs;
				script->dynamic_funcs = NULL;
			}
			
			free(script->path);
			free(script->name);
			free(script->botname);
			free(script->opchatname);
			free(script->hooks);
			free(script->config_name);
			free(script);
			w_Scripts[i] = NULL;
		}
	}
	
	// Now all subinterpreters are cleaned up
	// CRITICAL: Do NOT call Py_Finalize() if any interpreter used threading
	// This is a known Python limitation - Py_Finalize() with threading causes crashes
	// Reference: https://bugs.python.org/issue15751
	if (any_had_threads) {
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: Skipping Py_Finalize() because threading was used\n");
		fprintf(stderr, "PY: This prevents crashes but will leak Python memory (known limitation)\n");
		fprintf(stderr, "PY: Also skipping PyGILState_Release() to avoid thread state corruption\n");
		#endif
		// Do NOT release GIL or finalize - just leave Python in current state
		// This leaks memory but prevents crashes
	} else {
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: About to call Py_Finalize()...\n");
		#endif
		Py_Finalize();
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: Py_Finalize() completed successfully\n");
		#endif
		// No PyGILState_Release after Py_Finalize
	}
#endif
	
	// Clear tracking vectors
	w_Scripts_had_threads.clear();
	
	free(w_Python);
	return 1;
}

int w_ReserveID()
{
	w_TScript *script = (w_TScript*)calloc(1, sizeof(w_TScript));
	if (!script) return -1;
	script->id = w_Scripts.size();
	script->dynamic_funcs = NULL;  // Initialize dynamic function registry
	script->had_threads = false;   // Initialize threading flag
	w_Scripts.push_back(script);
	w_Scripts_had_threads.push_back(false);  // Initialize tracking vector
	return script->id;
}

int w_Load(w_Targs *args)
{
	int id;
	long starttime;
	char *path, *botname, *opchatname, *config_dir, *config_name;

	if (!w_unpack(args, "lssssls", &id, &path, &botname, &opchatname, &config_dir, &starttime, &config_name)) return -1;  // Fixed unpack to match 7 args from pack call; original had 6, causing data mismatch

	w_TScript *script = w_Scripts[id];
	if (!script) return -1;

	script->path = strdup(path);
	script->botname = strdup(botname);
	script->opchatname = strdup(opchatname);
	script->config_name = strdup(config_name);
	script->config_dir = strdup(config_dir);
	script->use_old_ontimer = false;

	// Extract script_dir and module_name
	char *path_dup = strdup(path);
	char *script_dir_c = dirname(path_dup);
	string script_dir = string(script_dir_c);
	free(path_dup);

	path_dup = strdup(path);
	char *base_c = basename(path_dup);
	string module_name = string(base_c);
	free(path_dup);

	size_t dot = module_name.rfind('.');
	if (dot != string::npos && module_name.substr(dot) == ".py") {
		module_name = module_name.substr(0, dot);
	}
	script->name = strdup(module_name.c_str());

	PyGILState_STATE gil = PyGILState_Ensure();
	PyThreadState *main_state = PyThreadState_Get();
#ifdef PYTHON_SINGLE_INTERPRETER
	// Single interpreter mode: all scripts share the same interpreter
	// No need to create a new interpreter or swap states
	script->state = main_state;
	log("PY: [%d:%s] Using SINGLE interpreter mode (threading-safe, data shared)\n", id, script->name);
#else
	// Sub-interpreter mode: create isolated interpreter and swap back to main
	script->state = Py_NewInterpreter();
	PyThreadState_Swap(main_state);
	log("PY: [%d:%s] Created SUB-interpreter (isolated, threading limited)\n", id, script->name);
#endif
	PyGILState_Release(gil);
	if (!script->state) return -1;

	// Acquire GIL and swap to script's interpreter (same as main in single mode)
	PyGILState_STATE sub_gil = PyGILState_Ensure();
	main_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);

	// Register vh module for this interpreter
	PyObject *vh_mod = vh_CreateModule(id);
	if (vh_mod) {
		PyObject *modules = PyImport_GetModuleDict();
		PyDict_SetItemString(modules, "vh", vh_mod);
		Py_DECREF(vh_mod);
	} else {
		log("PY: [%d:%s] Failed to create vh module\n", id, script->name);
		if (PyErr_Occurred()) PyErr_Print();
		PyThreadState_Swap(main_state);
		PyGILState_Release(sub_gil);
		return -1;
	}

	// Add script_dir and config_dir/scripts to sys.path
	// Important: Use PySys_GetObject to APPEND to path, not REPLACE it
	// This preserves access to Python standard library and site-packages
	PyObject *sys_path = PySys_GetObject("path"); // Borrowed reference, don't DECREF
	if (sys_path && PyList_Check(sys_path)) {
		// Prepend script directory (so it takes precedence)
		PyObject *script_dir_str = PyUnicode_FromString(script_dir.c_str());
		if (script_dir_str) {
			PyList_Insert(sys_path, 0, script_dir_str);
			Py_DECREF(script_dir_str);
		}
		
		// Prepend config_dir/scripts directory
		string scripts_path = string(config_dir) + "/scripts";
		PyObject *scripts_path_str = PyUnicode_FromString(scripts_path.c_str());
		if (scripts_path_str) {
			PyList_Insert(sys_path, 0, scripts_path_str);
			Py_DECREF(scripts_path_str);
		}
	} else {
		log("PY: Warning - could not access sys.path\n");
	}

#ifdef PYTHON_SINGLE_INTERPRETER
	// Single interpreter mode: Execute script in __main__ namespace so scripts share globals
	PyObject *main_module = PyImport_AddModule("__main__");
	if (!main_module) {
		log("PY: [%d:%s] Failed to get __main__ module\n", id, script->name);
		if (PyErr_Occurred()) PyErr_Print();
		PyThreadState_Swap(main_state);
		PyGILState_Release(sub_gil);
		return -1;
	}
	Py_INCREF(main_module);
	script->module = main_module;
	
	// Execute the script file in __main__'s namespace
	PyObject *main_dict = PyModule_GetDict(main_module);  // Borrowed ref
	
	// Set __file__ so scripts can use it (e.g., os.path.dirname(__file__))
	PyObject *file_str = PyUnicode_FromString(script->path);
	if (file_str) {
		PyDict_SetItemString(main_dict, "__file__", file_str);
		Py_DECREF(file_str);
	}
	
	FILE *script_file = fopen(script->path, "r");
	if (!script_file) {
		log("PY: [%d:%s] Failed to open script file for execution: %s\n", id, script->name, script->path);
		Py_DECREF(main_module);
		PyThreadState_Swap(main_state);
		PyGILState_Release(sub_gil);
		return -1;
	}
	
	PyObject *result = PyRun_File(script_file, script->path, Py_file_input, main_dict, main_dict);
	fclose(script_file);
	
	if (!result) {
		log("PY: [%d:%s] Failed to execute script in __main__ namespace\n", id, script->name);
		if (PyErr_Occurred()) PyErr_Print();
		Py_DECREF(main_module);
		PyThreadState_Swap(main_state);
		PyGILState_Release(sub_gil);
		return -1;
	}
	Py_DECREF(result);
#else
	// Sub-interpreter mode: Import as separate module (isolated namespace)
	script->module = PyImport_ImportModule(script->name);
	if (!script->module) {
		if (PyErr_Occurred()) PyErr_Print();
		PyThreadState_Swap(main_state);
		PyGILState_Release(sub_gil);
		return -1;
	}
#endif

	script->hooks = (char*)calloc(W_MAX_HOOKS, sizeof(char));

	for (int i = 0; i < W_MAX_HOOKS; i++) {
		const char *hname = w_HookName(i);
		if (hname) {
			PyObject *func = PyObject_GetAttrString(script->module, hname);
			if (func) {
				if (PyCallable_Check(func)) {
					script->hooks[i] = 1;
				}
			} else {
				PyErr_Clear();  // Clear error if attribute not found
			}
			Py_XDECREF(func);
		}
	}

	PyThreadState_Swap(main_state);
	PyGILState_Release(sub_gil);

	return id;
}

int w_Unload(int id)
{
	w_TScript *script = w_Scripts[id];
	if (!script) return 0;

	PyGILState_STATE gil = PyGILState_Ensure();
	PyThreadState *main_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);
	
	// Check if this interpreter used threading/asyncio
	bool had_threads = false;
	PyObject *sys_modules = PySys_GetObject("modules");  // Borrowed ref
	if (sys_modules && PyDict_Check(sys_modules)) {
		PyObject *key = PyUnicode_FromString("threading");
		if (key) {
			had_threads = (PyDict_Contains(sys_modules, key) == 1);
			Py_DECREF(key);
		}
		if (!had_threads) {
			key = PyUnicode_FromString("asyncio");
			if (key) {
				had_threads = (PyDict_Contains(sys_modules, key) == 1);
				Py_DECREF(key);
			}
		}
	}

	Py_XDECREF(script->module);
	
	// Store the threading flag in both places
	script->had_threads = had_threads;
	w_Scripts_had_threads[id] = had_threads;  // Persist even after script is freed
	
#ifdef PYTHON_SINGLE_INTERPRETER
	// Single interpreter mode: never end the interpreter, just clean the module
	// All scripts share the same interpreter, so we can't end it per-script
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: Single interpreter mode - skipping Py_EndInterpreter()\n");
	#endif
	PyThreadState_Swap(main_state);
#else
	// Sub-interpreter mode: end interpreter only if no threading was used
	// WORKAROUND: Do NOT call Py_EndInterpreter if threading was used
	// Reference: https://bugs.python.org/issue15751
	if (had_threads) {
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: Skipping Py_EndInterpreter() for script %d (threading detected)\n", id);
		#endif
		// Switch back but don't end interpreter
		PyThreadState_Swap(main_state);
	} else {
		Py_EndInterpreter(script->state);
		PyThreadState_Swap(main_state);
	}
#endif
	
	PyGILState_Release(gil);

	// Clean up dynamic function registry
	if (script->dynamic_funcs) {
		delete script->dynamic_funcs;
		script->dynamic_funcs = NULL;
	}

	free(script->path);
	free(script->name);
	free(script->botname);
	free(script->opchatname);
	free(script->hooks);
	free(script->config_name);
	free(script->config_dir);
	free(script);

	w_Scripts[id] = NULL;

	return 1;
}

int w_HasHook(int id, int hook)
{
	w_TScript *script = w_Scripts[id];
	if (!script) return 0;
	if (hook < 0 || hook >= W_MAX_HOOKS) return 0;
	return script->hooks[hook];
}

w_Targs *w_CallHook(int id, int num, w_Targs *params)
{
	w_TScript *script = w_Scripts[id];
	if (!script) return NULL;
	
	// Acquire GIL and switch to script's interpreter
	PyGILState_STATE gstate = PyGILState_Ensure();
	
	// CRITICAL: Re-check script validity after acquiring GIL
	// Script may have been unloaded by another thread between our first check and now
	script = w_Scripts[id];
	if (!script || !script->state) {
		PyGILState_Release(gstate);
		return NULL;
	}
	
#ifndef PYTHON_SINGLE_INTERPRETER
	// Sub-interpreter mode: need to swap to script's interpreter state
	// In single-interpreter mode, all scripts share the same state, so no swap needed
	PyThreadState *old_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);
#endif

	const char *name = w_HookName(num);
	if (!name) {
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	// CRITICAL: Increment module reference to protect from concurrent Py_XDECREF in w_Unload
	PyObject *module = script->module;
	Py_XINCREF(module);
	if (!module) {
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	PyObject *func = PyObject_GetAttrString(module, name);
	if (!func || !PyCallable_Check(func)) {
		Py_XDECREF(func);
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	size_t arg_count = params->format ? strlen(params->format) : 0;
	PyObject *args = PyTuple_New(arg_count);
	if (!args) {
		Py_DECREF(func);
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	// Initialize all tuple slots to None to avoid double-free on error paths
	for (size_t i = 0; i < arg_count; i++) {
		Py_INCREF(Py_None);
		PyTuple_SetItem(args, i, Py_None);
	}

	for (size_t i = 0; i < arg_count; i++) {
		switch (params->format[i]) {
			case 'l': {
				PyObject *long_obj = PyLong_FromLong(params->args[i].l);
				if (!long_obj) {
					#ifdef DEBUG_WRAPPER
					fprintf(stderr, "PY: w_CallHook - PyLong_FromLong failed\n");
					#endif
					PyErr_Clear();
					Py_DECREF(args);
					Py_DECREF(func);
					Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
					PyThreadState_Swap(old_state);
#endif
					PyGILState_Release(gstate);
					return NULL;
				}
				PyTuple_SetItem(args, i, long_obj);
				break;
			}
			case 's': {
				const char *s = params->args[i].s;
				if (s) {
					PyObject *str_obj = PyUnicode_FromString(s);
					if (!str_obj) {
						// Invalid UTF-8 or allocation failure
						#ifdef DEBUG_WRAPPER
						fprintf(stderr, "PY: w_CallHook - PyUnicode_FromString failed for hook %d\n", num);
						#endif
						PyErr_Clear();
						Py_DECREF(args);
						Py_DECREF(func);
						Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
						PyThreadState_Swap(old_state);
#endif
						PyGILState_Release(gstate);
						return NULL;
					}
					PyTuple_SetItem(args, i, str_obj);
				} else {
					Py_INCREF(Py_None);
					PyTuple_SetItem(args, i, Py_None);
				}
				break;
			}
			case 'd': {
				PyObject *float_obj = PyFloat_FromDouble(params->args[i].d);
				if (!float_obj) {
					#ifdef DEBUG_WRAPPER
					fprintf(stderr, "PY: w_CallHook - PyFloat_FromDouble failed\n");
					#endif
					PyErr_Clear();
					Py_DECREF(args);
					Py_DECREF(func);
					Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
					PyThreadState_Swap(old_state);
#endif
					PyGILState_Release(gstate);
					return NULL;
				}
				PyTuple_SetItem(args, i, float_obj);
				break;
			}
			case 'p': {
				PyObject *ptr_obj = PyLong_FromVoidPtr(params->args[i].p);
				if (!ptr_obj) {
					#ifdef DEBUG_WRAPPER
					fprintf(stderr, "PY: w_CallHook - PyLong_FromVoidPtr failed\n");
					#endif
					PyErr_Clear();
					Py_DECREF(args);
					Py_DECREF(func);
					Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
					PyThreadState_Swap(old_state);
#endif
					PyGILState_Release(gstate);
					return NULL;
				}
				PyTuple_SetItem(args, i, ptr_obj);
				break;
			}
			default:
				Py_DECREF(args);
				Py_DECREF(func);
				Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
				PyThreadState_Swap(old_state);
#endif
				PyGILState_Release(gstate);
				return NULL;
		}
	}

	// Validate that args tuple is fully populated
	for (size_t i = 0; i < arg_count; i++) {
		PyObject *item = PyTuple_GetItem(args, i);
		if (!item) {
			#ifdef DEBUG_WRAPPER
			fprintf(stderr, "PY: w_CallHook - NULL item in args tuple at index %zu\n", i);
			#endif

			Py_DECREF(args);
			Py_DECREF(func);
			Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
			PyThreadState_Swap(old_state);
#endif
			PyGILState_Release(gstate);
			return NULL;
		}
	}

	// Validate func and args before calling
	if (!PyCallable_Check(func)) {
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: w_CallHook - func is not callable\n");
		#endif
		Py_DECREF(args);
		Py_DECREF(func);
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	if (!PyTuple_Check(args)) {
		#ifdef DEBUG_WRAPPER
		fprintf(stderr, "PY: w_CallHook - args is not a tuple\n");
		#endif
		Py_DECREF(args);
		Py_DECREF(func);
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: w_CallHook - About to call hook %d with %zu args\n", num, arg_count);
	#endif
	PyObject *res = PyObject_CallObject(func, args);
	#ifdef DEBUG_WRAPPER
	fprintf(stderr, "PY: w_CallHook - Returned from PyObject_CallObject, res=%p\n", (void*)res);
	#endif

	Py_DECREF(args);
	Py_DECREF(func);

	if (!res) {
		if (PyErr_Occurred()) PyErr_Print();
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	w_Targs *ret = NULL;

	if (PyLong_Check(res)) {
		long l = PyLong_AsLong(res);
		ret = w_pack("l", l);
	} else if (PyFloat_Check(res)) {
		double d = PyFloat_AsDouble(res);
		ret = w_pack("d", d);
	} else if (PyUnicode_Check(res)) {
		const char *s = PyUnicode_AsUTF8(res);
		if (!s) s = "";
		ret = w_pack("s", strdup(s));
	} else if (res == Py_None) {
		ret = w_pack("l", (long)0);  // Default to 0 for None
	} else if (PyTuple_Check(res)) {
		int len = PyTuple_Size(res);
		if (len > W_MAX_RETVALS) len = W_MAX_RETVALS;
		ret = (w_Targs*)calloc(1, sizeof(w_Targs) + len * sizeof(w_Telement));
		if (!ret) {
			Py_DECREF(res);
			Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
			PyThreadState_Swap(old_state);
#endif
			PyGILState_Release(gstate);
			return NULL;
		}
		ret->format = "tab";
		for (int i = 0; i < len; i++) {
			PyObject *item = PyTuple_GetItem(res, i);
			if (PyLong_Check(item)) {
				ret->args[i].type = 'l';
				ret->args[i].l = PyLong_AsLong(item);
			} else if (PyFloat_Check(item)) {
				ret->args[i].type = 'd';
				ret->args[i].d = PyFloat_AsDouble(item);
			} else if (PyUnicode_Check(item)) {
				const char *s = PyUnicode_AsUTF8(item);
				if (!s) s = "";
				ret->args[i].type = 's';
				ret->args[i].s = strdup(s);
			} else {
				ret->args[i].type = 0;
			}
		}
	}

	Py_DECREF(res);
	Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
	PyThreadState_Swap(old_state);
#endif
	PyGILState_Release(gstate);
	return ret;
}

w_Targs *w_CallFunction(int id, const char *func_name, w_Targs *params)
{
	w_TScript *script = w_Scripts[id];
	if (!script) {
		log("PY: w_CallFunction - invalid script id %d\n", id);
		return NULL;
	}
	
	if (!func_name || func_name[0] == '\0') {
		log("PY: w_CallFunction - function name is NULL or empty\n");
		return NULL;
	}

	PyGILState_STATE gstate = PyGILState_Ensure();
	
	// CRITICAL: Re-check script validity after acquiring GIL
	// Script may have been unloaded by another thread between our first check and now
	script = w_Scripts[id];
	if (!script || !script->state) {
		log("PY: w_CallFunction - script %d was unloaded\n", id);
		PyGILState_Release(gstate);
		return NULL;
	}
	
	PyThreadState *old_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);

	// CRITICAL: Increment module reference to protect from concurrent Py_XDECREF in w_Unload
	PyObject *module = script->module;
	Py_XINCREF(module);
	if (!module) {
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	PyObject *func = PyObject_GetAttrString(module, func_name);
	if (!func) {
		log("PY: w_CallFunction - function '%s' not found in script\n", func_name);
		PyErr_Clear();
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}
	
	if (!PyCallable_Check(func)) {
		log("PY: w_CallFunction - '%s' is not callable\n", func_name);
		Py_DECREF(func);
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	size_t arg_count = (params && params->format) ? strlen(params->format) : 0;
	PyObject *args = PyTuple_New(arg_count);
	if (!args) {
		log("PY: w_CallFunction - failed to create argument tuple\n");
		Py_DECREF(func);
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	// Initialize all tuple slots to None to avoid double-free on error paths
	for (size_t i = 0; i < arg_count; i++) {
		Py_INCREF(Py_None);
		PyTuple_SetItem(args, i, Py_None);
	}

	for (size_t i = 0; i < arg_count; i++) {
		switch (params->format[i]) {
			case 'l':
				PyTuple_SetItem(args, i, PyLong_FromLong(params->args[i].l));
				break;
			case 's': {
				const char *s = params->args[i].s;
				if (s) {
					PyTuple_SetItem(args, i, PyUnicode_FromString(s));
				} else {
					Py_INCREF(Py_None);
					PyTuple_SetItem(args, i, Py_None);
				}
				break;
			}
			case 'd':
				PyTuple_SetItem(args, i, PyFloat_FromDouble(params->args[i].d));
				break;
			case 'p':
				PyTuple_SetItem(args, i, PyLong_FromVoidPtr(params->args[i].p));
				break;
			case 'L': {  //  List of strings
				char **list = params->args[i].L;
				if (!list) {
					PyTuple_SetItem(args, i, PyList_New(0));
					break;
				}
				// Count elements
				int count = 0;
				while (list[count] != NULL) count++;
				
				PyObject *py_list = PyList_New(count);
				for (int j = 0; j < count; j++) {
					PyList_SetItem(py_list, j, PyUnicode_FromString(list[j]));
				}
				PyTuple_SetItem(args, i, py_list);
				break;
			}
			case 'D': {  //  Dict/complex as JSON string
				const char *json_str = params->args[i].s;
				if (!json_str || json_str[0] == '\0') {
					PyTuple_SetItem(args, i, PyDict_New());
					break;
				}
#ifdef HAVE_RAPIDJSON
				// Use RapidJSON-based converter for fast parsing
				PyObject *py_obj = JsonStringToPyObject(json_str);
				if (py_obj) {
					PyTuple_SetItem(args, i, py_obj);
				} else {
					log("PY: w_CallFunction - failed to parse JSON: %s\n", json_str);
					PyErr_Clear();
					PyTuple_SetItem(args, i, PyDict_New());
				}
#else
				// Fallback to Python json module
				PyObject *json_module = PyImport_ImportModule("json");
				if (!json_module) {
					log("PY: w_CallFunction - failed to import json module\n");
					PyErr_Clear();
					PyTuple_SetItem(args, i, PyDict_New());
					break;
				}
				PyObject *loads_func = PyObject_GetAttrString(json_module, "loads");
				if (!loads_func) {
					log("PY: w_CallFunction - failed to get json.loads\n");
					PyErr_Clear();
					Py_DECREF(json_module);
					PyTuple_SetItem(args, i, PyDict_New());
					break;
				}
				PyObject *json_arg = PyUnicode_FromString(json_str);
				PyObject *py_dict = PyObject_CallFunction(loads_func, "O", json_arg);
				Py_DECREF(json_arg);
				Py_DECREF(loads_func);
				Py_DECREF(json_module);
				if (!py_dict || PyErr_Occurred()) {
					log("PY: w_CallFunction - failed to parse JSON: %s\n", json_str);
					PyErr_Clear();
					PyTuple_SetItem(args, i, PyDict_New());
				} else {
					PyTuple_SetItem(args, i, py_dict);
				}
#endif
				break;
			}
			case 'O':  //  PyObject* passthrough
				if (params->args[i].O) {
					Py_INCREF(params->args[i].O);  // Inc ref for tuple
					PyTuple_SetItem(args, i, params->args[i].O);
				} else {
					Py_INCREF(Py_None);
					PyTuple_SetItem(args, i, Py_None);
				}
				break;
		default:
			log("PY: w_CallFunction - unknown format character '%c'\n", params->format[i]);
			Py_DECREF(args);
			Py_DECREF(func);
			Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
			PyThreadState_Swap(old_state);
#endif
			PyGILState_Release(gstate);
			return NULL;
		}
	}	log2("PY: Calling function '%s' with %zu arguments\n", func_name, arg_count);
	PyObject *res = PyObject_CallObject(func, args);

	Py_DECREF(args);
	Py_DECREF(func);

	if (!res) {
		log("PY: w_CallFunction - call to '%s' failed:\n", func_name);
		if (PyErr_Occurred()) PyErr_Print();
		Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
		PyThreadState_Swap(old_state);
#endif
		PyGILState_Release(gstate);
		return NULL;
	}

	w_Targs *ret = NULL;

	if (PyLong_Check(res)) {
		long l = PyLong_AsLong(res);
		ret = w_pack("l", l);
	} else if (PyFloat_Check(res)) {
		double d = PyFloat_AsDouble(res);
		ret = w_pack("d", d);
	} else if (PyUnicode_Check(res)) {
		const char *s = PyUnicode_AsUTF8(res);
		if (!s) s = "";
		ret = w_pack("s", strdup(s));
	} else if (res == Py_None) {
		ret = w_pack("l", (long)0);  // Default to 0 for None
	} else if (PyList_Check(res) || PyTuple_Check(res) || PySet_Check(res) || PyFrozenSet_Check(res)) {  //  All containers use JSON marshaling
#ifdef HAVE_RAPIDJSON
		// Use RapidJSON-based converter for all containers
		char *json_str = PyObjectToJsonString(res);
		if (json_str) {
			ret = w_pack("D", json_str);
		} else {
			log("PY: w_CallFunction - failed to convert container to JSON\n");
			ret = w_pack("D", strdup("[]"));
		}
	} else if (PyDict_Check(res)) {  //  Dict to 'D' format (JSON)
#ifdef HAVE_RAPIDJSON
		// Use RapidJSON-based converter for fast serialization
		char *json_str = PyObjectToJsonString(res);
		if (json_str) {
			ret = w_pack("D", json_str);
		} else {
			log("PY: w_CallFunction - failed to convert dict to JSON\n");
			ret = w_pack("D", strdup("{}"));
		}
#else
		// Fallback to Python json module
		PyObject *json_module = PyImport_ImportModule("json");
		if (!json_module) {
			log("PY: w_CallFunction - failed to import json module for dict return\n");
			PyErr_Clear();
			ret = w_pack("D", strdup("{}"));
		} else {
			PyObject *dumps_func = PyObject_GetAttrString(json_module, "dumps");
			if (!dumps_func) {
				log("PY: w_CallFunction - failed to get json.dumps\n");
				PyErr_Clear();
				Py_DECREF(json_module);
				ret = w_pack("D", strdup("{}"));
			} else {
				PyObject *json_str = PyObject_CallFunction(dumps_func, "O", res);
				Py_DECREF(dumps_func);
				Py_DECREF(json_module);
				if (!json_str || PyErr_Occurred()) {
					log("PY: w_CallFunction - failed to convert dict to JSON\n");
					PyErr_Clear();
					ret = w_pack("D", strdup("{}"));
				} else {
					const char *s = PyUnicode_AsUTF8(json_str);
					ret = w_pack("D", s ? strdup(s) : strdup("{}"));
					Py_DECREF(json_str);
				}
			}
		}
#endif
	} else if (PyTuple_Check(res)) {
		int len = PyTuple_Size(res);
		if (len > W_MAX_RETVALS) len = W_MAX_RETVALS;
		ret = (w_Targs*)calloc(1, sizeof(w_Targs) + len * sizeof(w_Telement));
		if (!ret) {
			Py_DECREF(res);
			Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
			PyThreadState_Swap(old_state);
#endif
			PyGILState_Release(gstate);
			return NULL;
		}
		ret->format = "tab";
		for (int i = 0; i < len; i++) {
			PyObject *item = PyTuple_GetItem(res, i);
			if (PyLong_Check(item)) {
				ret->args[i].type = 'l';
				ret->args[i].l = PyLong_AsLong(item);
			} else if (PyFloat_Check(item)) {
				ret->args[i].type = 'd';
				ret->args[i].d = PyFloat_AsDouble(item);
			} else if (PyUnicode_Check(item)) {
				const char *s = PyUnicode_AsUTF8(item);
				if (!s) s = "";
				ret->args[i].type = 's';
				ret->args[i].s = strdup(s);
			} else {
				ret->args[i].type = 0;
			}
		}
	}
#else
	if (PyInt_Check(res) || PyLong_Check(res)) {
		long l = PyLong_AsLong(res);  // Handles both in Py2
		ret = w_pack("l", l);
	} else if (PyFloat_Check(res)) {
		double d = PyFloat_AsDouble(res);
		ret = w_pack("d", d);
	} else if (PyString_Check(res)) {
		char *s = PyString_AsString(res);
		if (!s) s = "";
		ret = w_pack("s", strdup(s));
	} else if (res == Py_None) {
		ret = w_pack("l", (long)0);  // Default to 0 for None
	} else if (PyTuple_Check(res)) {
		int len = PyTuple_Size(res);
		if (len > W_MAX_RETVALS) len = W_MAX_RETVALS;
		ret = (w_Targs*)calloc(1, sizeof(w_Targs) + len * sizeof(w_Telement));
		if (!ret) {
			Py_DECREF(res);
			Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
			PyThreadState_Swap(old_state);
#endif
			PyGILState_Release(gstate);
			return NULL;
		}et->format = "tab";
		for (int i = 0; i < len; i++) {
			PyObject *item = PyTuple_GetItem(res, i);
			if (PyInt_Check(item) || PyLong_Check(item)) {
				ret->args[i].type = 'l';
				ret->args[i].l = PyLong_AsLong(item);
			} else if (PyFloat_Check(item)) {
				ret->args[i].type = 'd';
				ret->args[i].d = PyFloat_AsDouble(item);
			} else if (PyString_Check(item)) {
				char *s = PyString_AsString(item);
				if (!s) s = "";
				ret->args[i].type = 's';
				ret->args[i].s = strdup(s);
			} else {
				ret->args[i].type = 0;
			}
		}
	}
#endif

	log2("PY: Function '%s' returned successfully\n", func_name);
	Py_DECREF(res);
	Py_XDECREF(module);
#ifndef PYTHON_SINGLE_INTERPRETER
	PyThreadState_Swap(old_state);
#endif
	PyGILState_Release(gstate);
	
	// NOTE: We do NOT free params here because:
	// 1. The caller might be passing C++ .c_str() pointers that shouldn't be freed
	// 2. The caller should manage the lifetime of params
	// 3. If params contains heap-allocated strings (from strdup), caller must free them
	// This is different from callbacks which return owned w_Targs* that must be freed
	
	return ret;
}

// ============================================================================
// Dynamic C++ Function Registration
// ============================================================================

// Register a C++ callback that can be called from Python
// This allows plugins to expose custom C++ functions to Python scripts
int w_RegisterFunction(int script_id, const char *func_name, w_Tcallback callback)
{
	if (script_id < 0 || script_id >= (int)w_Scripts.size()) {
		log("PY: w_RegisterFunction - invalid script_id %d\n", script_id);
		return 0;
	}
	
	w_TScript *script = w_Scripts[script_id];
	if (!script) {
		log("PY: w_RegisterFunction - script %d not loaded\n", script_id);
		return 0;
	}
	
	if (!func_name || !callback) {
		log("PY: w_RegisterFunction - NULL func_name or callback\n");
		return 0;
	}
	
	// Initialize dynamic_funcs map if needed
	if (!script->dynamic_funcs) {
		script->dynamic_funcs = new std::map<std::string, w_Tcallback>();
	}
	
	// Register the function
	(*script->dynamic_funcs)[std::string(func_name)] = callback;
	
	log1("PY: Registered dynamic function '%s' for script %d\n", func_name, script_id);
	return 1;
}

// Unregister a dynamically registered function
int w_UnregisterFunction(int script_id, const char *func_name)
{
	if (script_id < 0 || script_id >= (int)w_Scripts.size()) {
		log("PY: w_UnregisterFunction - invalid script_id %d\n", script_id);
		return 0;
	}
	
	w_TScript *script = w_Scripts[script_id];
	if (!script || !script->dynamic_funcs) {
		return 0;
	}
	
	if (!func_name) {
		log("PY: w_UnregisterFunction - NULL func_name\n");
		return 0;
	}
	
	auto it = script->dynamic_funcs->find(std::string(func_name));
	if (it == script->dynamic_funcs->end()) {
		log("PY: w_UnregisterFunction - function '%s' not found\n", func_name);
		return 0;
	}
	
	script->dynamic_funcs->erase(it);
	log1("PY: Unregistered dynamic function '%s' for script %d\n", func_name, script_id);
	return 1;
}

// Internal helper: Call a dynamically registered C++ function from Python
static PyObject* vh_CallDynamicFunction(PyObject *self, PyObject *args)
{
	// First argument must be the function name (string)
	if (PyTuple_Size(args) < 1) {
		PyErr_SetString(PyExc_TypeError, "CallDynamicFunction requires at least function name");
		return NULL;
	}
	
	PyObject *func_name_obj = PyTuple_GetItem(args, 0);
	if (!PyUnicode_Check(func_name_obj)) {
		PyErr_SetString(PyExc_TypeError, "First argument must be function name (string)");
		return NULL;
	}
	
	const char *func_name = PyUnicode_AsUTF8(func_name_obj);
	if (!func_name) {
		PyErr_SetString(PyExc_ValueError, "Invalid function name");
		return NULL;
	}
	
	// Get script ID from vh.myid
	long script_id = vh_GetScriptID();
	if (script_id < 0) {
		PyErr_SetString(PyExc_RuntimeError, "Could not determine script ID");
		return NULL;
	}
	
	w_TScript *script = w_Scripts[script_id];
	if (!script || !script->dynamic_funcs) {
		PyErr_Format(PyExc_RuntimeError, "No dynamic functions registered for script %ld", script_id);
		return NULL;
	}
	
	auto it = script->dynamic_funcs->find(std::string(func_name));
	if (it == script->dynamic_funcs->end()) {
		PyErr_Format(PyExc_NameError, "Dynamic function '%s' not found", func_name);
		return NULL;
	}
	
	w_Tcallback callback = it->second;
	
	// Parse remaining arguments (skip first arg which is function name)
	int arg_count = PyTuple_Size(args) - 1;
	
	// Build w_Targs manually to support complex types (lists, dicts)
	w_Targs *parsed_args = (w_Targs*)calloc(arg_count + 1, sizeof(w_Telement));
	if (!parsed_args) {
		PyErr_SetString(PyExc_MemoryError, "Failed to allocate argument structure");
		return NULL;
	}
	
	char *format_str = (char*)malloc(arg_count + 1);
	if (!format_str) {
		free(parsed_args);
		PyErr_SetString(PyExc_MemoryError, "Failed to allocate format string");
		return NULL;
	}
	
	// Parse each argument and build format string
	for (int i = 0; i < arg_count; i++) {
		PyObject *arg = PyTuple_GetItem(args, i + 1);
		
		if (PyLong_Check(arg)) {
			format_str[i] = 'l';
			parsed_args->args[i].type = 'l';
			parsed_args->args[i].l = PyLong_AsLong(arg);
		}
		else if (PyUnicode_Check(arg)) {
			format_str[i] = 's';
			parsed_args->args[i].type = 's';
			const char *s = PyUnicode_AsUTF8(arg);
			parsed_args->args[i].s = s ? strdup(s) : strdup("");
		}
		else if (PyFloat_Check(arg)) {
			format_str[i] = 'd';
			parsed_args->args[i].type = 'd';
			parsed_args->args[i].d = PyFloat_AsDouble(arg);
		}
		else if (PyList_Check(arg) || PyDict_Check(arg) || PySet_Check(arg) || PyFrozenSet_Check(arg) || PyTuple_Check(arg)) {
			// All container types use JSON marshaling ('D' format)
			format_str[i] = 'D';
			parsed_args->args[i].type = 'D';
			
#ifdef HAVE_RAPIDJSON
			char *json_str = PyObjectToJsonString(arg);
			if (json_str) {
				parsed_args->args[i].s = json_str;
			} else {
				// Fallback defaults based on type
				if (PyDict_Check(arg)) {
					parsed_args->args[i].s = strdup("{}");
				} else {
					parsed_args->args[i].s = strdup("[]");
				}
			}
#else
			// Fallback to Python json module
			PyObject *json_module = PyImport_ImportModule("json");
			if (!json_module) {
				format_str[i] = '\0';
				parsed_args->format = format_str;
				w_free_args(parsed_args);
				PyErr_SetString(PyExc_ImportError, "Failed to import json module");
				return NULL;
			}
			
			PyObject *dumps_func = PyObject_GetAttrString(json_module, "dumps");
			Py_DECREF(json_module);
			
			if (!dumps_func) {
				format_str[i] = '\0';
				parsed_args->format = format_str;
				w_free_args(parsed_args);
				PyErr_SetString(PyExc_AttributeError, "Failed to get json.dumps");
				return NULL;
			}
			
			PyObject *json_str = PyObject_CallFunction(dumps_func, "O", arg);
			Py_DECREF(dumps_func);
			
			if (!json_str) {
				format_str[i] = '\0';
				parsed_args->format = format_str;
				w_free_args(parsed_args);
				PyErr_SetString(PyExc_ValueError, "Failed to serialize to JSON");
				return NULL;
			}
			
			const char *s = PyUnicode_AsUTF8(json_str);
			parsed_args->args[i].s = s ? strdup(s) : (PyList_Check(arg) ? strdup("[]") : strdup("{}"));
			Py_DECREF(json_str);
#endif
		}
		else {
			// Cleanup
			format_str[i] = '\0';
			parsed_args->format = format_str;
			w_free_args(parsed_args);
			PyErr_Format(PyExc_TypeError, "Unsupported argument type at position %d: %s", 
				i, arg->ob_type->tp_name);
			return NULL;
		}
	}
	
	format_str[arg_count] = '\0';
	parsed_args->format = format_str;
	
	// Release GIL and call C++ callback
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *result = callback(-1, parsed_args);  // Use -1 for dynamic functions
	
	// Cleanup parsed_args
	w_free_args(parsed_args);
	
	PyEval_AcquireThread(state);
	
	if (!result) Py_RETURN_NONE;
	
	// Convert result back to Python - support all types
	if (!result->format || strlen(result->format) == 0) {
		w_free_args(result);
		Py_RETURN_NONE;
	}
	
	PyObject *py_result = NULL;
	
	switch (result->format[0]) {
		case 'l': {
			long ret_val;
			w_unpack(result, "l", &ret_val);
			py_result = PyLong_FromLong(ret_val);
			break;
		}
		case 's': {
			char *ret_val;
			w_unpack(result, "s", &ret_val);
			if (ret_val) {
				py_result = PyUnicode_FromString(ret_val);
			} else {
				Py_INCREF(Py_None);
				py_result = Py_None;
			}
			break;
		}
		case 'd': {
			double ret_val;
			w_unpack(result, "d", &ret_val);
			py_result = PyFloat_FromDouble(ret_val);
			break;
		}
		case 'L': {
			// List of strings - return Python list
			char **ret_val;
			w_unpack(result, "L", &ret_val);
			
			py_result = PyList_New(0);
			if (ret_val) {
				for (int i = 0; ret_val[i] != NULL; i++) {
					PyList_Append(py_result, PyUnicode_FromString(ret_val[i]));
				}
			}
			break;
		}
		case 'D': {
			// Dict/complex as JSON string - parse and return Python object
			char *json_str;
			w_unpack(result, "D", &json_str);
			
			if (!json_str || strlen(json_str) == 0) {
				py_result = PyDict_New();
			} else {
#ifdef HAVE_RAPIDJSON
				// Use RapidJSON-based converter for fast parsing
				py_result = JsonStringToPyObject(json_str);
				if (!py_result) {
					log("PY: vh_CallDynamicFunction - failed to parse JSON\n");
					PyErr_Clear();
					py_result = PyDict_New();
				}
#else
				// Fallback to Python json module
				PyObject *json_module = PyImport_ImportModule("json");
				if (!json_module) {
					log("PY: vh_CallDynamicFunction - failed to import json\n");
					PyErr_Clear();
					py_result = PyDict_New();
					break;
				}
				
				PyObject *loads_func = PyObject_GetAttrString(json_module, "loads");
				Py_DECREF(json_module);
				
				if (!loads_func) {
					log("PY: vh_CallDynamicFunction - failed to get json.loads\n");
					PyErr_Clear();
					py_result = PyDict_New();
					break;
				}
				
				PyObject *parsed_dict = PyObject_CallFunction(loads_func, "s", json_str);
				Py_DECREF(loads_func);
				
				if (!parsed_dict || PyErr_Occurred()) {
					log("PY: vh_CallDynamicFunction - failed to parse JSON\n");
					PyErr_Clear();
					py_result = PyDict_New();
				} else {
					py_result = parsed_dict;
				}
#endif
			}
			break;
		}
		default:
			log("PY: vh_CallDynamicFunction - unsupported return type '%c'\n", result->format[0]);
			Py_INCREF(Py_None);
			py_result = Py_None;
			break;
	}
	
	w_free_args(result);
	return py_result ? py_result : Py_None;
}

PyObject *w_GetHook(int hook)
{
	// Implementation if needed, but prompt doesn't have it, perhaps not used
	return NULL;
}

const char *w_HookName(int hook)
{
	switch (hook) {
		case W_OnNewConn:                 return "OnNewConn";
		case W_OnCloseConn:               return "OnCloseConn";
		case W_OnCloseConnEx:             return "OnCloseConnEx";
		case W_OnParsedMsgChat:           return "OnParsedMsgChat";
		case W_OnParsedMsgPM:             return "OnParsedMsgPM";
		case W_OnParsedMsgMCTo:           return "OnParsedMsgMCTo";
		case W_OnParsedMsgSearch:         return "OnParsedMsgSearch";
		case W_OnParsedMsgSR:             return "OnParsedMsgSR";
		case W_OnParsedMsgMyINFO:         return "OnParsedMsgMyINFO";
		case W_OnFirstMyINFO:             return "OnFirstMyINFO";
		case W_OnParsedMsgValidateNick:   return "OnParsedMsgValidateNick";
		case W_OnParsedMsgAny:            return "OnParsedMsgAny";
		case W_OnParsedMsgAnyEx:          return "OnParsedMsgAnyEx";
		case W_OnOpChatMessage:           return "OnOpChatMessage";
		case W_OnPublicBotMessage:        return "OnPublicBotMessage";
		case W_OnUnLoad:                  return "OnUnLoad";
		case W_OnCtmToHub:                return "OnCtmToHub";
		case W_OnParsedMsgSupports:       return "OnParsedMsgSupports";
		case W_OnParsedMsgMyHubURL:       return "OnParsedMsgMyHubURL";
		case W_OnParsedMsgExtJSON:        return "OnParsedMsgExtJSON";
		case W_OnParsedMsgBotINFO:        return "OnParsedMsgBotINFO";
		case W_OnParsedMsgVersion:        return "OnParsedMsgVersion";
		case W_OnParsedMsgMyPass:         return "OnParsedMsgMyPass";
		case W_OnParsedMsgConnectToMe:    return "OnParsedMsgConnectToMe";
		case W_OnParsedMsgRevConnectToMe: return "OnParsedMsgRevConnectToMe";
		case W_OnUnknownMsg:              return "OnUnknownMsg";
		case W_OnOperatorCommand:         return "OnOperatorCommand";
		case W_OnOperatorKicks:           return "OnOperatorKicks";
		case W_OnOperatorDrops:           return "OnOperatorDrops";
		case W_OnOperatorDropsWithReason: return "OnOperatorDropsWithReason";
		case W_OnValidateTag:             return "OnValidateTag";
		case W_OnUserCommand:             return "OnUserCommand";
		case W_OnHubCommand:              return "OnHubCommand";
		case W_OnScriptCommand:           return "OnScriptCommand";
		case W_OnUserInList:              return "OnUserInList";
		case W_OnUserLogin:               return "OnUserLogin";
		case W_OnUserLogout:              return "OnUserLogout";
		case W_OnTimer:                   return "OnTimer";
		case W_OnNewReg:                  return "OnNewReg";
		case W_OnNewBan:                  return "OnNewBan";
		case W_OnSetConfig:               return "OnSetConfig";
		default:                          return NULL;
	}
}

const char *w_CallName(int callback)
{
	switch (callback) {
		case W_SendToOpChat:         return "SendToOpChat";
		case W_SendToActive:         return "SendToActive";
		case W_SendToPassive:        return "SendToPassive";
		case W_SendToActiveClass:    return "SendToActiveClass";
		case W_SendToPassiveClass:   return "SendToPassiveClass";
		case W_SendDataToUser:       return "SendDataToUser";
		case W_SendDataToAll:        return "SendDataToAll";
		case W_SendPMToAll:          return "SendPMToAll";
		case W_CloseConnection:      return "CloseConnection";
		case W_GetMyINFO:            return "GetMyINFO";
		case W_SetMyINFO:            return "SetMyINFO";
		case W_GetUserClass:         return "GetUserClass";
		case W_GetUserHost:          return "GetUserHost";
		case W_GetUserIP:            return "GetUserIP";
		case W_SetUserIP:            return "SetUserIP";
		case W_SetMyINFOFlag:        return "SetMyINFOFlag";
		case W_UnsetMyINFOFlag:      return "UnsetMyINFOFlag";
		case W_GetUserHubURL:        return "GetUserHubURL";
		case W_GetUserExtJSON:       return "GetUserExtJSON";
		case W_GetUserCC:            return "GetUserCC";
		case W_GetIPCC:              return "GetIPCC";
		case W_GetIPCN:              return "GetIPCN";
		case W_GetIPASN:             return "GetIPASN";
		case W_GetGeoIP:             return "GetGeoIP";
		case W_GetNickList:          return "GetNickList";
		case W_GetOpList:            return "GetOpList";
		case W_GetBotList:           return "GetBotList";
		case W_AddRegUser:           return "AddRegUser";
		case W_DelRegUser:           return "DelRegUser";
		case W_SetRegClass:          return "SetRegClass";
		case W_Ban:                  return "Ban";
		case W_KickUser:             return "KickUser";
		case W_DelNickTempBan:       return "DelNickTempBan";
		case W_DelIPTempBan:         return "DelIPTempBan";
		case W_ParseCommand:         return "ParseCommand";
		case W_ScriptCommand:        return "ScriptCommand";
		case W_SetConfig:            return "SetConfig";
		case W_GetConfig:            return "GetConfig";
		case W_IsRobotNickBad:       return "IsRobotNickBad";
		case W_AddRobot:             return "AddRobot";
		case W_DelRobot:             return "DelRobot";
		case W_SQL:                  return "SQL";
		case W_SQLQuery:             return "SQLQuery";
		case W_SQLFetch:             return "SQLFetch";
		case W_SQLFree:              return "SQLFree";
		case W_GetServFreq:          return "GetServFreq";
		case W_GetUsersCount:        return "GetUsersCount";
		case W_GetTotalShareSize:    return "GetTotalShareSize";
		case W_UserRestrictions:     return "UserRestrictions";
		case W_Topic:                return "Topic";
		case W_mc:                   return "mc";
		case W_classmc:              return "classmc";
		case W_usermc:               return "usermc";
		case W_pm:                   return "pm";
		case W_name_and_version:     return "name_and_version";
		case W_StopHub:              return "StopHub";
		default:                     return NULL;
	}
}