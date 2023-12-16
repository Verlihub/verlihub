/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2024 Verlihub Team, info at verlihub dot net

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

#ifndef CPROTOCOMMAND_H
#define CPROTOCOMMAND_H
#include <string>

using namespace std;

namespace nVerliHub {
	namespace nProtocol {

/*
	brief DC protocol command
	provides a Method Telling wheter a given command is this one or not
	description of a command from a protocol, about command, ans syntax, all parameters etc...
	@author Daniel Muller
*/

class cProtoCommand
{
	public:
		// constructor and destructor
		cProtoCommand();
		cProtoCommand(const string& cmd);
		virtual ~cProtoCommand();
		// test if str is of this command
		bool AreYou(const string &str) const;
	public:
		// the command keyword
		string mCmd;
		// the length of the base command string
		size_t mBaseLength;
};

	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
