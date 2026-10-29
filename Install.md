=============================
Dependencies and installation
=============================

* Required dependencies:

	[ bin lib hdr ] GCC			4.8.0
	[ bin --- hdr ] CMake		3.16
	[ bin lib hdr ] MySQL		5.7.1
	[ --- lib hdr ] OpenSSL		1.1.0
	[ --- lib hdr ] LibICU		55.0
	[ bin --- --- ] Make
	[ --- lib hdr ] ZLib
	[ --- lib hdr ] PCRE2
	[ --- lib hdr ] GetText
	[ --- lib hdr ] MaxMindDB
	[ ??? ??? ??? ] LibIntl
	[ ??? ??? ??? ] Crypt
	[ ??? ??? ??? ] Threads
	[ ??? ??? ??? ] DL

* Optional dependencies:

	[ --- lib hdr ] Lua			5.2

* Install dependencies on Debian:

sudo apt install libpcre2-dev libssl-dev mysql-server mysql-client libmysqlclient-dev libmaxminddb-dev libicu-dev gettext libasprintf-dev g++ make cmake

 * Optional dependencies

 sudo apt install liblua5.2-dev

* Install dependencies on CentOS:

[todo]

* Install dependencies on Gentoo:

[todo]

* Install dependencies on RedHat:

[todo]

* Install dependencies on ArchLinux:

[todo]

* Install dependencies on Windows:

Note: Windows support is work in progress, it is tested with MinGW-w64 GCC from MSYS2 using UCRT64 environment: https://www.msys2.org
MINGW64 environment of MSYS2 is deprecated, other MinGW-w64 toolchains should work too, but they are not tested yet.

Run in "MSYS2 UCRT64" shell:

pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-libmariadbclient mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-icu mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-pcre2 mingw-w64-ucrt-x86_64-gettext-runtime mingw-w64-ucrt-x86_64-gettext-tools mingw-w64-ucrt-x86_64-dlfcn mingw-w64-ucrt-x86_64-libmaxminddb

 * Optional dependencies

 pacman -S --needed mingw-w64-ucrt-x86_64-lua

* Install database server on Windows:

MSYS2 has no MariaDB server, only client library, install MariaDB server for Windows from: https://mariadb.org/download
During installation set root password and keep "Install as service" enabled, so server starts with Windows.
Database and user for Verlihub are created by "verlihub --install" using root password, see "Install new hub" below.

* Compile and install Verlihub itself:

cd verlihub
mkdir build
cd build
cmake ..
make
make install (as super user)
ldconfig (as super user, this is required only when you get: cannot open shared object file, no such file or directory)
updatedb (as super user, people say this command is needed to update slocate locations)

* Compile and install Verlihub on Windows:

Run in "MSYS2 UCRT64" shell:

cd verlihub
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="C:/verlihub" ..
mingw32-make
mingw32-make install

 * Notes

 Installation prefix must be full path, it is built into Verlihub to find its files at runtime, relative path will not work.
 Use forward slashes, avoid spaces and "Program Files" which also requires administrator rights.
 Default installation prefix of CMake on Windows is not suitable, always set it.
 Verlihub needs MSYS2 libraries at runtime, they are found automatically in "MSYS2 UCRT64" shell.
 To run it outside of that shell, add C:/msys64/ucrt64/bin to PATH environment variable.

* Install new hub:

verlihub --install

 * Notes

 Installation asks for database information and creates database account and database when they don't exist, using administrator credentials of database server.
 Then it creates configuration folder with dbconfig file, default files, database tables, master account and basic hub settings.
 Run it as super user on Linux to choose system wide configuration folder like /etc/verlihub.
 Log of table creation is written to install_log file in configuration folder.
 In MSYS2 terminal passwords are shown while typing, in Windows console they are hidden.
 On MySQL server, when administrator credentials are asked, installation also allows database account to read memory usage of server for !sysinfo command, MariaDB server needs no extra permission.
 For existing database account on MySQL server run as database administrator, with your account name and host:

 GRANT SELECT ON performance_schema.memory_summary_global_by_event_name TO 'verlihub'@'localhost';

=======================
Supported CMake options
=======================

-DCMAKE_INSTALL_PREFIX=/usr (default: /usr/local)
	install prefix

-DLIB_INSTALL_DIR=<lib prefix> (default: lib)
	install lib to <prefix for install>/<lib prefix>

	example:
		-DLIB_INSTALL_DIR=lib64 - install lib to <prefix for install>/lib64

-DPLUGIN_INSTALL_DIR=<plugin prefix> (default: <lib prefix>)
	install plugins to <prefix for install>/<plugin prefix>

	example:
		-DPLUGIN_INSTALL_DIR=lib64/vh_plugins

-DUSE_TLS=OFF (default: ON)
	build without tls support

-DINSTALL_CMAKE_FILES=OFF (default: ON, on Windows OFF)
	dont install FindVerlihub.cmake into CMake root, it is the only file installed outside of install prefix

-DUSE_EPOLL=OFF (default: ON)
	use poll instead of epoll connection chooser, on Windows select instead of bundled wepoll

-DLOCALE_CONVERT=ON (default: OFF, on Alpine ON)
	convert translations to codepage of their language at build time, needed only when gettext of system can not convert them at runtime, like musl libc on Alpine

=======================
Translations
=======================

Translation files in po folder are UTF-8, Transifex reads templates from GitHub and sends translations back, see po/transifex.yml
Hub converts translations to charset of its locale at runtime, for example CP1251 with locale = ru_RU.CP1251 in dbconfig, or UTF-8 with ru_RU.UTF-8
Commands like verlihub --help and --install use language of system or user environment, for example LANG, on Windows console their output is UTF-8
Locale must contain charset after dot, when it is not installed on system, hub still translates messages using other installed locale like en_US.UTF-8, only character classification stays default
After changing texts in source, update templates with: make pot (on Windows: mingw32-make pot)

=======================================
Build plugins outside of Verlihub source
=======================================

Verlihub installs its headers, config.h and CMake package configuration, so plugin can be built in its own folder:

find_package(Verlihub 1.7 REQUIRED)
add_library(my_pi MODULE my_plugin.cpp)
target_link_libraries(my_pi Verlihub::verlihub)

Verlihub::verlihub gives library, headers and defines that Verlihub headers depend on, plugin must be built with same compiler.
When Verlihub is installed in own prefix, give it to CMake of plugin with: -DCMAKE_PREFIX_PATH=<install prefix>

===========
End of file
===========
