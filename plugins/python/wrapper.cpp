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
#include "src/cserverdc.h"
#include "src/cban.h"

#include <libgen.h>  // for basename, dirname
#include <cstring>   // for strrchr

using namespace std;
using namespace nVerliHub::nEnums;

vector<w_TScript *> w_Scripts;

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
			case 'L':  // Phase 3: List of strings
			case 'D':  // Phase 3: Dictionary (as JSON string)
			case 'O':  // Phase 3: PyObject* (advanced)
				break;
			default:
				log1("PY: pack: format string supports 'lsdpLDO' and not '%c'\n", format[i]);
				return NULL;
		}
	}

	a = (w_Targs*)calloc(flen + 1, sizeof(w_Telement));

	if (!a)
		return NULL;

	a->format = format;

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
			case 'L':  // Phase 3: List of strings (NULL-terminated char**)
				a->args[i].type = 'L';
				a->args[i].L = va_arg(ap, char **);
				break;
			case 'D':  // Phase 3: Dict as JSON string (treated as 's' internally)
				a->args[i].type = 'D';
				a->args[i].s = va_arg(ap, char *);
				break;
			case 'O':  // Phase 3: PyObject* passthrough
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

	if (a->format && (strcmp(a->format, format) != 0)) {
		log1("PY: unpack: format strings don't match: '%s' vs. '%s'\n", a->format, format);
		return 0;
	}

	unsigned int flen = strlen(format);
	va_list ap;
	va_start(ap, format);

	for (unsigned int i = 0; i < flen; i++) {
		switch (format[i]) {
			case 'l':
				*va_arg(ap, long *) = a->args[i].l;
				break;
			case 's':
				*va_arg(ap, char **) = a->args[i].s;
				break;
			case 'd':
				*va_arg(ap, double *) = a->args[i].d;
				break;
			case 'p':
				*va_arg(ap, void **) = a->args[i].p;
				break;
			case 'L':  // Phase 3: List of strings
				*va_arg(ap, char ***) = a->args[i].L;
				break;
			case 'D':  // Phase 3: Dict as JSON (unpacked as string)
				*va_arg(ap, char **) = a->args[i].s;
				break;
			case 'O':  // Phase 3: PyObject* passthrough
				*va_arg(ap, PyObject **) = a->args[i].O;
				break;
			default:
				va_end(ap);
				return 0;
		}
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
			case 'L':  // Phase 3: List of strings
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
			case 'D':  // Phase 3: Dict as JSON string
				snprintf(tmp, sizeof(tmp), "JSON:'%.50s%s' ", 
					a->args[i].s ? a->args[i].s : "",
					(a->args[i].s && strlen(a->args[i].s) > 50) ? "..." : "");
				strcat(buf, tmp);
				break;
			case 'O':  // Phase 3: PyObject*
				snprintf(tmp, sizeof(tmp), "PyObject:%p ", a->args[i].O);
				strcat(buf, tmp);
				break;
		}
	}
	strcat(buf, "]");
	return buf;
}

//==============================================================================
// Python 3 vh Module Implementation - Restored from Python 2 version
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
	
	// Allocate w_Targs
	w_Targs *a = (w_Targs*)calloc(len + 1, sizeof(w_Telement));
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
					a->args[i].s = (char*)PyUnicode_AsUTF8(p);
					if (!a->args[i].s) {
						free(pack_format);
						free(a);
						return 0;  // PyUnicode_AsUTF8 set exception
					}
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
	
	free((char*)a->format);
	free(a);
	
	PyEval_AcquireThread(state);
	
	if (!res) {
		Py_RETURN_FALSE;
	}
	
	long ret = 0;
	w_unpack(res, "l", &ret);
	free(res);
	
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
	
	free((char*)a->format);
	free(a);
	
	PyEval_AcquireThread(state);
	
	if (!res) {
		Py_RETURN_NONE;
	}
	
	char *ret = NULL;
	w_unpack(res, "s", &ret);
	
	PyObject *py_ret;
	if (ret && ret[0]) {
		py_ret = PyUnicode_FromString(ret);
	} else {
		Py_INCREF(Py_None);
		py_ret = Py_None;
	}
	
	free(res);
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
	
	free((char*)a->format);
	free(a);
	
	PyEval_AcquireThread(state);
	
	if (!res) {
		Py_RETURN_NONE;
	}
	
	long ret = 0;
	w_unpack(res, "l", &ret);
	free(res);
	
	return PyLong_FromLong(ret);
}

