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

#ifdef HAVE_CONFIG_H
	#include <config.h>
#endif

#include "cprotocommand.h"
#include "stringutils.h"

namespace nVerliHub {
	using namespace nUtils;

	namespace nProtocol {

cProtoCommand::cProtoCommand()
{
	mBaseLength = 0;
}

cProtoCommand::cProtoCommand(const string& cmd):
	mCmd(cmd)
{
	mBaseLength = mCmd.length();
}

cProtoCommand::~cProtoCommand()
{}

/*
	test if str is of this command
*/

bool cProtoCommand::AreYou(const string &str) const
{
	if (str.size())
		return (0 == StrCompare(str, 0, mBaseLength, mCmd));

	return false;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
