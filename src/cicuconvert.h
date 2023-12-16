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

#ifndef NUTILSCICUCONVERT_H
#define NUTILSCICUCONVERT_H

#include "cobj.h"

#include <string>
#include <unicode/translit.h>
#include <unicode/ucnv.h>

using std::string;

namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
	};

	namespace nUtils {
		class cICUConvert: public cObj
		{
			public:
				cICUConvert(nSocket::cServerDC *);
				~cICUConvert();

				bool Convert(const char *udat, unsigned int ulen, string &conv, const string &tset = ""); // hub default character set
				bool Translit(const char *udat, unsigned int ulen, string &tran);

				virtual string GetICUVersion() const
				{
					return mICUVer;
				}
			private:
				nSocket::cServerDC *mServ;
				UConverter *mConv;
				Transliterator *mTran;

				string mCharSet; // last used character set
				string mICUVer;
		};
	};
};

#endif