//==============================================================================
// Forward declarations for vh module functions
//==============================================================================
static PyObject* vh_Encode(PyObject *self, PyObject *args);
static PyObject* vh_Decode(PyObject *self, PyObject *args);
static PyObject* vh_CallDynamicFunction(PyObject *self, PyObject *args);

//==============================================================================
// vh Module Functions (53 functions restored from Python 2 version)
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
static PyObject* vh_GetMyINFO(PyObject *self, PyObject *args)          { return vh_CallString(W_GetMyINFO, args, "s"); }
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
static PyObject* vh_GetIPASN(PyObject *self, PyObject *args)           { return vh_CallString(W_GetIPASN, args, "s"); }
static PyObject* vh_GetGeoIP(PyObject *self, PyObject *args)           { return vh_CallString(W_GetGeoIP, args, "s"); }
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
static PyObject* vh_GetConfig(PyObject *self, PyObject *args)          { return vh_CallString(W_GetConfig, args, "ss"); }
static PyObject* vh_IsRobotNickBad(PyObject *self, PyObject *args)     { return vh_CallLong(W_IsRobotNickBad, args, "s"); }
static PyObject* vh_AddRobot(PyObject *self, PyObject *args)           { return vh_CallBool(W_AddRobot, args, "slssss"); }
static PyObject* vh_DelRobot(PyObject *self, PyObject *args)           { return vh_CallBool(W_DelRobot, args, "s"); }
static PyObject* vh_SQL(PyObject *self, PyObject *args)                { return vh_CallString(W_SQL, args, "sl"); }
static PyObject* vh_GetServFreq(PyObject *self, PyObject *args)        { return vh_CallLong(W_GetServFreq, args, ""); }
static PyObject* vh_GetUsersCount(PyObject *self, PyObject *args)      { return vh_CallLong(W_GetUsersCount, args, ""); }
static PyObject* vh_GetTotalShareSize(PyObject *self, PyObject *args)  { return vh_CallString(W_GetTotalShareSize, args, ""); }
static PyObject* vh_usermc(PyObject *self, PyObject *args)             { return vh_CallBool(W_usermc, args, "ss"); }
static PyObject* vh_pm(PyObject *self, PyObject *args)                 { return vh_CallBool(W_pm, args, "ss"); }
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
	
	char **list = NULL;
	w_unpack(res, "L", &list);
	
	PyObject *py_list = PyList_New(0);
	if (list) {
		for (int i = 0; list[i]; i++) {
			PyList_Append(py_list, PyUnicode_FromString(list[i]));
		}
	}
	
	free(res);
	return py_list;
}

static PyObject* vh_GetOpList(PyObject *self, PyObject *args)
{
	w_Targs *res = w_Python->callbacks[W_GetOpList](W_GetOpList, w_pack(""));
	if (!res) Py_RETURN_NONE;
	
	char **list = NULL;
	w_unpack(res, "L", &list);
	
	PyObject *py_list = PyList_New(0);
	if (list) {
		for (int i = 0; list[i]; i++) {
			PyList_Append(py_list, PyUnicode_FromString(list[i]));
		}
	}
	
	free(res);
	return py_list;
}

