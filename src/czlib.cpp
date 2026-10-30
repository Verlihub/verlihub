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

#include "czlib.h"
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

namespace nVerliHub {
	namespace nUtils {

cZLib::cZLib():
	inBufLen(ZLIB_BUFFER_SIZE),
	inLastLen(0),
	outBufLen(ZLIB_BUFFER_SIZE),
	outLastLen(0)
{
	inBuf = (char*)calloc(ZLIB_BUFFER_SIZE, 1);
	outBuf = (char*)calloc(ZLIB_BUFFER_SIZE, 1);

	if (outBuf)
		memcpy(outBuf, "$ZOn|", ZON_LEN);
}

cZLib::~cZLib()
{
	EndStream();

	if (outBuf)
		free(outBuf);

	if (inBuf)
		free(inBuf);
}

void cZLib::EndStream()
{
	if (!mStream)
		return;

	z_stream *strm = (z_stream*)mStream;
	deflateEnd(strm);
	delete strm;
	mStream = NULL;
}

char *cZLib::Compress(const char *buffer, size_t len, size_t &outLen, int &err, int level) // todo: do this in separate thread
{
	if (!buffer)
		return NULL;

	/*
		check if we are compressing same data as last time
		then return last compressed buffer to save resources
	*/

	if (inLastLen && outLastLen && (len == inLastLen) && inBuf && (memcmp(buffer, inBuf, len) == 0)) {
		outLen = outLastLen;

		if (len <= outLastLen) // fall back already here
			return NULL;
		else
			return outBuf;
	}

	if (inBufLen < len) // increase in buffer if not enough space
		for (; inBufLen < len; inBufLen += ZLIB_BUFFER_SIZE);

	char *new_buf = (char*)realloc(inBuf, inBufLen);

	if (!new_buf) {
		// note: failed realloc keeps old buffer, inBuf still points to it and next call uses it again, so it is not freed here
		outLen = inLastLen = outLastLen = 0;
		err = -inBufLen; // note: special message
		return NULL;
	}

	inBuf = new_buf;
	int comp_level = level;

	if (comp_level < Z_DEFAULT_COMPRESSION) // -1
		comp_level = Z_DEFAULT_COMPRESSION;
	else if (comp_level < Z_BEST_SPEED) // 1
		comp_level = Z_BEST_SPEED;
	else if (comp_level > Z_BEST_COMPRESSION) // 9
		comp_level = Z_BEST_COMPRESSION;

	z_stream *strm = (z_stream*)mStream;

	if (strm && (mStreamLevel != comp_level)) // note: level was changed, stream is made again
		EndStream();

	if (mStream) { // note: same stream for every flush, reset is much cheaper than setting it up again
		err = deflateReset(strm);

		if (err != Z_OK) {
			EndStream();
			outLen = inLastLen = outLastLen = 0;
			return NULL;
		}

	} else {
		strm = new z_stream;
		memset((void*)strm, '\0', sizeof(z_stream));
		strm->zalloc = Z_NULL; // allocate deflate state
		strm->zfree = Z_NULL;
		err = deflateInit(strm, comp_level);

		if (err != Z_OK) { // initialize
			delete strm;
			outLen = inLastLen = outLastLen = 0;
			return NULL;
		}

		mStream = strm;
		mStreamLevel = comp_level;
	}

	memcpy(inBuf, buffer, len); // copy data to in buffer

	if (outBufLen < len) { // increase out buffer if not enough space, note: compressed data bigger than input falls back to raw data anyway
		size_t want = outBufLen;

		for (; want < len; want += ZLIB_BUFFER_SIZE);

		char *new_out = (char*)realloc(outBuf, want); // note: buffer itself must grow, deflate writes up to its size, "$ZOn|" at start stays

		if (!new_out) { // note: stream stays for next time
			outLen = inLastLen = outLastLen = 0;
			err = -int(want); // note: special message
			return NULL;
		}

		outBuf = new_out;
		outBufLen = want;
	}

	strm->avail_in = (uInt)len;
	strm->next_in = (Bytef*)inBuf;
	strm->avail_out = (uInt)(outBufLen - ZON_LEN);
	strm->next_out = (Bytef*)(outBuf + ZON_LEN);

	if (deflate(strm, Z_FINISH) != Z_STREAM_END) { // compress, note: data that does not fit ends here too, reset next time makes stream usable again
		outLen = inLastLen = outLastLen = 0;
		return NULL;
	}

	outLen = outLastLen = (strm->total_out + ZON_LEN); // note: reset sets total to zero, so this is only this data
	inLastLen = len;

	if (len <= outLen) // out data size is bigger or equal in data size, fall back to raw data
		return NULL;

	return outBuf;
}

	}; // namespace nUtils
}; // namespace nVerliHub
