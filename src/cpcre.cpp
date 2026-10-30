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

	Please see http://www.gnu.org/licenses/ for a copy
	of the GNU General Public License.
*/

#include "cpcre.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "stringutils.h"

using namespace std;
namespace nVerliHub {
	namespace nUtils {

static uint32_t PartCount(int size) // note: one third of offset array is not used for results, so number of parts is same as in pcre1 interface
{
	return ((size >= 3) ? uint32_t(size / 3) : 1);
}

cPCRE::cPCRE(int offsetResultSize):
	mPattern(NULL),
	mMatch(NULL),
	mOffsetResults(NULL),
	mOffsetResultSize(offsetResultSize)
{
	Clear();
}

cPCRE::~cPCRE()
{
	if (mPattern)
		pcre2_code_free(mPattern);

	mPattern = NULL;

	if (mMatch)
		pcre2_match_data_free(mMatch);

	mMatch = NULL;

	if (mOffsetResults)
		delete [] mOffsetResults;

	mOffsetResults = NULL;
}

cPCRE::cPCRE(const char *pattern, unsigned int options, int offsetResultSize):
	mPattern(NULL),
	mMatch(NULL),
	mOffsetResults(NULL),
	mOffsetResultSize(offsetResultSize)
{
	Clear();
	Compile(pattern, options);
}

cPCRE::cPCRE(const string &pattern, unsigned int options, int coord):
	mPattern(NULL),
	mMatch(NULL),
	mOffsetResults(NULL),
	mOffsetResultSize(coord)
{
	Clear();
	Compile(pattern.c_str(), options);
}

bool cPCRE::Compile(const char *pattern, unsigned int options)
{
	if (mPattern) { // note: compiling again replaces old pattern
		pcre2_code_free(mPattern);
		mPattern = NULL;
	}

	mResult = 0;
	int errcode = 0;
	PCRE2_SIZE erroffset = 0;
	mPattern = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, options, &errcode, &erroffset, NULL);
	return mPattern != NULL;
}

void cPCRE::Clear()
{
	if (mPattern)
		pcre2_code_free(mPattern);

	mPattern = NULL;
	mResult = 0;
	const uint32_t parts = PartCount(mOffsetResultSize);

	if (!mOffsetResults)
		mOffsetResults = new int[2 * parts];

	if (!mMatch)
		mMatch = pcre2_match_data_create(parts, NULL);
}

int cPCRE::Exec(const string &subject)
{
	if (!mPattern || !mMatch) {
		mResult = PCRE2_ERROR_NULL;
		return mResult;
	}

	mResult = pcre2_match(mPattern, (PCRE2_SPTR)subject.data(), subject.size(), 0, 0, mMatch, NULL); // note: return values are same as in pcre1 interface, number of parts, zero when there are more parts than results can hold, negative when not matched

	if (mResult >= 0) {
		const PCRE2_SIZE *vec = pcre2_get_ovector_pointer(mMatch);
		const uint32_t count = 2 * pcre2_get_ovector_count(mMatch);

		for (uint32_t i = 0; i < count; i++)
			mOffsetResults[i] = ((vec[i] == PCRE2_UNSET) ? -1 : int(vec[i])); // note: pcre2 marks part that is not set with own value, here it is -1
	}

	return mResult;
}

int cPCRE::Compare(int index, const string &text, const char *text2)
{
	if (!this->PartFound(index))
		return -1;

	int start = mOffsetResults[index << 1];
	return StrCompare(text, start, mOffsetResults[(index << 1) + 1] - start, text2);
}

int cPCRE::Compare(int index, const string &text, const string &text2)
{
	return Compare(index, text, text2.c_str());
}

int cPCRE::Compare(const string &name, const string &text, const string &text2)
{
	return Compare(this->GeStringNumber(name), text, text2.c_str());
}

void cPCRE::Extract(int index, const string &src, string &dst)
{
	if (!this->PartFound(index))
		return;

	int start = mOffsetResults[index << 1];
	dst.assign(src, start, mOffsetResults[(index << 1) + 1] - start);
}

bool cPCRE::PartFound(int index)
{
	if ((index < 0) || (index >= mResult))
		return false;

	return mOffsetResults[index << 1] >= 0;
}

void cPCRE::Replace(int index, string &subject, const string &replace)
{
	if (!this->PartFound(index))
		return;

	int start = mOffsetResults[index << 1];
	subject.replace(start, mOffsetResults[(index << 1) + 1] - start, replace);
}

int cPCRE::GeStringNumber(const string &substring)
{
	if (!mPattern)
		return PCRE2_ERROR_NULL;

	return pcre2_substring_number_from_name(this->mPattern, (PCRE2_SPTR)substring.c_str());
}

	}; // namespace nUtils
}; // namespace nVerliHub