static PyObject* vh_GetBotList(PyObject *self, PyObject *args)
{
	w_Targs *res = w_Python->callbacks[W_GetBotList](W_GetBotList, w_pack(""));
	if (!res) Py_RETURN_NONE;
	
	char **list = NULL;
	w_unpack(res, "L", &list);
	
	PyObject *py_list = PyList_New(0);
	if (list) {
		for (int i = 0; list[i]; i++) {
			PyList_Append(py_list, PyUnicode_FromString(list[i]));
		}
	}
	
	free(res);
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
	
	w_Targs *packed = w_pack("sssss", (char*)nick, (char*)nochat, (char*)nopm, (char*)nosearch, (char*)noctm);
	
	PyThreadState *state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[W_UserRestrictions](W_UserRestrictions, packed);
	
	free(packed);
	PyEval_AcquireThread(state);
	
	if (!res) Py_RETURN_FALSE;
	
	long ret = 0;
	w_unpack(res, "l", &ret);
	free(res);
	
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
	{"GetMyINFO",          vh_GetMyINFO,          METH_VARARGS, "Get user MyINFO string"},
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
	{"CallDynamicFunction", vh_CallDynamicFunction, METH_VARARGS, "Call a dynamically registered C++ function (Dimension 4)"},
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
	PyGILState_STATE gil = PyGILState_Ensure();
	
	// Clean up all remaining subinterpreters before finalizing Python
	// We need to track the main state before we start ending subinterpreters
	PyThreadState *main_state = PyThreadState_Get();
	
	for (size_t i = 0; i < w_Scripts.size(); ++i) {
		w_TScript *script = w_Scripts[i];
		if (script && script->state) {
			// Switch to the subinterpreter's thread state
			PyThreadState_Swap(script->state);
			
			// Clean up Python objects in the subinterpreter
			Py_XDECREF(script->module);
			script->module = NULL;
			
			// End this subinterpreter (this frees script->state)
			Py_EndInterpreter(script->state);
			script->state = NULL;
			
			// Switch back to main interpreter
			PyThreadState_Swap(main_state);
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
	
	// Now all subinterpreters are cleaned up, safe to finalize
	Py_Finalize();
	// No PyGILState_Release after Py_Finalize
	free(w_Python);
	return 1;
}

int w_ReserveID()
{
	w_TScript *script = (w_TScript*)calloc(1, sizeof(w_TScript));
	if (!script) return -1;
	script->id = w_Scripts.size();
	script->dynamic_funcs = NULL;  // Initialize dynamic function registry
	w_Scripts.push_back(script);
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
	script->state = Py_NewInterpreter();
	PyThreadState_Swap(main_state);
	PyGILState_Release(gil);
	if (!script->state) return -1;

	PyGILState_STATE sub_gil = PyGILState_Ensure();
	main_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);

	// Register vh module for this sub-interpreter
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

	// Import the module (this executes the script)
	script->module = PyImport_ImportModule(script->name);
	if (!script->module) {
		if (PyErr_Occurred()) PyErr_Print();
		PyThreadState_Swap(main_state);
		PyGILState_Release(sub_gil);
		return -1;
	}

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

	Py_XDECREF(script->module);

	Py_EndInterpreter(script->state);

	PyThreadState_Swap(main_state);
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

	PyGILState_STATE gstate = PyGILState_Ensure();
	PyThreadState *old_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);

	const char *name = w_HookName(num);
	if (!name) {
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	PyObject *func = PyObject_GetAttrString(script->module, name);
	if (!func || !PyCallable_Check(func)) {
		Py_XDECREF(func);
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	size_t arg_count = params->format ? strlen(params->format) : 0;
	PyObject *args = PyTuple_New(arg_count);
	if (!args) {
		Py_DECREF(func);
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	for (size_t i = 0; i < arg_count; i++) {
		switch (params->format[i]) {
			case 'l':
				PyTuple_SetItem(args, i, PyLong_FromLong(params->args[i].l));
				break;
			case 's': {
				const char *s = params->args[i].s;
#if PY_MAJOR_VERSION >= 3
				PyTuple_SetItem(args, i, s ? PyUnicode_FromString(s) : Py_None);
#else
				PyTuple_SetItem(args, i, s ? PyString_FromString(s) : Py_None);
#endif
				break;
			}
			case 'd':
				PyTuple_SetItem(args, i, PyFloat_FromDouble(params->args[i].d));
				break;
			case 'p':
				PyTuple_SetItem(args, i, PyLong_FromVoidPtr(params->args[i].p));
				break;
			default:
				Py_DECREF(args);
				Py_DECREF(func);
				PyThreadState_Swap(old_state);
				PyGILState_Release(gstate);
				return NULL;
		}
	}

	PyObject *res = PyObject_CallObject(func, args);

	Py_DECREF(args);
	Py_DECREF(func);

	if (!res) {
		if (PyErr_Occurred()) PyErr_Print();
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	w_Targs *ret = NULL;

#if PY_MAJOR_VERSION >= 3
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
			PyThreadState_Swap(old_state);
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
			PyThreadState_Swap(old_state);
			PyGILState_Release(gstate);
			return NULL;
		}
		ret->format = "tab";
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

	Py_DECREF(res);
	PyThreadState_Swap(old_state);
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
	PyThreadState *old_state = PyThreadState_Get();
	PyThreadState_Swap(script->state);

	PyObject *func = PyObject_GetAttrString(script->module, func_name);
	if (!func) {
		log("PY: w_CallFunction - function '%s' not found in script\n", func_name);
		PyErr_Clear();
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}
	
	if (!PyCallable_Check(func)) {
		log("PY: w_CallFunction - '%s' is not callable\n", func_name);
		Py_DECREF(func);
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	size_t arg_count = (params && params->format) ? strlen(params->format) : 0;
	PyObject *args = PyTuple_New(arg_count);
	if (!args) {
		log("PY: w_CallFunction - failed to create argument tuple\n");
		Py_DECREF(func);
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	for (size_t i = 0; i < arg_count; i++) {
		switch (params->format[i]) {
			case 'l':
				PyTuple_SetItem(args, i, PyLong_FromLong(params->args[i].l));
				break;
			case 's': {
				const char *s = params->args[i].s;
#if PY_MAJOR_VERSION >= 3
				PyTuple_SetItem(args, i, s ? PyUnicode_FromString(s) : Py_None);
#else
				PyTuple_SetItem(args, i, s ? PyString_FromString(s) : Py_None);
#endif
				break;
			}
			case 'd':
				PyTuple_SetItem(args, i, PyFloat_FromDouble(params->args[i].d));
				break;
			case 'p':
				PyTuple_SetItem(args, i, PyLong_FromVoidPtr(params->args[i].p));
				break;
			case 'L': {  // Phase 3: List of strings
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
#if PY_MAJOR_VERSION >= 3
					PyList_SetItem(py_list, j, PyUnicode_FromString(list[j]));
#else
					PyList_SetItem(py_list, j, PyString_FromString(list[j]));
#endif
				}
				PyTuple_SetItem(args, i, py_list);
				break;
			}
			case 'D': {  // Phase 3: Dict as JSON string
				const char *json_str = params->args[i].s;
				if (!json_str || json_str[0] == '\0') {
					PyTuple_SetItem(args, i, PyDict_New());
					break;
				}
				// Parse JSON string to Python dict using json module
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
				
#if PY_MAJOR_VERSION >= 3
				PyObject *json_arg = PyUnicode_FromString(json_str);
#else
				PyObject *json_arg = PyString_FromString(json_str);
#endif
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
				break;
			}
			case 'O':  // Phase 3: PyObject* passthrough
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
				PyThreadState_Swap(old_state);
				PyGILState_Release(gstate);
				return NULL;
		}
	}

	log2("PY: Calling function '%s' with %zu arguments\n", func_name, arg_count);
	PyObject *res = PyObject_CallObject(func, args);

	Py_DECREF(args);
	Py_DECREF(func);

	if (!res) {
		log("PY: w_CallFunction - call to '%s' failed:\n", func_name);
		if (PyErr_Occurred()) PyErr_Print();
		PyThreadState_Swap(old_state);
		PyGILState_Release(gstate);
		return NULL;
	}

	w_Targs *ret = NULL;

#if PY_MAJOR_VERSION >= 3
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
	} else if (PyList_Check(res)) {  // Phase 3: List to 'L' format
		int len = PyList_Size(res);
		char **str_array = (char **)malloc((len + 1) * sizeof(char *));
		if (!str_array) {
			log("PY: w_CallFunction - failed to allocate memory for list\n");
		} else {
			for (int i = 0; i < len; i++) {
				PyObject *item = PyList_GetItem(res, i);
				if (PyUnicode_Check(item)) {
					const char *s = PyUnicode_AsUTF8(item);
					str_array[i] = s ? strdup(s) : strdup("");
				} else {
					str_array[i] = strdup("");
				}
			}
			str_array[len] = NULL;  // NULL-terminate
			ret = w_pack("L", str_array);
		}
	} else if (PyDict_Check(res)) {  // Phase 3: Dict to 'D' format (JSON)
		// Convert dict to JSON string
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
	} else if (PyTuple_Check(res)) {
		int len = PyTuple_Size(res);
		if (len > W_MAX_RETVALS) len = W_MAX_RETVALS;
		ret = (w_Targs*)calloc(1, sizeof(w_Targs) + len * sizeof(w_Telement));
		if (!ret) {
			Py_DECREF(res);
			PyThreadState_Swap(old_state);
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
			PyThreadState_Swap(old_state);
			PyGILState_Release(gstate);
			return NULL;
		}
		ret->format = "tab";
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
	PyThreadState_Swap(old_state);
	PyGILState_Release(gstate);
	return ret;
}

// ============================================================================
// Dimension 4: Dynamic C++ Function Registration
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
		else if (PyList_Check(arg)) {
			// Convert Python list to NULL-terminated char** array
			format_str[i] = 'L';
			parsed_args->args[i].type = 'L';
			
			int list_size = PyList_Size(arg);
			char **str_array = (char**)malloc((list_size + 1) * sizeof(char*));
			if (!str_array) {
				// Cleanup
				for (int j = 0; j < i; j++) {
					if (format_str[j] == 's') free(parsed_args->args[j].s);
					else if (format_str[j] == 'L') {
						for (int k = 0; parsed_args->args[j].L && parsed_args->args[j].L[k]; k++)
							free(parsed_args->args[j].L[k]);
						free(parsed_args->args[j].L);
					}
					else if (format_str[j] == 'D') free(parsed_args->args[j].s);
				}
				free(format_str);
				free(parsed_args);
				PyErr_SetString(PyExc_MemoryError, "Failed to allocate list array");
				return NULL;
			}
			
			for (int j = 0; j < list_size; j++) {
				PyObject *item = PyList_GetItem(arg, j);
				if (PyUnicode_Check(item)) {
					const char *s = PyUnicode_AsUTF8(item);
					str_array[j] = s ? strdup(s) : strdup("");
				} else {
					str_array[j] = strdup("");
				}
			}
			str_array[list_size] = NULL;
			parsed_args->args[i].L = str_array;
		}
		else if (PyDict_Check(arg)) {
			// Convert Python dict to JSON string
			format_str[i] = 'D';
			parsed_args->args[i].type = 'D';
			
			// Import json module
			PyObject *json_module = PyImport_ImportModule("json");
			if (!json_module) {
				// Cleanup
				for (int j = 0; j < i; j++) {
					if (format_str[j] == 's') free(parsed_args->args[j].s);
					else if (format_str[j] == 'L') {
						for (int k = 0; parsed_args->args[j].L && parsed_args->args[j].L[k]; k++)
							free(parsed_args->args[j].L[k]);
						free(parsed_args->args[j].L);
					}
					else if (format_str[j] == 'D') free(parsed_args->args[j].s);
				}
				free(format_str);
				free(parsed_args);
				PyErr_SetString(PyExc_ImportError, "Failed to import json module");
				return NULL;
			}
			
			PyObject *dumps_func = PyObject_GetAttrString(json_module, "dumps");
			Py_DECREF(json_module);
			
			if (!dumps_func) {
				// Cleanup
				for (int j = 0; j < i; j++) {
					if (format_str[j] == 's') free(parsed_args->args[j].s);
					else if (format_str[j] == 'L') {
						for (int k = 0; parsed_args->args[j].L && parsed_args->args[j].L[k]; k++)
							free(parsed_args->args[j].L[k]);
						free(parsed_args->args[j].L);
					}
					else if (format_str[j] == 'D') free(parsed_args->args[j].s);
				}
				free(format_str);
				free(parsed_args);
				PyErr_SetString(PyExc_AttributeError, "Failed to get json.dumps");
				return NULL;
			}
			
			PyObject *json_str = PyObject_CallFunction(dumps_func, "O", arg);
			Py_DECREF(dumps_func);
			
			if (!json_str) {
				// Cleanup
				for (int j = 0; j < i; j++) {
					if (format_str[j] == 's') free(parsed_args->args[j].s);
					else if (format_str[j] == 'L') {
						for (int k = 0; parsed_args->args[j].L && parsed_args->args[j].L[k]; k++)
							free(parsed_args->args[j].L[k]);
						free(parsed_args->args[j].L);
					}
					else if (format_str[j] == 'D') free(parsed_args->args[j].s);
				}
				free(format_str);
				free(parsed_args);
				PyErr_SetString(PyExc_ValueError, "Failed to serialize dict to JSON");
				return NULL;
			}
			
			const char *s = PyUnicode_AsUTF8(json_str);
			parsed_args->args[i].s = s ? strdup(s) : strdup("{}");
			Py_DECREF(json_str);
		}
		else {
			// Cleanup
			for (int j = 0; j < i; j++) {
				if (format_str[j] == 's') free(parsed_args->args[j].s);
				else if (format_str[j] == 'L') {
					for (int k = 0; parsed_args->args[j].L && parsed_args->args[j].L[k]; k++)
						free(parsed_args->args[j].L[k]);
					free(parsed_args->args[j].L);
				}
				else if (format_str[j] == 'D') free(parsed_args->args[j].s);
			}
			free(format_str);
			free(parsed_args);
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
	if (parsed_args) {
		if (parsed_args->format) {
			for (int i = 0; parsed_args->format[i]; i++) {
				if (parsed_args->format[i] == 's' && parsed_args->args[i].s)
					free(parsed_args->args[i].s);
				else if (parsed_args->format[i] == 'L' && parsed_args->args[i].L) {
					for (int j = 0; parsed_args->args[i].L[j]; j++)
						free(parsed_args->args[i].L[j]);
					free(parsed_args->args[i].L);
				}
				else if (parsed_args->format[i] == 'D' && parsed_args->args[i].s)
					free(parsed_args->args[i].s);
			}
			free((void*)parsed_args->format);
		}
		free(parsed_args);
	}
	
	PyEval_AcquireThread(state);
	
	if (!result) Py_RETURN_NONE;
	
	// Convert result back to Python - support all types
	if (!result->format || strlen(result->format) == 0) {
		free(result);
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
			// Dict as JSON string - parse and return Python dict
			char *json_str;
			w_unpack(result, "D", &json_str);
			
			if (!json_str || strlen(json_str) == 0) {
				py_result = PyDict_New();
			} else {
				// Import json module
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
			}
			break;
		}
		default:
			log("PY: vh_CallDynamicFunction - unsupported return type '%c'\n", result->format[0]);
			Py_INCREF(Py_None);
			py_result = Py_None;
			break;
	}
	
	free(result);
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