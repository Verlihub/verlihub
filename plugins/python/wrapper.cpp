/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net

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

using namespace std;

vector<w_TScript*> w_Scripts;

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

// TODO: use some better string handling tools ... and maybe use more C++ ;)

void w_LogLevel (int level)
{
	log_level = level;
}

char * w_SubStr (char *s, int _start, int _end)  // similar to python [_start:_end] slice
{
	int start = _start, end = _end, len, len2;
	char *s2;
	len = strlen(s);
	if (start < 0) start = 0;
	if (start >= len) return strdup("");
	if (end < 0) end = len + end;
	if (end == 0) end = len;  // this is other than in python: here [0:0] will return the whole string
	if (end <= start) return strdup("");
	if (end > len) end = len;
	
	len2 = end - start;
	s2 = (char*)malloc(len2 + 1);
	s2[len2] = 0;
	s2 = strncpy (s2, &s[start], len2);
	return s2;
}

int w_IdentStr (char *s1, char *s2, int n)
{
	int len1, len2, i;
	len1 = strlen(s1);
	len2 = strlen(s2);
	if (n > 0 && n < len1) len1 = n;
	if (n > 0 && n < len2) len2 = n;
	if (len1 != len2) return 0;
	for (i=0; i<len1; i++)
		if (s1[i] != s2[i]) return 0;
	return 1;
}

int w_FindStr (char *s, char *key, int start)
{
	if (start < 0) start = 0;
	int len, len1, len2, i;
	len1 = strlen(key);
	len = strlen(s);
	if (len1 > len || len1 == 0 || len == 0) return -1;
	len2 = len - len1 + 1;
	for (i=start; i<len2; i++)
		if (s[i]==key[1])
		{
			if ( w_IdentStr (&s[i], key, len1) )
				return i;	
		}
	return -1;
}


w_TScript *w_Python = NULL;


// Call accepts only the following in_format PyArg_ParseTuple format characters:
// "s" (string or Unicode object) [const char *] --- Call treats "s" like "z" so object can be string or None!
// "z" (string or None) [const char *]
// "l" (integer) [long int]
// "d" (float) [double]
// "O" (object) [PyObject *]
// "|" remaining arguments are optional
// ":" end of format string
// ";" end of format string

const char* w_packprint (w_Targs* a);  // forward declaration

w_Targs* w_vapack (const char* format, va_list ap )
{
	w_Targs* a;
	for (unsigned int i=0; i < strlen(format); i++)
		switch (format[i])
		{
			case 'l': case 's': case 'd': case 'p': break;
			default: log1("PY: pack: format string supports 'lsdp' and not '%c'\n", format[i]); return NULL;
		}
	a = (w_Targs*) calloc(strlen(format)+1, sizeof(w_Telement));
	if (!a) return NULL;
	a->format = format;
	for (unsigned int i=0; i < strlen(format); i++)
		switch (format[i])
		{
			case 'l': a->args[i].type = 'l'; a->args[i].l = va_arg(ap, long);   break;
			case 's': a->args[i].type = 's'; a->args[i].s = va_arg(ap, char*);  break;
			case 'd': a->args[i].type = 'd'; a->args[i].d = va_arg(ap, double); break;
			case 'p': a->args[i].type = 'p'; a->args[i].p = va_arg(ap, void*);  break;
		}
	if (log_level > 5) { log("PY: pack   format: %s\n", w_packprint(a)); }
	return a;
}

int w_vaunpack (w_Targs* a, const char* format, va_list ap )
{
	long *l;
	char **s;
	double *d;
	void **p;
	
	if (!a || !a->format) return 0;
	if (strcmp (format, a->format) != 0) return 0;
	for (unsigned int i=0; i < strlen(format); i++)
	{
		switch (format[i])
		{
			case 'l': case 's': case 'd': case 'p': break;
			default: log1("PY: unpack: format string supports 'lsdp' and not '%c'\n", format[i]); return 0;
		}
		if (format[i] != a->args[i].type)
		{ log1("PY: unpack: format string and stored argument types don't match!\n"); return 0; }
	}
	a->format = format;
	for (unsigned int i=0; i < strlen(format); i++)
		switch (format[i])
		{
			case 'l': l = va_arg(ap, long*);   *l = a->args[i].l;  break;
			case 's': s = va_arg(ap, char**);  *s = a->args[i].s;  break;
			case 'd': d = va_arg(ap, double*); *d = a->args[i].d;  break;
			case 'p': p = va_arg(ap, void**);  *p = a->args[i].p;  break;
		}
	return 1; // success
}


w_Targs* w_pack (const char* format, ... )
{
	w_Targs* res;
	va_list ap;
	va_start(ap, format);
	res = w_vapack (format, ap);
	va_end(ap);
	return res;
}

int w_unpack (w_Targs* a, const char* format, ... )
{
	int res;
	va_list ap;
	va_start(ap, format);
	res = w_vaunpack (a, format, ap);
	va_end(ap);
	return res;
}

const char* w_packprint (w_Targs* a)
{
	static string o;
	if (!a || !a->format) return "(null)";
	o = string() + a->format + " ( ";
	char *buf = (char*) calloc(410, sizeof(char));
	for (unsigned int i=0; i < strlen(a->format); i++)
	{
		if (i > 0) o += ", ";
		switch (a->format[i])
		{
			case 'l': snprintf( buf, 400, "l:%ld", a->args[i].l); o += buf;  break;
			case 's': snprintf( buf, 400, "s:%s",  a->args[i].s); o += buf;  break;
			case 'd': snprintf( buf, 400, "l:%f",  a->args[i].d); o += buf;  break;
			case 'p': snprintf( buf, 400, "l:%p",  a->args[i].p); o += buf;  break;
			default:  o += "invalid";
		}
	}
	free(buf);
	o += " )";
	return o.c_str();
}

const char *GetName(const char *path)
{
	if (!path || !strlen(path)) return NULL;
	for (int i = strlen(path) - 1; i >= 0; i--)
		if (path[i] == '/' || path[i] == '\\') return &path[i+1];
	return path;
}

int GetFreeID()
{
	unsigned int id = 0;
	while (id < w_Scripts.size())
	{
		if (!w_Scripts[id]) return id;
		id++;
	}
	w_Scripts.push_back(NULL);
	return id;
}

