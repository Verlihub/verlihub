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
	[ --- lib hdr ] PCRE
	[ --- lib hdr ] GetText
	[ --- lib hdr ] MaxMindDB
	[ ??? ??? ??? ] LibIntl
	[ ??? ??? ??? ] Crypt
	[ ??? ??? ??? ] Threads
	[ ??? ??? ??? ] DL

* Optional dependencies:

	[ --- lib hdr ] Lua			5.2
	[ bin --- --- ] Dialog

* Install dependencies on Debian:

sudo apt install libpcre3-dev libssl-dev mysql-server mysql-client libmysqlclient-dev libmaxminddb-dev libicu-dev gettext libasprintf-dev g++ make cmake

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

pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-libmariadbclient mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-icu mingw-w64-ucrt-x86_64-zlib mingw-w64-ucrt-x86_64-pcre mingw-w64-ucrt-x86_64-gettext-runtime mingw-w64-ucrt-x86_64-gettext-tools mingw-w64-ucrt-x86_64-dlfcn mingw-w64-ucrt-x86_64-libmaxminddb

 * Optional dependencies

 pacman -S --needed mingw-w64-ucrt-x86_64-lua

* Install database server on Windows:

MSYS2 has no MariaDB server, only client library, install MariaDB server for Windows from: https://mariadb.org/download
During installation set root password and keep "Install as service" enabled, so server starts with Windows.
Then open "MySQL Client" from MariaDB start menu folder, log in with root password and create database and user for Verlihub:

CREATE DATABASE verlihub;
CREATE USER 'verlihub'@'localhost' IDENTIFIED BY 'your_password';
GRANT ALL ON verlihub.* TO 'verlihub'@'localhost';
FLUSH PRIVILEGES;

 * Notes

 Hub setup script "vh --install" is not available on Windows yet, so configuration folder and its dbconfig file are not created automatically.

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

-DUSE_EPOLL=OFF (default: ON)
	use poll instead of epoll connection chooser, on Windows select instead of bundled wepoll

===========
End of file
===========
