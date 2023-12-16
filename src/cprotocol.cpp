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

#include "cprotocol.h"
#include "stringutils.h"

namespace nVerliHub {
	using namespace nUtils;
	using namespace nEnums;

	namespace nProtocol {

cProtocol::cProtocol():
	cObj("cProtocol")
{}

cProtocol::~cProtocol()
{}

cMessageParser::cMessageParser(int MaxChunks):
	cObj("cMessageParser"),
	mChunks(MaxChunks),
	mChStrings(NULL),
	mChStrMap(0l),
	//Overfill(false),
	//Received(false),
	mError(false),
	mModified(false),
	mType(eMSG_UNPARSED),
	mLen(0),
	mKWSize(0),
	mMaxChunks(MaxChunks)
{
	mChStrings = new std::string[2 * mMaxChunks];
}

cMessageParser::~cMessageParser()
{
	mChunks.clear();

	if (mChStrings)
		delete[] mChStrings;

	mChStrings = NULL;
}

// reinitialize the structure
void cMessageParser::ReInit()
{
	mChunks.clear();
	mChunks.resize(mMaxChunks);
	mLen = 0;
	mChStrMap = 0l;
	//Overfill = false;
	//Received = false;
	mError = false;
	mModified = false;
	mStr.resize(0);
	//mStr.reserve(512); // todo: do we really need this big reserve? some commands are alot smaller
	mType = eMSG_UNPARSED;
	mKWSize = 0;
}

// apply the chunkstring into the main string
void cMessageParser::ApplyChunk(unsigned int n)
{
	if (!n)
		return;

	if (n >= mChunks.size())
		return;

	unsigned long flag = 1 << n;

	if (mChStrMap & flag) {
		tChunk &chu = mChunks[n];
		mStr.replace(chu.first, chu.second, mChStrings[n]);
		mModified = true;
	}
}

// return the n'th chunk, as splited by SplitChunks function
string &cMessageParser::ChunkString(unsigned int n)
{
	if (!n) // the zeroth string is always the complete one, and its pointer is reserved for the empty string
		return mStr;

	if (n >= mChunks.size()) // this should never happen, but if it happens, we are prepared
		return mChStrings[0];

	unsigned long flag = 1 << n;

	if (!(mChStrMap & flag)) {
		mChStrMap |= flag;
		tChunk &chu = mChunks[n];

		if ((chu.first >= 0) && (chu.second >= 0) && ((unsigned int)chu.first <= mStr.length()) && ((unsigned int)chu.second <= mStr.length())) { // chunk can be empty
			mChStrings[n].assign(mStr, chu.first, chu.second);
		} else if (ErrLog(1)) {
			LogStream() << "Error in parsing message, chunk " << n << ": " << mStr << endl;
		}

		ShrinkStringToFit(mChStrings[n]);
	}

	return mChStrings[n];
}

// splits message into two chunks by a delimiter and stores them in the chunklist
bool cMessageParser::SplitOnTwo(size_t start, const string &lim, int cn1, int cn2, size_t len, bool left)
{
	if (!len)
		len = mLen;

	unsigned long i;

	if (left) {
		i = mStr.find(lim, start);

		if ((i == mStr.npos) || ((i - start) >= len))
			return false;
	} else {
		i = mStr.rfind(lim, (start + len) - lim.length());

		if ((i == mStr.npos) || (i < start))
			return false;
	}

	SetChunk(cn1, start, i - start);
	SetChunk(cn2, i + lim.length(), mLen - i - lim.length());
	return true;
}

// splits message into two chunks by a delimiter and stores them in the chunklist
bool cMessageParser::SplitOnTwo(size_t start, const char lim, int cn1, int cn2, size_t len, bool left)
{
	if (!len)
		len = mLen;

	unsigned long i;

	if (left) {
		i = mStr.find_first_of(lim, start);

		if ((i == mStr.npos) || ((i - start) >= len))
			return false;
	} else {
		i = mStr.find_last_of(lim, (start + len) - 1);

		if ((i == mStr.npos) || (i < start))
			return false;
	}

	SetChunk(cn1, start, i - start);
	SetChunk(cn2, i + 1, mLen - i - 1);
	return true;
}

// splits the chunk number ch into two chunks by a delimiter and stores them in the chunklist under numbers cn1 and cn2
bool cMessageParser::SplitOnTwo(const char lim, int ch, int cn1, int cn2, bool left)
{
	tChunk &chu = mChunks[ch];
	return SplitOnTwo(chu.first, lim, cn1, cn2, chu.second, left);
}

// splits the chunk number ch into two chunks by a delimiter and stores them in the chunklist under numbers cn1 and cn2
bool cMessageParser::SplitOnTwo(const string &lim, int ch, int cn1, int cn2, bool left)
{
	tChunk &chu = mChunks[ch];
	return SplitOnTwo(chu.first, lim, cn1, cn2, chu.second, left);
}

// reduce the chunk from left by amount, cn is the chunk number
bool cMessageParser::ChunkRedLeft(int cn, int amount)
{
	tChunk &ch = mChunks[cn];

	if ((unsigned int)(ch.first + amount) < mLen) {
		ch.first += amount;
		ch.second -= amount;
		return true;
	} else {
		return false;
	}
}

// reduce the chunk from right by amount, cn is the chunk number
bool cMessageParser::ChunkRedRight(int cn, int amount)
{
	mChunks[cn].second -= amount;
	return true;
}

// increase the chunk from left by amount, cn is the chunk number
bool cMessageParser::ChunkIncLeft(int cn, int amount)
{
	mChunks[cn].second += amount;
	return true;
}

// get the string
string &cMessageParser::GetStr()
{
	return mStr;
}

// fill in a given chunk
void cMessageParser::SetChunk(int n, int start, int len)
{
	tChunk &ch = mChunks[n];
	ch.first = start;
	ch.second = len;
}

	}; // namespace nProtocol
}; // namespace nVerliHub
