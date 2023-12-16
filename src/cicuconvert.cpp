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

#include "cicuconvert.h"
#include "stringutils.h"
#include "cserverdc.h"

#include <unicode/uclean.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>
#include <unicode/uvernum.h>

using namespace std;

namespace nVerliHub {
	namespace nUtils {

cICUConvert::cICUConvert(cServerDC *serv):
	cObj("cICUConvert"),
	mServ(serv),
	mConv(NULL),
	mTran(NULL),
	mCharSet(DEFAULT_HUB_ENCODING),
	mICUVer(U_ICU_VERSION)
{
	UErrorCode ok = U_ZERO_ERROR;
	u_init(&ok);

	if (U_FAILURE(ok)) {
		vhLog(0) << "Failed to initialize ICU library, conversion and transliteration will be disabled: " << u_errorName(ok) << endl;
		u_cleanup();

	} else {
		if (mServ && mServ->mC.hub_encoding.size())
			mCharSet = mServ->mC.hub_encoding;

		ok = U_ZERO_ERROR;
		mConv = ucnv_open(mCharSet.c_str(), &ok);

		if (U_FAILURE(ok)) {
			vhLog(0) << "Failed to create ICU converter using '" << mCharSet << "', conversion will be disabled: " << u_errorName(ok) << endl;

			if (mConv) {
				ucnv_close(mConv);
				mConv = NULL;
			}
		}

		/*
		StringEnumeration *enu = Transliterator::getAvailableIDs(ok);
		int cnt = enu->count(ok), len = 0;
		const char* cur = NULL;

		for (int pos = 0; pos < cnt; ++pos) {
			cur = enu->next(&len, ok);

			if (len && cur)
				vhLog(0) << cur << endl;
		}

		delete enu;
		*/

		//UParseError err;
		ok = U_ZERO_ERROR;
		//mTran = Transliterator::createFromRules(UNICODE_STRING_SIMPLE("VHICU"), UNICODE_STRING_SIMPLE("NFD; [:M:] Remove; NFC;"), UTRANS_FORWARD, err, ok); // Any-Latin; Latin-ASCII; Title;
		// todo: spent three days on this, but valgrind still shows uninitialized values, please help
		mTran = Transliterator::createInstance("Any-Latin; Latin-ASCII;", UTRANS_FORWARD, ok); // NFD; [:M:] Remove; NFC;

		if (U_FAILURE(ok)) {
			vhLog(0) << "Failed to create ICU transliterator, transliteration will be disabled: " << u_errorName(ok) << endl;

			if (mTran) {
				delete mTran;
				mTran = NULL;
			}

		//} else {
			//Transliterator::registerInstance(mTran);
		}

		if (!mConv && !mTran)
			u_cleanup();
	}
}

cICUConvert::~cICUConvert()
{
	if (mConv) {
		ucnv_close(mConv);
		mConv = NULL;
	}

	if (mTran) {
		//Transliterator::unregister(mTran->getID()); // UNICODE_STRING_SIMPLE("VHICU")
		delete mTran;
		mTran = NULL;
	}

	u_cleanup();
}

bool cICUConvert::Convert(const char *udat, unsigned int ulen, string &conv, const string &tset)
{
	if (!mConv) // converter unavailable
		return false;

	UErrorCode ok = U_ZERO_ERROR;

	if (tset.size() && (mCharSet != tset)) {
		vhLog(0) << "Recreating ICU converter due to changed character set: " << mCharSet << " > " << tset << endl;
		mCharSet = tset;
		ucnv_close(mConv);
		mConv = ucnv_open(mCharSet.c_str(), &ok);

		if (U_FAILURE(ok)) {
			vhLog(0) << "Failed to create ICU converter using '" << mCharSet << "', conversion will be disabled: " << u_errorName(ok) << endl;

			if (mConv) {
				ucnv_close(mConv);
				mConv = NULL;
			}

			if (!mTran)
				u_cleanup();

			return false;
		}
	}

	UnicodeString ustr = UnicodeString::fromUTF8(StringPiece(udat, ulen));
	int len = UCNV_GET_MAX_BYTES_FOR_STRING(ulen, ucnv_getMaxCharSize(mConv));
	char *targ = (char*)malloc(len);
	ok = U_ZERO_ERROR;
	len = ucnv_fromUChars(mConv, targ, len, ustr.getTerminatedBuffer(), -1, &ok);

	if (U_FAILURE(ok) || (len <= 0)) {
		free(targ);
		return false;
	}

	conv.clear();
	conv.assign(targ, len);
	free(targ);
	return true;
}

bool cICUConvert::Translit(const char *udat, unsigned int ulen, string &tran)
{
	if (!mTran) // transliterator unavailable
		return false;

	UnicodeString ustr = UnicodeString::fromUTF8(StringPiece(udat, ulen));
	mTran->transliterate(ustr);
	tran.clear();
	ustr.toUTF8String(tran);
	return true;
}

	};
};