int GetID()
{
	long id = -1;
	PyObject *m, *f;
	m = PyDict_GetItemString(PyImport_GetModuleDict(), "vh");  // m is a borrowed reference, do not decref!
	if (!m) { log("PY: GetID: Can't get vh module\n"); return -1; }
	if (!PyObject_HasAttrString(m, "myid")) { log("PY: GetID: vh module had no myid attribute\n"); return -1; }
	f = PyObject_GetAttrString(m, "myid");
	if (PyInt_Check(f))
	{
		id = PyInt_AsLong(f);
		Py_XDECREF(f);
		if ((id > -1) && ((unsigned long)id < w_Scripts.size()) && w_Scripts[id]) return id;
		log("PY: GetID: no script pointer found at retrieved id: %ld\n", id); return -1;
	}
	return -1;
}	
	
int Call (int func, PyObject *args, const char *in_format, const char *out_format, ... )
{
	if (func < 0 || func >= W_MAX_CALLBACKS || !args || !in_format || !out_format) return 0;
	if ( !w_Python->callbacks[func] ) return 0;
	long id = GetID();
	if (id < 0) { log("PY: Call %s: Couldn't get interpreter ID! Aborting call.\n", w_CallName(func)); return 0; }
	const char *name = w_Scripts[id]->name;
	if (!PyTuple_CheckExact(args)) { log1("PY: [%ld:%s] Call %s: args is not a python tuple!\n", id, name, w_CallName(func)); return 0; }
	char *pack_format = (char*) calloc (strlen(in_format)+5, sizeof(char));
	if (!pack_format) { log("PY: [%ld:%s] Call %s: malloc failed!\n", id, name, w_CallName(func)); return 0; }
	int required = -1;
	int in_len = strlen(in_format);
	for (int pos=0, pos2=0, end=0; pos < in_len && end < 1; pos++)
	{
		log5("PY: [%ld:%s] Call %s: scanning arguments: pos:%d, pos2:%d, char:%c\n", id, name, w_CallName(func), pos, pos2, in_format[pos]);
		switch (in_format[pos])
		{
			case 'l': pack_format[pos2] = 'l'; ++pos2; break;
			case 'z':
			case 's': pack_format[pos2] = 's'; ++pos2; break;
			case 'd': pack_format[pos2] = 'd'; ++pos2; break;
			case '0': pack_format[pos2] = 'p'; ++pos2; break;
			case ':':
			case ';': end = 1; break;
			case '|': if (required > -1) 
					{ log1("PY: [%ld:%s] Call %s: 2 pipe chars found inside the format string\n", id, name, w_CallName(func)); free(pack_format); return 0; }
				required = pos; break;
			default: log1("PY: [%ld:%s] Call %s: unsupported format character: '%c'\n", id, name, w_CallName(func), in_format[pos]); free(pack_format); return 0;
		}
	}
	int len = strlen(pack_format);
	if (required < 0) required = len;
	
	int tlen = PyTuple_Size(args);
	if (tlen < required)  
	{
		log1("PY: [%ld:%s] Call %s: too few arguments: need %d but got %d\n", id, name, w_CallName(func), required, tlen);
		PyErr_SetString(PyExc_TypeError, "too few arguments");
		free(pack_format);
		return 0;
	}
	if (tlen > len)       
	{
		log1("PY: [%ld:%s] Call %s: too many arguments: need min %d, max %d but got %d\n", id, name, w_CallName(func), required, len, tlen);
		PyErr_SetString(PyExc_TypeError, "too many arguments");
		free(pack_format);
		return 0;
	}
	
	w_Targs *a = (w_Targs*) calloc(len+1, sizeof(w_Telement));
	if (!a) { log("PY: [%ld:%s] Call %s: malloc failed!\n", id, name, w_CallName(func)); free(pack_format); return 0; }
	a->format = pack_format;
	
	for (int i=0; i < len; ++i)
	{
		if (i >= tlen)
		{
			switch (pack_format[i])
			{
				case 'l': a->args[i].type = 'l'; a->args[i].l = (long)0; break;
				case 's': a->args[i].type = 's'; a->args[i].s = (char*)NULL; break;
				case 'd': a->args[i].type = 'd'; a->args[i].d = 0.0; break;
				case 'p': a->args[i].type = 'p'; a->args[i].p = NULL; break;
			}
			continue;
		}
		PyObject *p = PyTuple_GetItem( args, i );
		if (!p) { log1("PY: [%ld:%s] Call %s: failed to read argument %d\n", id, name, w_CallName(func), i+1); free(pack_format); free(a); return 0; }
		switch (pack_format[i])
		{
			case 'l': 
				a->args[i].type = 'l';
				if (!PyInt_Check(p)) { log1("PY: [%ld:%s] Call %s: argument %d was not an int object\n", id, name, w_CallName(func), i+1); free(pack_format); free(a); return 0; }
				a->args[i].l = PyInt_AsLong(p);
				break;
			case 's': 
				a->args[i].type = 's';
				if (p == Py_None) { a->args[i].s = (char*)NULL; break; }
				if (!PyString_Check(p)) { log1("PY: [%ld:%s] Call %s: argument %d was not a string object\n", id, name, w_CallName(func), i+1); free(pack_format); free(a); return 0; }
				a->args[i].s = PyString_AsString(p);
				break;
			case 'd': 
				a->args[i].type = 'd';
				if (!PyFloat_Check(p)) { log1("PY: [%ld:%s] Call %s: argument %d was not a float object\n", id, name, w_CallName(func), i+1); free(pack_format); free(a); return 0; }
				a->args[i].d = PyFloat_AsDouble(p);
				break;
			case 'p': 
				a->args[i].type = 'p';
				if (p == Py_None) { a->args[i].p = NULL; break; }
				a->args[i].p = (void*) p;
				break;
		}
		
	}
	
	log2("PY: [%ld:%s] Call %s arguments: %s\n", id, name, w_CallName(func), w_packprint(a));
	
	PyThreadState * state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	
	w_Targs *res = w_Python->callbacks[func] (id, a);
	free(pack_format); free(a);
	
	PyEval_AcquireThread(state);
	
	log2("PY: [%ld:%s] Call %s return values: %s\n", id, name, w_CallName(func), w_packprint(res));
	if (!res) { return 0; }
	
	va_list ap;
	va_start(ap, out_format);
	int i = w_vaunpack (res, out_format, ap);
	va_end(ap);
	
	free(res); 
	if (i) return 1;
	return 0;
}

long BasicCall (int func, PyObject *args, const char *in_format)
{
	const char *out_format = "l";
	long out = 0;
	if ( Call(func, args, in_format, out_format, &out) ) return out;
	return 0;
}

