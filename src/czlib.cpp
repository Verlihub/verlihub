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
#include <string_view>
#include <functional>

namespace nVerliHub {
	namespace nUtils {

cZLib::cZLib():
	outBufLen(0)
{
	outBuf = (char*)calloc(ZLIB_BUFFER_SIZE, 1);

	if (outBuf) {
		memcpy(outBuf, "$ZOn|", ZON_LEN);
		outBufLen = ZLIB_BUFFER_SIZE;
	}
}

cZLib::~cZLib()
{
	EndStream();

	if (outBuf)
		free(outBuf);
}

size_t cZLib::GetInBufLen() const
{
	size_t size = 0;

	for (const sCache &ent: mCache)
		size += ent.mIn.capacity();

	return size;
}

size_t cZLib::GetOutBufLen() const
{
	size_t size = outBufLen;

	for (const sCache &ent: mCache)
		size += ent.mOut.capacity();

	return size;
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

int cZLib::Level(int level)
{
	if (level <= Z_DEFAULT_COMPRESSION) // -1
		return 6; // note: default level of zlib, its header has no constant for it

	if (level < Z_BEST_SPEED) // 1, note: 0 becomes 1
		return Z_BEST_SPEED;

	if (level > Z_BEST_COMPRESSION) // 9
		return Z_BEST_COMPRESSION;

	return level;
}

const char *cZLib::Compress(const char *buffer, size_t len, size_t &outLen, int &err, int level, bool &cached) // todo: do this in separate thread
{
	cached = false;
	outLen = 0;

	if (!buffer)
		return NULL;

	const int comp_level = Level(level);
	const bool keep = (len <= sCacheMax);
	size_t hash = 0;

	if (keep) { // note: same data was compressed recently, its result is used again
		hash = std::hash<std::string_view>()(std::string_view(buffer, len));

		for (sCache &ent: mCache) {
			if (ent.mUsed && (ent.mHash == hash) && (ent.mLevel == comp_level) && (ent.mIn.size() == len) && (memcmp(ent.mIn.data(), buffer, len) == 0)) {
				size_t pos = 0; // note: number of entries used after this one

				for (const sCache &cur: mCache) {
					if (cur.mUsed > ent.mUsed)
						pos++;
				}

				mCacheHits[pos]++;
				ent.mUsed = ++mCacheUse;
				cached = true;
				outLen = ent.mOutLen;

				if (len <= outLen) // note: compressed data is not smaller, fall back to raw data
					return NULL;

				return ent.mOut.data();
			}
		}
	}

	mCacheMisses++;
	z_stream *strm = (z_stream*)mStream;

	if (strm && (mStreamLevel != comp_level)) // note: level was changed, stream is made again
		EndStream();

	if (mStream) { // note: same stream for every flush, reset is much cheaper than setting it up again
		err = deflateReset(strm);

		if (err != Z_OK) {
			EndStream();
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
			return NULL;
		}

		mStream = strm;
		mStreamLevel = comp_level;
	}

	if (outBufLen < len) { // increase out buffer if not enough space, note: compressed data bigger than input falls back to raw data anyway
		size_t want = outBufLen;

		for (; want < len; want += ZLIB_BUFFER_SIZE);

		char *new_out = (char*)realloc(outBuf, want); // note: buffer itself must grow, deflate writes up to its size

		if (!new_out) { // note: stream stays for next time
			err = -int(want); // note: special message
			return NULL;
		}

		outBuf = new_out;
		outBufLen = want;
		memcpy(outBuf, "$ZOn|", ZON_LEN); // note: buffer can be new when first one was not allocated
	}

	strm->avail_in = (uInt)len;
	strm->next_in = (Bytef*)buffer; // note: deflate only reads input
	strm->avail_out = (uInt)(outBufLen - ZON_LEN);
	strm->next_out = (Bytef*)(outBuf + ZON_LEN);

	if (deflate(strm, Z_FINISH) != Z_STREAM_END) // compress, note: data that does not fit ends here too, reset next time makes stream usable again
		return NULL;

	outLen = (strm->total_out + ZON_LEN); // note: reset sets total to zero, so this is only this data

	if (keep) { // note: least recently used or empty entry is replaced
		sCache *ent = &mCache[0];

		for (sCache &cur: mCache) {
			if (cur.mUsed < ent->mUsed)
				ent = &cur;
		}

		ent->mHash = hash;
		ent->mLevel = comp_level;
		ent->mIn.assign(buffer, len);
		ent->mOutLen = outLen;

		if (len > outLen)
			ent->mOut.assign(outBuf, outLen);
		else
			ent->mOut.clear();

		ent->mUsed = ++mCacheUse;
	}

	if (len <= outLen) // out data size is bigger or equal in data size, fall back to raw data
		return NULL;

	return outBuf;
}

	}; // namespace nUtils
}; // namespace nVerliHub
