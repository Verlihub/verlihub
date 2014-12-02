/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2012 Verlihub Team, devs at verlihub-project dot org
	Copyright (C) 2013-2014 RoLex, webmaster at feardc dot net

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

#include <zlib.h>
#include "czlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

namespace nVerliHub {
	namespace nUtils {

cZLib::cZLib():
	zBufferPos(0),
	zBufferLen(ZLIB_BUFFER_SIZE),
	outBufferLen(ZLIB_BUFFER_SIZE)
{
	outBuffer = (char*)calloc(ZLIB_BUFFER_SIZE, 1);
	zBuffer = (char*)calloc(ZLIB_BUFFER_SIZE, 1);
	memcpy(outBuffer, "$ZOn|", ZON_LEN);
}

cZLib::~cZLib()
{
	if (outBuffer)
		free(outBuffer);

	if (zBuffer)
		free(zBuffer);
}

char *cZLib::Compress(const char *buffer, size_t len, size_t &outLen)
{
	/*
		check if we are compressing same data as last time
		then return last compressed buffer to save resources
	*/

	/*
	if ((buffer != NULL) && (zBuffer != NULL) && (outBuffer != NULL)) {
		string last_data, this_data(buffer);
		last_data.assign(zBuffer, len);
		// debug
		cerr << "last_data(" << last_data.size() << "): " << last_data << endl;
		cerr << "this_data(" << this_data.size() << "): " << this_data << endl;

		if (last_data == this_data) {
			string last_zlib(outBuffer);
			size_t last_size = last_zlib.size();
			// debug
			cerr << "last_zlib(" << last_size << "): " << last_zlib << endl;

			if (last_size > ZON_LEN) {
				if (last_size < len) {
					// debug
					cerr << "return last buffer" << endl;
					outLen = last_size;
					return outBuffer;
				} else {
					// debug
					cerr << "return same buffer" << endl;
					return NULL;
				}
			}
		}
	}

	// debug
	cerr << "keep compressing" << endl;
	*/

	z_stream strm;
	memset((void*) &strm, '\0', sizeof(strm));

	if ((zBufferLen - zBufferPos) < len)
		for (; (zBufferLen - zBufferPos) < len; zBufferLen += ZLIB_BUFFER_SIZE);

	char *new_buffer = (char*)realloc(zBuffer, zBufferLen);

	if (new_buffer == NULL) { // todo: throw exception and log error
		free(zBuffer);
		return NULL;
	}

	zBuffer = new_buffer;
	strm.zalloc = Z_NULL; // allocate deflate state
	strm.zfree = Z_NULL;
	strm.data_type = Z_TEXT;

	if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK)
		return NULL;

	memcpy(zBuffer + zBufferPos, buffer, len); // copy data in zlib buffer
	zBufferPos += len;

	if (outBufferLen < zBufferPos) // increase out buffer if not enough
		for (; outBufferLen < zBufferPos; outBufferLen += ZLIB_BUFFER_SIZE);

	strm.avail_in = (uInt)zBufferPos;
	strm.next_in = (Bytef*)zBuffer;
	strm.next_out = (Bytef*)outBuffer + ZON_LEN; // $ZOn|
	strm.avail_out = (uInt)(outBufferLen - ZON_LEN);

	if (deflate(&strm, Z_FINISH) != Z_STREAM_END) { // compress
		deflateEnd(&strm);
		return NULL;
	}

	outLen = strm.total_out + ZON_LEN; // $ZOn|
	deflateEnd(&strm);

	if (zBufferPos < outLen) { // if compressed data is bigger than raw data, fallback to raw data
		outLen = zBufferPos;
		zBufferPos = 0;
		return zBuffer;
	}

	//zBuffer[zBufferPos] = '\0'; // clear for debug
	zBufferPos = 0;
	return outBuffer;
}

void cZLib::AppendData(const char *buffer, size_t len)
{
	if ((zBufferLen - zBufferPos) < len) // increase zlib buffer if not enough
		for (; (zBufferLen - zBufferPos) < len; zBufferLen += ZLIB_BUFFER_SIZE);

	char *new_buffer = (char*)realloc(zBuffer, zBufferLen);

	if (new_buffer == NULL) { // todo: throw exception and log error
		free(zBuffer);
		return;
	}

	zBuffer = new_buffer;
	memcpy(zBuffer + zBufferPos, buffer, len);
	zBufferPos += len;
}

	}; // namespace nUtils
}; // namespace nVerliHub
