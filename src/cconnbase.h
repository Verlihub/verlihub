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

#ifndef NSERVERCCONNBASE_H
#define NSERVERCCONNBASE_H

#ifdef _WIN32
	#include <winsock2.h>
#endif

namespace nVerliHub {
	namespace nSocket {

#ifdef _WIN32
	typedef SOCKET tSocket; // note: windows socket is handle, not small number, so it can not be used as index
#else
	typedef int tSocket;
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
#endif

/** a Base class for cConnChoose Usage - for connections
	provides pure virtual conversion to a tSocket (uint or SOCKET) type
 */
class cConnBase
{
	public: virtual operator tSocket() const =0;
};

	}; // namespace nSocket

}; // namespace nVerliHub

#endif
