/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2026 Verlihub Team, info at verlihub dot net

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

	Please see https://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#ifndef NSOCKETCPORTTEST_H
#define NSOCKETCPORTTEST_H

#include <string>

namespace nVerliHub {
	namespace nSocket {

/*
	$PortTest, client finds out if its ports can be reached from outside
	note: these block, they run on worker thread and only ever go to address of user who asked
*/

// connects to port and closes at once, nothing is sent, bind is source address and can be empty
bool PortTestTCP(const std::string &addr, unsigned int port, const std::string &bind, int msec);

// sends text to port, client tells by itself if it arrived
void PortTestUDP(const std::string &addr, unsigned int port, const std::string &bind, const std::string &text);

	}; // namespace nSocket
}; // namespace nVerliHub

#endif
