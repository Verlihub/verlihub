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

#ifndef NZLIB_H
#define NZLIB_H

#include <cstring>
#include <string>
#include <stdint.h>

#define ZLIB_BUFFER_SIZE 256*1024 // deflateInit() will allocate on the order of 256K bytes for the internal state
#define ZON_LEN 5 // "$ZOn|" command length

using namespace std;

namespace nVerliHub {
namespace nUtils {

	/// @addtogroup Core
	/// @{
	/**
	 * Wrapper class for zlib compression library.
	 * @author Davide Simoncelli
	 */
	class cZLib
	{
		public:
			/**
			* Class constructor.
			*/
			cZLib();

			/**
			* Class destructor.
			*/
			~cZLib();

			/**
			* Compress the given data.
			* Result of same data compressed recently is taken from cache.
			*
			* @param buffer Pointer to buffer data.
			* @param len Data length.
			* @param outLen Length of compressed data.
			* @param cached Set when result was taken from cache.
			* @return Pointer to compressed data, valid until next call, NULL when compressed data is not smaller or compression failed.
			*/
			const char* Compress(const char *buffer, size_t len, size_t &outLen, int &err, int level, bool &cached);

			// level that zlib really uses for configured level, from 1 to 9
			static int Level(int level);

			// memory of cached data
			size_t GetCacheSize() const;

			// memory of compression buffer
			size_t GetBufferSize() const
			{
				return outBufLen;
			}

			// note: position 0 is most recently used data, so hits by position show how many entries are useful
			static constexpr size_t sCacheSize = 16;

			unsigned long long CacheHits(size_t pos) const
			{
				return ((pos < sCacheSize) ? mCacheHits[pos] : 0);
			}

			unsigned long long CacheMisses() const
			{
				return mCacheMisses;
			}
		private:
			// out buffer
			char *outBuf;
			size_t outBufLen;

			// note: recent different data with its result, users without own data in a flush get same data as others of same group, so it is compressed once per group
			struct sCache
			{
				size_t mHash = 0;
				int mLevel = 0;
				string mIn;
				string mOut; // note: compressed data with "$ZOn|" in front, empty when it is not smaller
				size_t mOutLen = 0;
				unsigned long long mUsed = 0; // note: 0 = empty, else order of last use
			};

			static constexpr size_t sCacheMax = 65536; // note: bigger data is not kept, so cache takes at most about 2 mb
			sCache mCache[sCacheSize];
			unsigned long long mCacheUse = 0;
			unsigned long long mCacheHits[sCacheSize] = {};
			unsigned long long mCacheMisses = 0; // note: every real compression, also of data too big for cache

			// note: one deflate stream for all flushes, reset is much cheaper than setting it up, type is z_stream, so zlib.h is not needed here
			void *mStream = NULL;
			int mStreamLevel = 0;
			void EndStream();
	};

}; // namespace nUtils
}; // namespace nVerliHub

#endif
