#	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
#	Copyright (C) 2006-2015 Verlihub Team, info at verlihub dot net
#
#	Verlihub is free software; You can redistribute it
#	and modify it under the terms of the GNU General
#	Public License as published by the Free Software
#	Foundation, either version 3 of the license, or at
#	your option any later version.
#
#	Verlihub is distributed in the hope that it will be
#	useful, but without any warranty, without even the
#	implied warranty of merchantability or fitness for
#	a particular purpose. See the GNU General Public
#	License for more details.
#
#	Please see http://www.gnu.org/licenses/ for a copy
#	of the GNU General Public License.

# Plugman plugin

SET(ENABLE_PLUGIN TRUE)
ADD_PLUGIN(ENABLE_PLUGIN plugman)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "plugman" "Pluging Manager support")

# Lua plugin

Find_Package(Lua52) # Lua 5.2
#Find_Package(LuaSocket)

IF(LUA52_FOUND)
	ADD_PLUGIN(LUA52_FOUND lua)
	MACRO_LOG_FEATURE(LUA52_FOUND "lua" "Allows you to use scripts written in Lua (http://www.lua.org)" "http://www.lua.org" "5.2" "Lua libray NOT found")
ELSE()
	Find_Package(Lua51) # Lua 5.1
	#Find_Package(LuaSocket)
	ADD_PLUGIN(LUA51_FOUND lua)
	MACRO_LOG_FEATURE(LUA51_FOUND "lua" "Allows you to use scripts written in Lua (http://www.lua.org)" "http://www.lua.org" "5.1" "Lua libray NOT found")
ENDIF()

# Python plugin

SET(Python_ADDITIONAL_VERSIONS 2.4 2.5 2.6 2.7)
Find_Package(PythonLibs)
ADD_PLUGIN(PYTHONLIBS_FOUND python)
MACRO_LOG_FEATURE(PYTHONLIBS_FOUND "python" "Allows you to use scripts written with Python (http://www.python.org)" "http://www.python.org" "2.7" "Python libray NOT found")

# Perl plugin

Find_Package(PerlLibs)
# Workaround for Debian: libperl-dev required
Find_Library(PERLLIBS_SO perl)
IF(PERLLIBS_FOUND AND PERLLIBS_SO)
	SET(PERLLIBS_FOUND TRUE)
ELSE()
	SET(PERLLIBS_FOUND FALSE)
ENDIF()
ADD_PLUGIN(PERLLIBS_FOUND perl)
MACRO_LOG_FEATURE(PERLLIBS_FOUND "perl" "Allows you to use scripts written with Perl" "http://www.perl.org" "5.8" "Perl library NOT found")

# Other plugins

ADD_PLUGIN(ENABLE_PLUGIN forbid)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "forbid" "Allows you to filter main chat and private messages with forbidden words")

ADD_PLUGIN(ENABLE_PLUGIN chatroom)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "chatroom" "Set up individual chatrooms for users")

ADD_PLUGIN(ENABLE_PLUGIN iplog)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "iplog" "Logs IP addresses as users join and leave the hub")

ADD_PLUGIN(ENABLE_PLUGIN isp)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "isp" "Allows you to check connection types, nickname prefixes, and minimum shares, among other things")

ADD_PLUGIN(ENABLE_PLUGIN messenger)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "messenger" "Send messages to offline users")

ADD_PLUGIN(ENABLE_PLUGIN stats)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "stats" "Periodically saves statistics in the database (i.e. users, share, upload, searching, etc...)")

ADD_PLUGIN(ENABLE_PLUGIN replacer)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "replacer" "Replace given patterns in text messages")

ADD_PLUGIN(ENABLE_PLUGIN floodprot)
MACRO_LOG_FEATURE(ENABLE_PLUGIN "floodprot" "Offer more control from users flooding the hub")
