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

#include <iostream>
#include <string>
#include "cdctag.h"
#include "cconntypes.h"
#include <string>
#include <iostream>
#include <math.h>
#include "cdcconf.h"
#include "i18n.h"

using namespace std;
namespace nVerliHub {
	using namespace nTables;
	using namespace nSocket;
	using namespace nEnums;
cDCTag::cDCTag(cServerDC *mS, cDCClient *c) : mServer(mS), client(c)
{
	mTotHubs = -1;
	mHubsUsr = -1;
	mHubsReg = -1;
	mHubsOp = -1;
	mSlots = -1;
	mLimit = -1;
}

cDCTag::cDCTag(cServerDC *mS) : mServer(mS)
{
	mTotHubs = -1;
	mHubsUsr = -1;
	mHubsReg = -1;
	mHubsOp = -1;
	mSlots = -1;
	mLimit = -1;
}

cDCTag::~cDCTag() { }

bool cDCTag::ValidateTag(ostream &os, cConnType *conn_type, int &code)
{
	if (client && client->mBan) {
		os << _("Your client is banned.");
		code = eTC_BANNED;
		return false;
	}

	// not parsed or unknown tag

	if ((mClientMode == eCM_SOCK5) && !mServer->mC.tag_allow_sock5) {
		os << _("Connections through proxy server are not allowed in this hub.");
		code = eTC_SOCK5;
		return false;
	}

	if ((mClientMode == eCM_PASSIVE) && !mServer->mC.tag_allow_passive) {
		os << _("Passive connections are restricted, consider changing to active.");
		code = eTC_PASSIVE;
		return false;
	}

	if ((mTotHubs < 0) || (mSlots < 0)) {
		os << _("Your client tag is reporting less than 0 hubs or less than 0 slots, looks like a bug in your client.");
		code = eTC_PARSE;
		return false;
	}

	string MsgToUser;

	if (!mServer->mC.tag_allow_unknown && !client) {
		os << _("Unknown clients are not allowed in this hub.");
		code = eTC_UNKNOWN;
		return false;
	}

	if ((mServer->mC.tag_max_hubs > 0) && (mTotHubs > mServer->mC.tag_max_hubs)) {
		os << autosprintf(_("Too many open hubs, maximum is %d and you have %d."), mServer->mC.tag_max_hubs, mTotHubs);
		code = eTC_MAX_HUB;
		return false;
	}

	if ((mServer->mC.tag_min_hubs > 0) && (mTotHubs < mServer->mC.tag_min_hubs)) {
		os << autosprintf(_("Too few open hubs, minimum is %d and you have %d."), mServer->mC.tag_min_hubs, mTotHubs);
		code = eTC_MIN_HUB;
		return false;
	}

	if ((mHubsUsr != -1) && (mHubsUsr < mServer->mC.tag_min_hubs_usr)) {
		os << autosprintf(_("Too few open hubs as user, minimum is %d and you have %d."), mServer->mC.tag_min_hubs_usr, mHubsUsr);
		code = eTC_MIN_HUB_USR;
		return false;
	}

	if ((mHubsReg != -1) && (mHubsReg < mServer->mC.tag_min_hubs_reg)) {
		os << autosprintf(_("Too few open hubs as registered user, minimum is %d and you have %d."), mServer->mC.tag_min_hubs_reg, mHubsReg);
		code = eTC_MIN_HUB_REG;
		return false;
	}

	if ((mHubsOp != -1) && (mHubsOp < mServer->mC.tag_min_hubs_op)) {
		os << autosprintf(_("Too few open hubs as operator, minimum is %d and you have %d."), mServer->mC.tag_min_hubs_op, mHubsOp);
		code = eTC_MIN_HUB_OP;
		return false;
	}

	if ((conn_type->mTagMinSlots > 0) && (mSlots < conn_type->mTagMinSlots)) {
		os << autosprintf(_("Too little open slots for your connection type %s, minimum is %d and you have %d."), conn_type->mIdentifier.c_str(), conn_type->mTagMinSlots, mSlots);
		code = eTC_MIN_SLOTS;
		return false;
	}

	if ((conn_type->mTagMaxSlots > 0) && (mSlots > conn_type->mTagMaxSlots)) {
		os << autosprintf(_("Too many open slots for your connection type %s, maximum is %d and you have %d."), conn_type->mIdentifier.c_str(), conn_type->mTagMaxSlots, mSlots);
		code = eTC_MAX_SLOTS;
		return false;
	}

	double ratio = 0;
	if (mTotHubs > 0) ratio = (double)mSlots / mTotHubs;

	if ((mServer->mC.tag_min_hs_ratio > 0) && (ratio < mServer->mC.tag_min_hs_ratio)) {
		os << autosprintf(_("Your slots per hub ratio %.2f is too low, minimum is %.2f."), ratio, mServer->mC.tag_min_hs_ratio);
		os << " " << autosprintf(_("Open %d slots for %d hubs."), (int)(ceil((double)mTotHubs * mServer->mC.tag_min_hs_ratio)), mTotHubs);
		code = eTC_MIN_HS_RATIO;
		return false;
	}

	if ((mServer->mC.tag_max_hs_ratio > 0) && (ratio > mServer->mC.tag_max_hs_ratio)) {
		os << autosprintf(_("Your slots per hub ratio %.2f is too high, maximum is %.2f."), ratio, mServer->mC.tag_max_hs_ratio);
		os << " " << autosprintf(_("Open %d slots for %d hubs."), (int)(ceil((double)mTotHubs * mServer->mC.tag_max_hs_ratio)), mTotHubs);
		code = eTC_MAX_HS_RATIO;
		return false;
	}

	if (mLimit >= 0) {
		// well, DCGUI bug, if (tag->mClientType == eCT_DCGUI) limit *= mSlots;
		if ((conn_type->mTagMinLimit) > mLimit) {
			os << autosprintf(_("Too low upload limit for your connection type %s, minimum upload rate is %.2f."), conn_type->mIdentifier.c_str(), conn_type->mTagMinLimit);
			code = eTC_MIN_LIMIT;
			return false;
		}

		if ((conn_type->mTagMinLSRatio * mSlots) > mLimit ) {
			os << autosprintf(_("Too low upload limit per slot for your connection type %s, minimum upload limit is %.2f per slot."), conn_type->mIdentifier.c_str(), conn_type->mTagMinLSRatio);
			code = eTC_MIN_LS_RATIO;
			return false;
		}
	}

	// use tag_min_version and tag_max_version for unknown client or use the version number in the matching rule
	double minVersion = mServer->mC.tag_min_version, maxVersion = mServer->mC.tag_max_version;

	if (client) {
		minVersion = client->mMinVersion;
		maxVersion = client->mMaxVersion;
	}

	if ((minVersion != -1) && (mClientVersion < minVersion)) {
		os << _("Your client version is too old, please upgrade it.") << " ";

		if (client)
			os << autosprintf(_("Allowed minimum version number for %s client is: %.2f"), client->mName.c_str(), minVersion);
		else
			os << autosprintf(_("Allowed minimum version number for your client is: %.2f"), minVersion);

		code = eTC_MIN_VERSION;
		return false;
	}

	if ((maxVersion != -1) && (mClientVersion < maxVersion)) {
		os << _("Your client version is too recent, please downgrade it.") << " ";

		if (client)
			os << autosprintf(_("Allowed maximum version number for %s client is: %.2f"), client->mName.c_str(), maxVersion);
		else
			os << autosprintf(_("Allowed maximum version number for your client is: %.2f"), maxVersion);

		code = eTC_MAX_VERSION;
		return false;
	}

	return true;
}

ostream &operator << (ostream &os, cDCTag &tag)
{
	os << "Client " << (tag.client ? tag.client->mName : "Unknown") << " (v." << tag.mClientVersion << ")" << endl;
	os << "[::] Mode: ";

	if (tag.mClientMode == eCM_ACTIVE)
		os << "Active";
	else if (tag.mClientMode == eCM_PASSIVE)
		os << "Passive";
	else if (tag.mClientMode == eCM_SOCK5)
		os << "SOCKS";
	else if (tag.mClientMode == eCM_OTHER)
		os << "Other";

	os << endl;
	os << "[::] Open hubs: ";
	if(tag.mTotHubs >=0)
		os << tag.mTotHubs;
	else
		os <<"Not available";
	os << endl;
	os << "[::] Open slots: ";
	if(tag.mSlots >=0)
		os << tag.mSlots;
	else
		os << "Not available";
	os << endl;
	return os;
}

}; // namespace nVerliHub
