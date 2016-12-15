/*
	Copyright (C) 2003-2005 Daniel Muller, dan at verliba dot cz
	Copyright (C) 2006-2017 Verlihub Team, info at verlihub dot net

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

#include "cpenaltylist.h"
#include "cquery.h"
#include "i18n.h"
#include <sys/time.h>

namespace nVerliHub {
	using namespace nMySQL;
	namespace nTables {

cPenaltyList::cPenaltyList(cMySQL &mysql):
	cConfMySQL(mysql),
	mCache(mysql, "temp_rights", "nick")
{
	mMySQLTable.mName = "temp_rights";
	AddCol("nick", "varchar(128) primary key", "", false, mModel.mNick);
	AddPrimaryKey("nick");
	AddCol("op", "varchar(128)", "", true, mModel.mOpNick);
	AddCol("since", "int(11)", "", true, mModel.mSince);
	AddCol("st_chat", "int(11)", "1", true, mModel.mStartChat);
	AddCol("st_search", "int(11)", "1", true, mModel.mStartSearch);
	AddCol("st_ctm", "int(11)", "1", true, mModel.mStartCTM);
	AddCol("st_pm", "int(11)", "1", true, mModel.mStartPM);
	AddCol("st_kick", "int(11)", "1", true,mModel.mStopKick);
	AddCol("st_share0", "int(11)", "1", true, mModel.mStopShare0);
	AddCol("st_reg", "int(11)", "1", true, mModel.mStopReg);
	AddCol("st_opchat", "int(11)", "1", true, mModel.mStopOpchat);
	mMySQLTable.mExtra = "index creation_index(since)";
	SetBaseTo(&mModel);
}

cPenaltyList::~cPenaltyList()
{}

void cPenaltyList::Cleanup()
{
	cTime Now = cTime().Sec();
	cQuery query(mMySQL);
	query.OStream() << "delete from `" << mMySQLTable.mName << "` where `since` < " << (Now - (3600 * 24 * 7)); // and `st_chat` != 0 and `st_search` != 0 and `st_ctm` != 0 and `st_pm` != 0 and `st_kick` != 0 and `st_share0` != 0 and `st_opchat` != 0 and `st_reg` != 0
	query.Query();
	query.Clear();
}

bool cPenaltyList::LoadTo(sPenalty &pen, const string &Nick)
{
	if (mCache.IsLoaded() && !mCache.Find(Nick))
		return false;

	SetBaseTo(&pen);
	pen.mNick = Nick;
	return LoadPK();
}

bool cPenaltyList::AddPenalty(sPenalty &penal)
{
	SetBaseTo(&mModel);
	mModel.mNick = penal.mNick;
	mModel.mOpNick = penal.mOpNick;
	bool keep = false;

	if (LoadPK()) {
		if (penal.mStartChat > mModel.mStartChat)
			mModel.mStartChat = penal.mStartChat;

		if (penal.mStartCTM > mModel.mStartCTM)
			mModel.mStartCTM = penal.mStartCTM;

		if (penal.mStartPM > mModel.mStartPM)
			mModel.mStartPM = penal.mStartPM;

		if (penal.mStartSearch > mModel.mStartSearch)
			mModel.mStartSearch = penal.mStartSearch;

		if (penal.mStopKick > mModel.mStopKick)
			mModel.mStopKick = penal.mStopKick;

		if (penal.mStopShare0 > mModel.mStopShare0)
			mModel.mStopShare0 = penal.mStopShare0;

		if (penal.mStopReg > mModel.mStopReg)
			mModel.mStopReg = penal.mStopReg;

		if (penal.mStopOpchat > mModel.mStopOpchat)
			mModel.mStopOpchat = penal.mStopOpchat;

		keep = mModel.ToKeepIt();
	} else {
		SetBaseTo(&penal);
		keep = penal.ToKeepIt();

		if (keep && mCache.IsLoaded())
			mCache.Add(penal.mNick);
	}

	DeletePK();

	if (keep)
		return SavePK(false);
	else
		return false;
}

bool cPenaltyList::RemPenalty(sPenalty &penal)
{
	SetBaseTo(&mModel);
	mModel.mNick = penal.mNick;
	mModel.mOpNick = penal.mOpNick;
	cTime Now = cTime().Sec();
	bool keep = false;

	if (LoadPK()) {
		if (penal.mStartChat < Now)
			mModel.mStartChat = 1;
		else
			keep = true;

		if (penal.mStartCTM < Now)
			mModel.mStartCTM = 1;
		else
			keep = true;

		if (penal.mStartPM < Now)
			mModel.mStartPM = 1;
		else
			keep = true;

		if (penal.mStartSearch < Now)
			mModel.mStartSearch = 1;
		else
			keep = true;

		if (penal.mStopKick < Now)
			mModel.mStopKick = 1;
		else
			keep = true;

		if (penal.mStopShare0 < Now)
			mModel.mStopShare0 = 1;
		else
			keep = true;

		if (penal.mStopReg < Now)
			mModel.mStopReg = 1;
		else
			keep = true;

		if (penal.mStopOpchat < Now)
			mModel.mStopOpchat = 1;
		else
			keep = true;
	}

	DeletePK();

	if (keep)
		return SavePK();

	return true;
}

void cPenaltyList::ListAll(ostream &os)
{
	cQuery query(mMySQL);
	query.OStream() << "select `nick` from `" << mMySQLTable.mName << "`";
	query.Query();
	unsigned int tot = query.StoreResult();
	cTime now = cTime().Sec();
	long dif;
	MYSQL_ROW row = NULL;
	bool sep;
	sPenalty pen;
	os << "\r\n\r\n\t" << _("Nick") << "\t\t" << _("Operator") << "\t\t" << _("Rights and restrictions") << "\r\n";
	os << "\t" << string(120, '-') << "\r\n";

	for (unsigned int pos = 0; pos < tot; pos++) {
		row = query.Row();

		if (row && this->LoadTo(pen, row[0])) {
			os << "\t" << pen.mNick << "\t\t" << pen.mOpNick << "\t\t";
			sep = false;

			if (pen.mStartChat > 1) {
				dif = pen.mStartChat - now;

				if (dif > 0) {
					//if (sep)
						//os << ", ";

					os << autosprintf(_("Chat: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStartPM > 1) {
				dif = pen.mStartPM - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("PM: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStartSearch > 1) {
				dif = pen.mStartSearch - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Search: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStartCTM > 1) {
				dif = pen.mStartCTM - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Download: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopShare0 > 1) {
				dif = pen.mStopShare0 - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Share: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopReg > 1) {
				dif = pen.mStopReg - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Register: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopOpchat > 1) {
				dif = pen.mStopOpchat - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Operator chat: %s"), cTime(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopKick > 1) {
				dif = pen.mStopKick - now;

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Kick: %s"), cTime(dif).AsPeriod().AsString().c_str());
					//sep = true;
				}
			}

			os << "\r\n";
		}
	}

	query.Clear();
}

	}; // namespace nTables
}; // namespace nVerliHub
