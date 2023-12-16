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
			* If there are available data in the internal buffer,
			* the given data is appended after the data and then
			* compression is performed.
			*
			* @param buffer Pointer to buffer data.
			* @param len Data length.
			* @param outLen Length of compressed data.
			* @return Pointer to compressed data.
			*/
			char* Compress(const char *buffer, size_t len, size_t &outLen, int &err, int level);

			// get buffer sizes
			size_t GetInBufLen()
			{
				return inBufLen;
			}
			size_t GetOutBufLen()
			{
				return outBufLen;
			}
		private:
			// in buffer
			char *inBuf;
			size_t inBufLen;
			size_t inLastLen;

			// out buffer
			char *outBuf;
			size_t outBufLen;
			size_t outLastLen;
	};

}; // namespace nUtils
}; // namespace nVerliHub

#endif
