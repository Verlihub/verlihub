/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2014 Verlihub Project, devs at verlihub-project dot org

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

cZLib::cZLib() :
zBufferPos(0),
zBufferLen(ZLIB_BUFFER_SIZE),
outBufferLen(ZLIB_BUFFER_SIZE)
{
	outBuffer = (char *) calloc(ZLIB_BUFFER_SIZE, 1);
	zBuffer = (char *) calloc(ZLIB_BUFFER_SIZE, 1);
	memcpy(outBuffer, "$ZOn|", 5);
}

cZLib::~cZLib()
{
	if(outBuffer)
		free(outBuffer);
	if(zBuffer)
		free(zBuffer);
}

char *cZLib::Compress(const char *buffer, size_t len, size_t &outLen)
{
	z_stream strm;
	memset((void *) &strm, '\0', sizeof(strm));
	if((zBufferLen - zBufferPos) < len)
		for(; (zBufferLen - zBufferPos) < len; zBufferLen += ZLIB_BUFFER_SIZE);

	char * new_buffer = (char *) realloc(zBuffer, zBufferLen);
	// TODO: Throw exception and log error
	if (new_buffer == NULL) {
		free(zBuffer);
		return NULL;
	}
	zBuffer = new_buffer;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.data_type = Z_TEXT;

	if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK)
		return NULL;

	// Copy data in ZLib buffer
	memcpy(zBuffer + zBufferPos, buffer, len);
	zBufferPos += len;

	// Increase out buffer if not enough
	if(outBufferLen < zBufferPos)
		for(; outBufferLen < zBufferPos; outBufferLen += ZLIB_BUFFER_SIZE);

	strm.avail_in = (uInt) zBufferPos;
	strm.next_in  = (Bytef*) zBuffer;

	strm.next_out = (Bytef*) outBuffer + ZON_LEN; /** $ZOn| **/
	strm.avail_out = (uInt) (outBufferLen - ZON_LEN);

	// Compress
	if(deflate(&strm, Z_FINISH) != Z_STREAM_END) {
		deflateEnd(&strm);
		return NULL;
	}

	outLen = strm.total_out + 5; /** $ZOn and pipe **/
	deflateEnd(&strm);
	// If compressed data is bigger than raw data, fallback to raw data
	if(zBufferPos < outLen) {

		outLen = zBufferPos;
		zBufferPos = 0;
		return zBuffer;
	}

	//Clear for DEBUG
	//zBuffer[zBufferPos] = '\0';
	zBufferPos = 0;
	return outBuffer;
}

void cZLib::AppendData(const char *buffer, size_t len)
{
	// Increase ZLib buffer if not enough
	if((zBufferLen - zBufferPos) < len)
		for(; (zBufferLen - zBufferPos) < len; zBufferLen += ZLIB_BUFFER_SIZE);

	char * new_buffer = (char *) realloc(zBuffer, zBufferLen);
	// TODO: Throw exception and log error
	if (new_buffer == NULL) {
		free(zBuffer);
		return;
	}
	zBuffer = new_buffer;
	memcpy(zBuffer + zBufferPos, buffer, len);
	zBufferPos += len;
}
	}; // namespace nUtils
}; // namespace nVerliHub
