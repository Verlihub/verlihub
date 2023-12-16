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

#include "cserverdc.h"
#include "cban.h"
#include "ctime.h"
#include "cbanlist.h"
#include "stringutils.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nEnums;
	using namespace nUtils;
	using namespace nSocket;
	namespace nTables {

cBan::cBan(cServerDC *s):
	cObj("cBan"),
	mS(s)
{
	mShare = 0;
	mDateStart = 0;
	mDateEnd = 0;
	mLastHit = 0;
	mType = 0;
	mRangeMin = 0;
	mRangeMax = 0;
	//mDisplayType = 0;
}

cBan::~cBan()
{}

cUnBan::cUnBan(cServerDC *s):
	cBan(s)
{
	mDateUnban = 0;
}

cUnBan::cUnBan(cBan &Ban, cServerDC *s):
	cBan(s)
{
	mIP = Ban.mIP;
	mNick = Ban.mNick;
	mHost = Ban.mHost;
	mShare = Ban.mShare;
	mRangeMin = Ban.mRangeMin;
	mRangeMax = Ban.mRangeMax;
	mDateStart = Ban.mDateStart;
	mDateEnd = Ban.mDateEnd;
	mLastHit = Ban.mLastHit;
	mNickOp = Ban.mNickOp;
	mReason = Ban.mReason;
	mType = Ban.mType;
	mDateUnban = 0;
}

cUnBan::~cUnBan()
{}

ostream & operator << (ostream &os, cBan &ban)
{
	/*
	switch (ban.mDisplayType) {
		case 0:
	*/
			ban.DisplayComplete(os);
	/*
			break;
		case 1:
			ban.DisplayUser(os);
			break;
		case 2:
			ban.DisplayKick(os);
			break;
		default:
			os << _("Unknown");
			break;
	}
	*/

	return os;
}

void cBan::DisplayUser(ostream &os)
{
	os << "\r\n";

	if (mNick.size())
		os << " [*] " << autosprintf(_("Nick: %s"), mNick.c_str()) << "\r\n";

	if (!mRangeMin && mIP.size() && (mIP[0] != '_'))
		os << " [*] " << autosprintf(_("IP: %s"), mIP.c_str()) << "\r\n";

	string loaddr, hiaddr;

	if (mRangeMin) {
		cBanList::Num2Ip(mRangeMin, loaddr);
		cBanList::Num2Ip(mRangeMax, hiaddr);
		os << " [*] " << autosprintf(_("IP range: %s - %s"), loaddr.c_str(), hiaddr.c_str()) << "\r\n";
	}

	if (mShare)
		os << " [*] " << autosprintf(_("Share: %llu [%s]"), mShare, convertByte(mShare, false).c_str()) << "\r\n";

	if (mReason.size())
		os << " [*] " << autosprintf(_("Reason: %s"), mReason.c_str()) << "\r\n";

	if (mS->mC.ban_extra_message.size()) // extra ban message
		os << " [*] " << autosprintf(_("Extra: %s"), mS->mC.ban_extra_message.c_str()) << "\r\n";

	os << " [*] " << _("Time") << ": ";

	if (mDateEnd) {
		cTimePrint HowLong(mDateEnd - mS->mTime.Sec());
		os << autosprintf(_("%s left"), HowLong.AsPeriod().AsString().c_str());
	} else {
		os << _("Permanently");
	}

	os << "\r\n";

	if (mNoteUsr.size())
		os << " [*] " << autosprintf(_("User note: %s"), mNoteUsr.c_str()) << "\r\n";
}

/*
void cUnBan::DisplayUser(ostream &os)
{
	this->cBan::DisplayUser(os);
	os << autosprintf(_("Removed: %s by %s because %s"), cTimePrint(mDateUnban,0).AsDate().AsString().c_str(), mUnNickOp.c_str(), mUnReason.c_str()) << "\r\n";
}
*/

void cBan::DisplayComplete(ostream &os)
{
	DisplayUser(os);

	if (mNoteOp.size())
		os << " [*] " << autosprintf(_("Operator note: %s"), mNoteOp.c_str()) << "\r\n";

	os << " [*] " << autosprintf(_("Type: %s"), this->GetBanType()) << "\r\n";

	if (mNickOp.size())
		os << " [*] " << autosprintf(_("Operator: %s"), mNickOp.c_str()) << "\r\n";

	os << " [*] " << _("Last hit") << ": ";

	if (mLastHit)
		os << autosprintf(_("%s ago"), cTimePrint(mS->mTime.Sec() - mLastHit).AsPeriod().AsString().c_str());
	else
		os << _("Never");

	os << "\r\n";
}

const char *cBan::GetBanType()
{
	static const char *banTypes[] = {_("Nick + IP"), _("IP"), _("Nick"), _("IP range"), _("Host level 1"), _("Host level 2"), _("Host level 3"), _("Share size"), _("Nick prefix"), _("Reverse host")};
	return banTypes[mType];
}

void cBan::SetType(unsigned type)
{
	/*
		Undefined behavior. Check the shift operator '<<'. The right operand ('mType' = [0..1023]) is greater than or equal to the length in bits of the promoted left operand.
		this is false because: mType < nEnums::eBF_LAST
	*/

	for (mType = 0; mType < nEnums::eBF_LAST; mType++) {
		if (type == (unsigned)(1 << mType))
			return;
	}

	mType = eBF_NICK; // default type in case we fail
}

/*
void cUnBan::DisplayComplete(ostream &os)
{
	this->cBan::DisplayComplete(os);
	os << autosprintf(_("Removed: %s by %s because %s"), cTimePrint(mDateUnban,0).AsDate().AsString().c_str(), mUnNickOp.c_str(), mUnReason.c_str()) << "\r\n";
}
*/

void cBan::DisplayKick(ostream &os)
{
	os << "\t\t";

	if (mDateEnd) {
		cTimePrint HowLong(mDateEnd - mS->mTime.Sec(), 0);

		if (HowLong.Sec() < 0) {
			os << autosprintf(_("Expired %s"), cTimePrint(mDateEnd, 0).AsDate().AsString().c_str());
		} else {
			os << HowLong.AsPeriod().AsString().c_str();
			os << "\t";
		}
	} else {
		os << _("Permanent");
		os << "\t";
	}

	os << "\t";

	if (mLastHit)
		os << cTimePrint(mS->mTime.Sec() - mLastHit).AsPeriod().AsString();
	else
		os << _("Never");
}

void cBan::DisplayInline(ostream &os)
{
	const char* ban_type = GetBanType();
	string item;

	switch (1 << mType) {
		case eBF_NICK:
		case eBF_PREFIX:
			item = mNick;
			break;
		case eBF_NICKIP:
			item = mNick + " + " + mIP;
			break;
		case eBF_IP:
		case eBF_RANGE:
			item = mIP;
			break;
		case eBF_HOST1:
		case eBF_HOST2:
		case eBF_HOST3:
			item = mHost;
			break;
		case eBF_SHARE:
			item = StringFrom(mShare); // dont use convertByte because we banned exact share
			break;
		default:
			item = mNick;
			break;
	}

	os << "\t" << item;

	if (item.size() <= 8)
		os << "\t";

	if (item.size() <= 16)
		os << "\t";

	if (item.size() <= 24)
		os << "\t";

	os << "\t" << ban_type;

	if (strlen(ban_type) <= 8)
		os << "\t";

	os << "\t" << mNickOp;
	DisplayKick(os);
}

	}; // namespace nTables
}; // Namespace nVerliHub