static PyObject * __SendToOpChat(PyObject *self, PyObject *args) // (data)
{
	return pybool(BasicCall(W_SendToOpChat, args, "s"));
}

static PyObject * __SendToActive(PyObject *self, PyObject *args) // (data)
{
	return pybool(BasicCall(W_SendToActive, args, "s"));
}

static PyObject * __SendToPassive(PyObject *self, PyObject *args) // (data)
{
	return pybool(BasicCall(W_SendToPassive, args, "s"));
}

static PyObject * __SendToActiveClass(PyObject *self, PyObject *args) // (data, min_class, max_class)
{
	return pybool(BasicCall(W_SendToActiveClass, args, "sll"));
}

static PyObject * __SendToPassiveClass(PyObject *self, PyObject *args) // (data, min_class, max_class)
{
	return pybool(BasicCall(W_SendToPassiveClass, args, "sll"));
}

static PyObject * __SendDataToUser(PyObject *self, PyObject *args)  // (data, nick)
{	return pybool( BasicCall( W_SendDataToUser, args, "ss" ) );	}

static PyObject * __SendDataToAll(PyObject *self, PyObject *args)  // (data, min_class, max_class)
{	return pybool( BasicCall( W_SendDataToAll, args, "sll" ) );	}

static PyObject * __SendPMToAll(PyObject *self, PyObject *args)  // (data, from, min_class, max_class)
{	return pybool( BasicCall( W_SendPMToAll, args, "ssll" ) );	}

static PyObject * __CloseConnection(PyObject *self, PyObject *args)  // (nick)
{	return pybool( BasicCall( W_CloseConnection, args, "s" ) );	}

