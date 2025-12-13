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
				break;
			default:
				log1("PY: pack: format string supports 'lsdp' and not '%c'\n", format[i]);
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
	char tmp[64];
	if (a->format) strcat(buf, a->format);
	strcat(buf, " [ ");
	for (unsigned int i = 0; i < strlen(a->format); i++) {
		switch (a->format[i]) {
			case 'l':
				snprintf(tmp, 64, "%ld ", a->args[i].l);
				strcat(buf, tmp);
				break;
			case 's':
				snprintf(tmp, 64, "'%s' ", a->args[i].s);
				strcat(buf, tmp);
				break;
			case 'd':
				snprintf(tmp, 64, "%f ", a->args[i].d);
				strcat(buf, tmp);
				break;
			case 'p':
				snprintf(tmp, 64, "%p ", a->args[i].p);
				strcat(buf, tmp);
				break;
		}
	}
	strcat(buf, "]");
	return buf;
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

	// Set sys.path to include script_dir and config_dir/scripts
	string pypath = script_dir + ":" + string(config_dir) + "/scripts";
	char *pypath_c = strdup(pypath.c_str());

#if PY_MAJOR_VERSION >= 3
	wchar_t *wpath = Py_DecodeLocale(pypath_c, NULL);
	if (wpath) {
		PySys_SetPath(wpath);
		PyMem_RawFree(wpath);
	} else {
		log("PY: Failed to decode locale for PySys_SetPath\n");
	}
#else
	PySys_SetPath(pypath_c);
#endif
	free(pypath_c);

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