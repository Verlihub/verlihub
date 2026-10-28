===================
Third party notices
===================

Verlihub itself is licensed under GNU General Public License version 3 or later, see License.md.
This file lists code from other projects that is included in Verlihub, and libraries that Verlihub uses.

=============
Included code
=============

* wepoll - epoll for Windows

	Version: 1.5.8
	Website: https://github.com/piscisaureus/wepoll
	License: BSD 2-Clause, see src/wepoll/LICENSE
	Copyright: 2012-2020, Bert Belder
	Files: src/wepoll/wepoll.c, src/wepoll/wepoll.h
	Used for: epoll connection chooser on Windows, only built on Windows

* CMake - FindLua module

	Website: https://cmake.org
	License: BSD 3-Clause, see https://cmake.org/licensing
	Copyright: Kitware, Inc. and contributors
	Files: cmake/Modules/FindLua.cmake
	Used for: finding Lua library when building

==============
Used libraries
==============

These libraries are not included in Verlihub source, they are installed separately and linked.
When Verlihub is distributed in binary form together with these libraries, their licenses apply to them.

* OpenSSL 3.x: Apache License 2.0
* OpenSSL 1.1.x: OpenSSL and SSLeay license
* MariaDB Connector/C: GNU LGPL 2.1 or later
* MySQL client library: GNU GPL 2 with Universal FOSS Exception
* ICU: Unicode License
* ZLib: zlib License
* PCRE2: BSD 3-Clause
* GetText, LibIntl and LibAsprintf: GNU LGPL 2.1 or later
* MaxMindDB (libmaxminddb): Apache License 2.0
* Lua: MIT License
* DLfcn-Win32, on Windows: MIT License
* MinGW-w64 runtime and winpthreads, on Windows: permissive licenses, see COPYING files of mingw-w64
* GCC runtime libstdc++ and libgcc: GNU GPL 3 with GCC Runtime Library Exception

* MaxMind databases

	Verlihub does not include any MaxMind database, hub owner downloads it separately.
	GeoLite2 databases are licensed under Creative Commons Attribution-ShareAlike 4.0 and GeoLite2 End User License Agreement.
	This product can use GeoLite2 data created by MaxMind, available from https://www.maxmind.com

===========
End of file
===========