static PyObject * __GetMyINFO(PyObject *self, PyObject *args)  // (nick)
{
	const char *nick, *desc, *tag, *speed, *mail, *size;
	if (Call( W_GetMyINFO, args, "s", "ssssss", &nick, &desc, &tag, &speed, &mail, &size ))
	{
		PyObject *p = Py_BuildValue ( "(ssssss)", nick, desc, tag, speed, mail, size );
		freee(nick); freee(desc); freee(tag); freee(speed); freee(mail); freee(size);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __SetMyINFO(PyObject *self, PyObject *args)  // (nick, desc, tag, speed, mail, size)
{	return pybool( BasicCall( W_SetMyINFO, args, "s|sssss" ) );	}

static PyObject * __GetUserClass(PyObject *self, PyObject *args)  // (nick)
{
	long uclass = -1;
	if (Call( W_GetUserClass, args, "s", "l", &uclass )) return Py_BuildValue ( "l", uclass );
	Py_RETURN_NONE;
}

static PyObject * __GetRawNickList(PyObject *self, PyObject *args)
{
	char *lst;
	if (Call( W_GetNickList, args, "", "s", &lst ))
	{
		PyObject *p = Py_BuildValue ( "s", lst );
		freee(lst);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __GetRawOpList(PyObject *self, PyObject *args)
{
	char *lst;
	if (Call( W_GetOpList, args, "", "s", &lst ))
	{
		PyObject *p = Py_BuildValue ( "s", lst );
		freee(lst);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __GetNickList(PyObject *self, PyObject *args)
{
	char *rawlist;
	int len, pos = 10, i;
	if (!Call( W_GetNickList, args, "", "s", &rawlist )) Py_RETURN_NONE;
	
	len = strlen(rawlist);   // rawlist looks like this: "$NickList nick1$$nick2$$lastnick$$"
	if (len <= 12 || !w_IdentStr(rawlist, (char*)"$NickList ", 10)) Py_RETURN_NONE;
	
	PyObject* lst = PyList_New (0);
	while (1)
	{
		i = w_FindStr(rawlist, (char*)"$$", pos);
		if (i < 0) break;
		PyList_Append (lst, Py_BuildValue ("s", w_SubStr(rawlist, pos, i)));
		pos = i + 2;
	}
	freee(rawlist);
	return lst;
}


static PyObject * __GetOpList(PyObject *self, PyObject *args)
{
	char *rawlist;
	int len, pos = 8, i;
	if (!Call( W_GetOpList, args, "", "s", &rawlist )) Py_RETURN_NONE;
	
	len = strlen(rawlist);   // rawlist looks like this: "$OpList nick1$$nick2$$lastnick$$"
	if (len <= 10 || !w_IdentStr(rawlist, (char*)"$OpList ", 8)) Py_RETURN_NONE;
	
	PyObject* lst = PyList_New (0);
	while (1)
	{
		i = w_FindStr(rawlist, (char*)"$$", pos);
		if (i < 0) break;
		PyList_Append (lst, Py_BuildValue ("s", w_SubStr(rawlist, pos, i)));
		pos = i + 2;
	}
	freee(rawlist);
	return lst;
	
}

static PyObject * __GetUserHost(PyObject *self, PyObject *args)  // (nick)
{
	char *res;
	if (Call( W_GetUserHost, args, "s", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __GetUserIP(PyObject *self, PyObject *args)  // (nick)
{
	char *res;
	if (Call( W_GetUserIP, args, "s", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __GetUserCC(PyObject *self, PyObject *args)  // (nick)
{
	char *res;
	if (Call( W_GetUserCC, args, "s", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __GetIPCC(PyObject *self, PyObject *args)  // (ip)
{
	char *res;
	if (Call( W_GetIPCC, args, "s", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __GetIPCN(PyObject *self, PyObject *args)  // (ip)
{
	char *res;
	if (Call( W_GetIPCN, args, "s", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __Ban(PyObject *self, PyObject *args)  // (nick, time, type)
{	return pybool( BasicCall( W_Ban, args, "ssl" ) );	}

static PyObject * __KickUser(PyObject *self, PyObject *args)  // (op, nick, data)
{	return pybool( BasicCall( W_KickUser, args, "sss" ) );	}

static PyObject * __ParseCommand(PyObject *self, PyObject *args)  // (cmd)
{	return pybool( BasicCall( W_ParseCommand, args, "s" ) );	}

static PyObject * __SetConfig(PyObject *self, PyObject *args)  // (conf, var, val)
{	return pybool( BasicCall( W_SetConfig, args, "sss" ) );	}

static PyObject * __GetConfig(PyObject *self, PyObject *args)  // (conf, var)
{
	char *res;
	if (Call( W_GetConfig, args, "ss", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __AddRobot(PyObject *self, PyObject *args)  // (nick, uclass, desc, speed, email, share)
{	return pybool( BasicCall( W_AddRobot, args, "slssss" ) );	}

static PyObject * __DelRobot(PyObject *self, PyObject *args)  // (nick)
{	return pybool( BasicCall( W_DelRobot, args, "s" ) );	}

static PyObject * __SQL(PyObject *self, PyObject *args)  // (query, limit)
{
	char **fields;
	long rows, cols, res;
	if (!Call( W_SQL, args, "s|l", "lllp", &res, &rows, &cols, (void**) &fields )) return Py_BuildValue ( "(lO)", (long)0, PyList_New (0) );
	if (!fields) return Py_BuildValue ( "(lO)", res, PyList_New (0) );
	PyObject* ret = PyTuple_New (2);
	PyObject* lst = PyList_New (0);
	for (int row=0; row < rows && res > 0; row++)
	{
		PyObject* pyrow = PyList_New (0);
		for (int col=0; col < cols; col++)
		{
			log5("PY: wrapper::SQL   adding table element [%ld] --> [%d,%d] = ", row*cols + col, row, col);
			log5("%s\n", fields[row*cols + col]);
			if (!fields[row*cols + col]) { res = 0; break; }
			PyList_Append (pyrow, Py_BuildValue ("s", fields[row*cols + col]));
			free (fields[row*cols + col]);
		}
		if (!res) break;
		PyList_Append (lst, pyrow);
	}
	free (fields);
	PyTuple_SetItem( ret, 0, PyLong_FromLong(res) );
	PyTuple_SetItem( ret, 1, lst );
	return ret;
}

static PyObject * __GetUsersCount(PyObject *self, PyObject *args)
{	return Py_BuildValue ( "l", BasicCall( W_GetUsersCount, args, "" ) );	}

static PyObject * __GetTotalShareSize(PyObject *self, PyObject *args)
{
	char *res;
	if (Call( W_GetTotalShareSize, args, "", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}

static PyObject * __usermc(PyObject *self, PyObject *args)  //  (data, nick)
{	return pybool( BasicCall( W_usermc, args, "ss" ) );	}

static PyObject * __pm(PyObject *self, PyObject *args)
{	return pybool( BasicCall( W_pm, args, "ss" ) );	}

static PyObject * __mc(PyObject *self, PyObject *args)
{	return pybool( BasicCall( W_mc, args, "s" ) );	}

static PyObject * __classmc(PyObject *self, PyObject *args)
{	return pybool( BasicCall( W_classmc, args, "sll" ) );	}

static PyObject * __UserRestrictions(PyObject *self, PyObject *args, PyObject *keywds)
{
	long ret;
	w_Targs *res, *params;
	char *user;
	char *nochattime = (char*)""; // "" == don't change, "0" == unset, "1" == set default, "3h" == 3 hours (can use any valid VH time)
	char *nopmtime = (char*)"";
	char *nosearchtime = (char*)"";
	char *noctmtime = (char*)"";
	static char *kwlist[] = {(char*)"nick", (char*)"chat", (char*)"pm", (char*)"search", (char*)"ctm", 0};
	
	long id = GetID();
	if (id < 0) { log("PY: Call UserRestrictions: Couldn't get interpreter ID! Aborting call.\n"); return NULL; }
	
	if ( !w_Python->callbacks[W_UserRestrictions] ) return NULL;
	if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|ssss:UserRestrictions", kwlist, &user, &nochattime, &nopmtime, &nosearchtime, &noctmtime)) return NULL;
	
	PyThreadState * state = PyThreadState_Get();
	PyEval_ReleaseThread(state);
	params = w_pack( "sssss", user, nochattime, nopmtime, nosearchtime, noctmtime);
	res = w_Python->callbacks[W_UserRestrictions] (id, params);
	free(params);
	PyEval_AcquireThread(state);
	
	
	if (!w_unpack(res, "l", &ret)) { if (res) free(res); return NULL; }
	free(res);
	
	PyObject *dict = PyDict_New();
	if (!dict) return NULL;
	if (ret < 0) return dict; // an error has occured, returning an empty dictionary
	
	PyDict_SetItemString (dict, "chat", 	pybool( ret & w_UR_CHAT )); 	//< cannot talk in the main chat
	PyDict_SetItemString (dict, "pm", 	pybool( ret & w_UR_PM )); 	//< cannot send private messages
	PyDict_SetItemString (dict, "search", 	pybool( ret & w_UR_SEARCH )); 	//< cannot search
	PyDict_SetItemString (dict, "ctm", 	pybool( ret & w_UR_CTM )); 	//< cannot start downloads
	return dict;
}


static PyObject * __Topic(PyObject *self, PyObject *args)
{	
	char *res;
	if (Call( W_Topic, args, "|s", "s", &res ))
	{
		PyObject *p = Py_BuildValue ( "s", res );
		freee(res);
		return p;
	}
	Py_RETURN_NONE;
}


static PyObject * __encode(PyObject *self, PyObject *args)
{
	char *data;
	ostringstream dest;
	if ( !PyArg_ParseTuple(args, "s:encode", &data) ) return NULL;
	for (unsigned int i=0; i < strlen(data); i++)
	{
		switch (data[i])
		{
			//case 0: case 5: case 36: case 96: case 124: case 126:
			case 5: case '$': case '`': case '|': case '~':
				dest << "&#" << unsigned(data[i]) << ";"; break;
			default: dest << data[i];
		}
	}
	return Py_BuildValue ( "s", dest.str().c_str() );
}

static PyObject * __decode(PyObject *self, PyObject *args)
{
	char *data;
	string str;
	string      t1 = "&#5;",  t2 = "&#36;",  t3 = "&#96;",  t4 = "&#124;",  t5 = "&#126;";
	int         l1 = 4,       l2 = 5,        l3 = 5,        l4 = 6,         l5 = 6;
	const char *r1 = "\x05", *r2 = "$",     *r3 = "`",     *r4 = "|",      *r5 = "~";
	ostringstream dest;
	if ( !PyArg_ParseTuple(args, "s:decode", &data) ) return NULL;
	str = data;
	size_t pos = 0, len = str.length();
	while (true)
	{
		size_t fpos = str.find("&#", pos);
		if (fpos == string::npos) { dest << str.substr(pos); break; }
		dest << str.substr(pos, fpos-pos);
		if ( fpos + l1 <= len && str.substr(pos, l1) == t1) { dest << r1; pos += l1; continue; }
		if ( fpos + l2 <= len && str.substr(pos, l2) == t2) { dest << r2; pos += l2; continue; }
		if ( fpos + l3 <= len && str.substr(pos, l3) == t3) { dest << r3; pos += l3; continue; }
		if ( fpos + l4 <= len && str.substr(pos, l4) == t4) { dest << r4; pos += l4; continue; }
		if ( fpos + l5 <= len && str.substr(pos, l5) == t5) { dest << r5; pos += l5; continue; }
	}
	return Py_BuildValue ( "s", dest.str().c_str() );
}


static PyMethodDef w_vh_methods[] = {
	{"SendToOpChat", __SendToOpChat, METH_VARARGS},
	{"SendToActive", __SendToActive, METH_VARARGS},
	{"SendToPassive", __SendToPassive, METH_VARARGS},
	{"SendToActiveClass", __SendToActiveClass, METH_VARARGS},
	{"SendToPassiveClass", __SendToPassiveClass, METH_VARARGS},
	{"SendDataToUser",		__SendDataToUser,		METH_VARARGS},
	{"SendDataToAll",		__SendDataToAll,		METH_VARARGS},
	{"SendPMToAll",			__SendPMToAll,			METH_VARARGS},
	{"CloseConnection",		__CloseConnection,		METH_VARARGS},
	{"GetMyINFO",			__GetMyINFO,			METH_VARARGS},
	{"SetMyINFO",			__SetMyINFO,			METH_VARARGS},
	{"GetUserClass",		__GetUserClass,			METH_VARARGS},
	{"GetRawNickList",		__GetRawNickList,		METH_VARARGS},
	{"GetRawOpList",		__GetRawOpList,			METH_VARARGS},
	{"GetNickList",			__GetNickList,			METH_VARARGS},
	{"GetOpList",			__GetOpList,			METH_VARARGS},
	{"GetUserHost",			__GetUserHost,			METH_VARARGS},
	{"GetUserIP",			__GetUserIP,			METH_VARARGS},
	{"GetUserCC",			__GetUserCC,			METH_VARARGS},
	{"GetIPCC",			    __GetIPCC,			    METH_VARARGS},
	{"GetIPCN",			    __GetIPCN,			    METH_VARARGS},
	{"Ban",				__Ban,				METH_VARARGS},
	{"KickUser",			__KickUser,			METH_VARARGS},
	{"ParseCommand",		__ParseCommand,			METH_VARARGS},
	{"SetConfig",			__SetConfig,			METH_VARARGS},
	{"GetConfig",			__GetConfig,			METH_VARARGS},
	{"AddRobot",			__AddRobot,			METH_VARARGS},
	{"DelRobot",			__DelRobot,			METH_VARARGS},
	{"SQL",				__SQL,				METH_VARARGS},
	{"GetUsersCount",		__GetUsersCount,		METH_VARARGS},
	{"GetTotalShareSize",		__GetTotalShareSize,		METH_VARARGS},
	{"usermc",			__usermc,			METH_VARARGS},
	{"pm",				__pm,				METH_VARARGS},
	{"mc",				__mc,				METH_VARARGS},
	{"classmc",			__classmc,			METH_VARARGS},
	{"UserRestrictions",		(PyCFunction)__UserRestrictions,	METH_VARARGS|METH_KEYWORDS},
	{"Topic",			__Topic,			METH_VARARGS},
	{"encode",			__encode,			METH_VARARGS},
	{"decode",			__decode,			METH_VARARGS},
	//{"",		__,		METH_VARARGS},
	//{"",		__,		METH_VARARGS},
	//{"",		__,		METH_VARARGS},
	{NULL, NULL}
};



int w_Begin(w_Tcallback* cblist)
{
	w_Python = (w_TScript*) calloc (1, sizeof(w_TScript));
	w_Python->callbacks = (w_Tcallback*) calloc(W_MAX_CALLBACKS, sizeof(void*));
	w_Python->name = "core";
	w_Python->path = "core";
	
	PyEval_InitThreads();
	Py_Initialize();
	w_Python->state = PyThreadState_Get();	
	if (w_Python->state)
	{
		int i;
		if (cblist)
			for (i=0; i<W_MAX_CALLBACKS; i++)
				w_Python->callbacks[i] = cblist[i];
	}
	PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();
	
	w_Scripts.reserve(10);
	return (w_Python->state) ? 1 : 0;
}


int w_End()
{
	if (!w_Python) return 0;
	for (unsigned int id = 0; id < w_Scripts.size(); id++)
		if (w_Scripts[id]) { log2( "PY: End   found a running interpreter. Shutting it down first then ending\n"); w_Unload(id); }
	
	if (w_Python->state) 
	{
		log3("PY: End   found main thread state, attempting to acquire it...\n");
		PyEval_AcquireThread(w_Python->state);
		log3("PY: End   calling Py_Finalize...\n");
		Py_Finalize();
		log2("PY: End   python main interpreter ended\n");
		return 0;
	} 
	PyEval_AcquireLock();
	Py_Finalize();
	return 0;
}

int w_ReserveID()
{
	return GetFreeID();
}

int w_Load(w_Targs* args)
{
	const char* scriptname = "?";
	const char* botname = "VH";
	const char* opchatname = "OPchat";
	const char* basedir = ".";
	long starttime = 0;
	long id = 0;
	if(!w_Python->state || !w_unpack( args, "lssssl", &id, &scriptname, &botname, &opchatname, &basedir, &starttime)) return -1;
	if (id != GetFreeID()) { log2("PY: cannot start a new python interpreter with ID %ld\n", id); return -1; }
	w_TScript * script = (w_TScript*) calloc (1, sizeof(w_TScript));
	w_Scripts[id] = script;
	script->id = id;
	script->callbacks = w_Python->callbacks; 
	script->botname = botname;
	script->opchatname = opchatname;
	script->path = strdup(scriptname);
	script->name = GetName(script->path);
	const char *name = script->name;
	
	log2("PY: [%ld:%s] starting new python interpreter for %s\n", id, name, scriptname);
	
	if (log_level > 2)
	{
		printf("PY: [%ld:%s] available callbacks: ", id, name);
		int i;
		for (i=0; i< W_MAX_CALLBACKS; i++)
			( script->callbacks[i] ) ? printf("%d", i % 10) : printf(".");
		printf("\n"); fflush(stdout);
	}
	
	PyEval_AcquireLock();
	script->state = Py_NewInterpreter();
	
	if (script->state == NULL) {
		log("PY: [%ld:%s] error: Can't create interpreter state\n", id, name);
		PyEval_ReleaseLock();
		return w_Unload(id);
	}
	PyEval_ReleaseThread(script->state);
	
	char *argv[] = {(char*)"<vh>", 0};
	FILE * fp;
	PyObject *m, *o, *pFunc, *module;
	
	PyEval_AcquireThread(script->state);
		
	PySys_SetArgv(1, argv);
	//PySys_SetObject("__plugin__", (PyObject *) plugin); // TODO: maybe add some useful data to that later

	m = Py_InitModule("vh", w_vh_methods);
	if (m == NULL) {
		log("PY: [%ld:%s] error: Can't create vh module\n", id, name);
		PyErr_Print();
		PyEval_ReleaseThread(script->state); return w_Unload(id);
	}
	
	PyModule_AddIntConstant(m, "myid", (long)id);
	PyModule_AddStringConstant(m, "botname", (char *)script->botname);
	PyModule_AddStringConstant(m, "opchatname", (char *)script->opchatname);
	PyModule_AddStringConstant(m, "name", (char *)script->name);
	PyModule_AddStringConstant(m, "path", (char *)script->path);
	PyModule_AddStringConstant(m, "basedir", (char *)basedir);
	PyModule_AddIntConstant(m, "starttime", starttime);
	
	

	o = Py_BuildValue("(ii)", 1, 0);
	PyObject_SetAttrString(m, "__version__", o);

	fp = fopen(scriptname, "r");
	if (fp == NULL) {
		log("PY: [%ld:%s] error: Can't open file %s :::: %s\n", id, name, scriptname, strerror(errno));
		PyEval_ReleaseThread(script->state); return w_Unload(id);
	}

	//printf("PY: wrapper: attempting to run the script...\n"); fflush(stdout);
	if (PyRun_SimpleFile(fp, scriptname) != 0) {
		log("PY: [%ld:%s] error: Error loading module: %s\n", id, name, scriptname);
		PyErr_Print();
		fclose(fp);
		PyEval_ReleaseThread(script->state); return w_Unload(id);
	}
	fclose(fp);

	module = PyDict_GetItemString(PyImport_GetModuleDict(), "__main__");
	if (module == NULL) {
		log("PY: [%ld:%s] error: Can't get __main__ module\n", id, name);
		PyEval_ReleaseThread(script->state); return w_Unload(id);
	}
	
	
	char* hooks;
	hooks = (char*) calloc(W_MAX_HOOKS, sizeof(char));
	for (int i=0; i<W_MAX_HOOKS; i++)
	{
		pFunc = w_GetHook(i);
		if (!pFunc) continue;
		hooks[i] = 1;
		Py_DECREF(pFunc);
	}
	script->hooks = hooks;

	if (log_level > 2)
	{
		printf("PY: [%ld:%s] available hooks:     ", id, name);
		for (int i=0; i< W_MAX_HOOKS; i++)
			( script->hooks[i] ) ? printf("%d", i % 10) : printf(".");
		printf("\n"); fflush(stdout);
	}

	PyEval_ReleaseThread(script->state);

	return id;
}
	
	
int w_Unload(int id)
{
	if ((id < 0) || ((unsigned int)id >= w_Scripts.size()) || !w_Scripts[id]) { log("PY: Unload   error: No script with id: %d\n", id); return -1; }
	w_TScript *script = w_Scripts[id];
	PyThreadState * state = script->state;
	if(state) 
	{
		PyEval_AcquireThread(state);
		
		PyObject *module, *pFunc;
		pFunc = NULL;
		
		module = PyDict_GetItemString(PyImport_GetModuleDict(), "__main__");
		if (module == NULL) 
		{ log("PY: [%d:%s] Unload   error: Can't get __main__ module\n", id, script->name); }
		if (module) 
			if (PyObject_HasAttrString(module, "UnLoad"))
			{
				pFunc = PyObject_GetAttrString(module, "UnLoad"); 
				if (pFunc && PyCallable_Check(pFunc))
				{
					PyObject *pArgs, *pValue;
					pArgs = PyTuple_New(0);
					pValue = PyObject_CallObject( pFunc, pArgs );
					Py_XDECREF(pValue);
					Py_DECREF(pArgs);
					log2("PY: [%d:%s] Unload   calling UnLoad script function\n", id, script->name);
				}
				Py_XDECREF(pFunc);
			}
	
		Py_EndInterpreter(state);
		log2("PY: [%d:%s] interpreter ended\n", id, script->name);
		PyEval_ReleaseLock();
	}
	else { log2("PY: [%d:%s] Unload   no thread state found\n", id, script->name); }
	if (script->hooks) free(script->hooks);
	//if (script->name) free(script->name);
	// we don't free callbacks because they point to the global w_Python->callbacks
	w_Scripts[id] = NULL;
	free(script);
	return -1;
}	
	
int w_HasHook(int id, int hook)
{
	if ((id < 0) || ((unsigned int)id >= w_Scripts.size()) || !w_Scripts[id]) { log("PY: HasHook error: No script with id: %d\n", id); return 0; }
	w_TScript *script = w_Scripts[id];
	if (!script || hook < 0 || hook >= W_MAX_HOOKS) return 0;
	if (hook == W_OnOperatorCommand) return 1;
	if (script->hooks[hook]) return 1;
	return 0;
}


PyObject * w_GetHook(int hook)
{
	const char * s = w_HookName(hook);
	if (!s) return NULL;
	PyObject *m, *f;
	m = PyDict_GetItemString(PyImport_GetModuleDict(), "__main__");  // m is a borrowed reference, do not decref!
	if (!m) { log("PY: error: Can't get __main__ module\n"); return NULL; }
	if (!PyObject_HasAttrString(m, (char*)s)) return NULL;
	f = PyObject_GetAttrString(m, (char*)s);
	if (!f || !PyCallable_Check(f))
	{
		Py_XDECREF(f);
		return NULL;
	}
	return f;
}


w_Targs* w_CallHook (int id , int func, w_Targs* params)   // return > 0 means further processing by another plugins or the hub
{
	if ((id < 0) || ((unsigned int)id >= w_Scripts.size()) || !w_Scripts[id]) { log2("PY: CallHook error: No script with id: %d\n", id); return NULL; }
	w_TScript *script = w_Scripts[id];
	const char *name = script->name;
	w_Targs *res = NULL;
	if (!script->state) return NULL;
	if (func > W_MAX_HOOKS) return NULL;
	if (!params) return NULL;

	PyEval_AcquireThread(script->state);

	PyObject * args = NULL;
	PyObject * pFunc = NULL;

	pFunc = w_GetHook(func);
	if (!pFunc) { PyEval_ReleaseThread(script->state); return NULL; }

	char *s0 = NULL;
	char *s1 = NULL;
	char *s2 = NULL;
	char *s3 = NULL;
	char *s4 = NULL;
	char *s5 = NULL;
	long l0 = 0;
	long l1 = 0;

	switch (func) {
		/*
		case W_OnTimer:
			if (! w_unpack( params, ""))
			{ log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params)); break; }
			args = Py_BuildValue("()");
			break;
		*/

		case W_OnTimer:
		case W_OnUnLoad:
			if (!w_unpack(params, "l", &l0)) {
				log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params));
				break;
			}

			args = Py_BuildValue("(l)", l0);
			break;

		case W_OnUserLogin:
		case W_OnUserLogout:
		case W_OnNewConn:
		case W_OnCloseConn:
		case W_OnParsedMsgValidateNick:
			if (! w_unpack( params, "s", &s0))
			{ log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params)); break; }
			args = Py_BuildValue("(z)", s0);
			break;
		case W_OnOperatorCommand:
		case W_OnOperatorDrops:
		case W_OnUserCommand:
		case W_OnValidateTag:
		case W_OnParsedMsgChat:
		case W_OnParsedMsgSupport:
		case W_OnParsedMsgBotINFO:
		case W_OnParsedMsgVersion:
		case W_OnParsedMsgMyPass:
		case W_OnParsedMsgRevConnectToMe:
		case W_OnParsedMsgSearch:
		case W_OnParsedMsgSR:
		case W_OnParsedMsgAny:
		case W_OnParsedMsgAnyEx:
		case W_OnOpChatMessage:
		case W_OnUnknownMsg:
			if (! w_unpack( params, "ss", &s0, &s1))
			{ log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params)); break; }
			args = Py_BuildValue("(zz)", s0, s1);
			break;
		case W_OnOperatorKicks:
		case W_OnParsedMsgPM:
		case W_OnParsedMsgMCTo:
			if (! w_unpack( params, "sss", &s0, &s1, &s2))
			{ log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params)); break; }
			args = Py_BuildValue("(zzz)", s0, s1, s2);
			break;
		case W_OnNewReg:
			if (!w_unpack(params, "ssl", &s0, &s1, &l0)) {
				log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params));
				break;
			}

			args = Py_BuildValue("(zzl)", s0, s1, l0);
			break;
		case W_OnNewBan:
		case W_OnParsedMsgConnectToMe:
			if (! w_unpack( params, "ssss", &s0, &s1, &s2, &s3))
			{ log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params)); break; }
			args = Py_BuildValue("(zzzz)", s0, s1, s2, s3);
			break;
		case W_OnCtmToHub:
			if (!w_unpack(params, "sslls", &s0, &s1, &l0, &l1, &s2)) {
				log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params));
				break;
			}

			args = Py_BuildValue("(zzllz)", s0, s1, l0, l1, s2);
			break;
		case W_OnParsedMsgMyINFO:
		case W_OnFirstMyINFO:
			if (! w_unpack( params, "ssssss", &s0, &s1, &s2, &s3, &s4, &s5))
			{ log1("PY: [%d:%s] CallHook %s: unexpected parameters %s\n", id, name, w_HookName(func), w_packprint(params)); break; }
			args = Py_BuildValue("(zzzzzz)", s0, s1, s2, s3, s4, s5);
			break;
		default:
			break;
	}
	
	if (!args) { Py_DECREF(pFunc); log1("PY: [%d:%s] CallHook %s: no arguments object could be created\n", id, name, w_HookName(func)); PyEval_ReleaseThread(script->state); return NULL; }
	
	PyObject * pValue = PyObject_CallObject(pFunc, args);
	Py_DECREF(args);
	Py_DECREF(pFunc);
	if (pValue != NULL) 
	{
		switch(func)
		{
			//case W_OnParsedMsgValidateNick:
			//case W_OnOperatorCommand:
			//case W_OnUserCommand:
			case W_OnParsedMsgChat:
				if (PyString_Check(pValue))  // a replacement message
				{
					char *msg = PyString_AsString(pValue);
					if (msg)
					{
						log2("PY: [%d:%s] CallHook OnParsedMsgChat: returned %s\n", id, name, msg);
						res = w_pack("ss", (char*)NULL, msg);
						break;
					}
				}
				if (PyTuple_Check(pValue))
					if (PyTuple_Size(pValue) == 2)   // replacement for both nick and message (NULL values mean no change)
					{
						char *nick = NULL;
						char *msg = NULL;
						if (PyArg_ParseTuple(pValue, "zz:OnParsedMsgChat", &nick, &msg))
						{
							res = w_pack("ss", nick, msg);
							log2("PY: [%d:%s] CallHook OnParsedMsgChat: returned ( %s, %s )\n", id, name, nick, msg);
							break;
						}
						else PyErr_Print();
						break;
					}
			//case W_OnParsedMsgSearch:
			//case W_OnParsedMsgSR:
			case W_OnParsedMsgMyINFO:
				if (PyTuple_Check(pValue))
					if (PyTuple_Size(pValue) == 5)   //  (desc, tag, speed, email, sharesize)
					{
						char *desc = NULL;
						char *tag = NULL;
						char *speed = NULL;
						char *email = NULL;
						char *share = NULL;
						if (PyArg_ParseTuple(pValue, "zzzzz:OnParsedMsgMyINFO", &desc, &tag, &speed, &email, &share))
						{
							res = w_pack("sssss", desc, tag, speed, email, share);
							log2("PY: [%d:%s] CallHook OnParsedMsgMyINFO: returned ( %s, %s, %s, %s, %s )\n", id, name, desc, tag, speed, email, share);
							break;
						}
						else PyErr_Print();
						break;
					}
			case W_OnFirstMyINFO:
				if (PyTuple_Check(pValue))
					if (PyTuple_Size(pValue) == 5)   //  (desc, tag, speed, email, sharesize)
					{
						char *desc = NULL;
						char *tag = NULL;
						char *speed = NULL;
						char *email = NULL;
						char *share = NULL;
						if (PyArg_ParseTuple(pValue, "zzzzz:OnFirstMyINFO", &desc, &tag, &speed, &email, &share))
						{
							res = w_pack("sssss", desc, tag, speed, email, share);
							log2("PY: [%d:%s] CallHook OnFirstMyINFO: returned ( %s, %s, %s, %s, %s )\n", id, name, desc, tag, speed, email, share);
							break;
						}
						else PyErr_Print();
						break;
					}
			//case W_OnParsedMsgAny:
			//case W_OnParsedMsgAnyEx:
			//case W_OnOpChatMessage:
			//case W_OnUnLoad:
			//case W_OnCtmToHub:
			//case W_OnNewReg:
			//case W_OnUnknownMsg:
			//case W_OnOperatorKicks:
			//case W_OnParsedMsgPM:
			//case W_OnParsedMsgMCTo:
			//case W_OnParsedMsgConnectToMe:
			default:
				if (pValue == Py_None) res = w_pack("l", (long)1);
				else if (PyInt_Check(pValue)) res = w_pack("l", (long)PyInt_AS_LONG(pValue));
				else 
				{ 
					if (log_level > 0)
					{
						printf("PY: [%d:%s] CallHook %s: unexpected return value: ", id, name, w_HookName(func));
						PyObject_Print(pValue, stdout, 0);
						printf("\n"); fflush(stdout);
					}
					res = w_pack("l", (long)1);
				}
				break;
		}
		Py_DECREF(pValue);	
	}
	else { log("PY: [%d:%s] Call (%s): failed\n", id, name, w_HookName(func)); PyErr_Print(); fflush(stdout); }
	if (!res) res = w_pack("l", (long)1);
	
	PyEval_ReleaseThread(script->state);
	return res;
}


const char * w_HookName(int hook)
{
	switch (hook) {
		case W_OnNewConn: 			return "OnNewConn";
		case W_OnCloseConn: 		return "OnCloseConn";
		case W_OnParsedMsgChat: 	return "OnParsedMsgChat";
		case W_OnParsedMsgPM: 		return "OnParsedMsgPM";
		case W_OnParsedMsgMCTo: 	return "OnParsedMsgMCTo";
		case W_OnParsedMsgSearch: 	return "OnParsedMsgSearch";
		case W_OnParsedMsgSR: 		return "OnParsedMsgSR";
		case W_OnParsedMsgMyINFO: 	return "OnParsedMsgMyINFO";
		case W_OnFirstMyINFO: 		return "OnFirstMyINFO";
		case W_OnParsedMsgValidateNick: return "OnParsedMsgValidateNick";
		case W_OnParsedMsgAny: 		return "OnParsedMsgAny";
		case W_OnParsedMsgAnyEx:	return "OnParsedMsgAnyEx";
		case W_OnOpChatMessage: return "OnOpChatMessage";
		case W_OnUnLoad: return "OnUnLoad";
		case W_OnCtmToHub: return "OnCtmToHub";
		case W_OnParsedMsgSupport: 	return "OnParsedMsgSupport";
		case W_OnParsedMsgBotINFO: 	return "OnParsedMsgBotINFO";
		case W_OnParsedMsgVersion: 	return "OnParsedMsgVersion";
		case W_OnParsedMsgMyPass: 	return "OnParsedMsgMyPass";
		case W_OnParsedMsgConnectToMe: 	return "OnParsedMsgConnectToMe";
		case W_OnParsedMsgRevConnectToMe: return "OnParsedMsgRevConnectToMe";
		case W_OnUnknownMsg: 		return "OnUnknownMsg";
		case W_OnOperatorCommand: 	return "OnOperatorCommand";
		case W_OnOperatorKicks: 	return "OnOperatorKicks";
		case W_OnOperatorDrops: 	return "OnOperatorDrops";
		case W_OnValidateTag: 		return "OnValidateTag";
		case W_OnUserCommand: 		return "OnUserCommand";
		case W_OnUserLogin: 		return "OnUserLogin";
		case W_OnUserLogout: 		return "OnUserLogout";
		case W_OnTimer: return "OnTimer";
		case W_OnNewReg: return "OnNewReg";
		case W_OnNewBan: 		return "OnNewBan";
		default:			return NULL;
	}	
}

const char * w_CallName(int callback)
{
	switch (callback) {
		case W_SendToOpChat: return "SendToOpChat";
		case W_SendToActive: return "SendToActive";
		case W_SendToPassive: return "SendToPassive";
		case W_SendToActiveClass: return "SendToActiveClass";
		case W_SendToPassiveClass: return "SendToPassiveClass";
		case W_SendDataToUser: 		return "SendDataToUser";
		case W_SendDataToAll: 		return "SendDataToAll";
		case W_SendPMToAll: 		return "SendPMToAll";
		case W_CloseConnection: 	return "CloseConnection";
		case W_GetMyINFO: 		return "GetMyINFO";
		case W_SetMyINFO: 		return "SetMyINFO";
		case W_GetUserClass: 		return "GetUserClass";
		case W_GetUserHost: 		return "GetUserHost";
		case W_GetUserIP: 		return "GetUserIP";
		case W_GetUserCC: 		return "GetUserCC";
		case W_GetIPCC: 		return "GetIPCC";
		case W_GetIPCN: 		return "GetIPCN";
		case W_GetNickList: 		return "GetNickList";
		case W_GetOpList: 		return "GetOpList";
		case W_Ban: 			return "Ban";
		case W_KickUser: 		return "KickUser";
		case W_ParseCommand: 		return "ParseCommand";
		case W_SetConfig: 		return "SetConfig";
		case W_GetConfig: 		return "GetConfig";
		case W_AddRobot: 		return "AddRobot";
		case W_DelRobot: 		return "DelRobot";
		case W_SQL: 			return "SQL";
		case W_SQLQuery: 		return "SQLQuery";
		case W_SQLFetch: 		return "SQLFetch";
		case W_SQLFree: 		return "SQLFree";
		case W_GetUsersCount: 		return "GetUsersCount";
		case W_GetTotalShareSize: 	return "GetTotalShareSize";
		case W_UserRestrictions: 	return "UserRestrictions";
		case W_Topic: 			return "Topic";
		case W_mc: 			return "mc";
		case W_classmc: 		return "classmc";
		case W_usermc: 			return "usermc";
		case W_pm: 			return "pm";
		default:			return NULL;
	}	
}
