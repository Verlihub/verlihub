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
#include "cpenaltylist.h"
#include "cquery.h"
#include "i18n.h"

namespace nVerliHub {
	using namespace nMySQL;
	using namespace nSocket;

	namespace nTables {

cPenaltyList::cPenaltyList(cMySQL &mysql, cServerDC *serv):
	cConfMySQL(mysql),
	mCache(mysql, "temp_rights", "nick"),
	mServ(serv)
{
	mMySQLTable.mName = "temp_rights";
	AddCol("nick", "varchar(128)", "", false, mModel.mNick);
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
	const long now = mServ->mTime.Sec(); // Now -= (60 * 60 * 24 * 7);
	cQuery query(mMySQL);
	query.OStream() << "delete from `" << mMySQLTable.mName << "` where (`st_chat` < " << now << ") and (`st_search` < " << now << ") and (`st_ctm` < " << now << ") and (`st_pm` < " << now << ") and (`st_kick` < " << now << ") and (`st_share0` < " << now << ") and (`st_reg` < " << now << ") and (`st_opchat` < " << now << ')';
	query.Query();
	query.Clear();
}

void cPenaltyList::CleanType(const string &type)
{
	cQuery query(mMySQL);
	query.OStream() << "delete from `" << mMySQLTable.mName << "` where (`";
	cConfMySQL::WriteStringConstant(query.OStream(), type);
	query.OStream() << "` > 1)";
	query.Query();
	query.Clear();
}

bool cPenaltyList::LoadTo(sPenalty &pen, const string &nick)
{
	if (mServ->mC.use_penlist_cache && (!mCache.IsLoaded() || !mCache.Find(nick))) // table can be empty aswell
		return false;

	SetBaseTo(&pen);
	pen.mNick = nick;
	return LoadPK();
}

bool cPenaltyList::AddPenalty(sPenalty &penal)
{
	SetBaseTo(&mModel);
	mModel.mNick = penal.mNick;
	mModel.mOpNick = penal.mOpNick;
	bool keep = false;

	if (LoadPK()) { // existing user
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

	} else { // new user
		SetBaseTo(&penal);
		keep = penal.ToKeepIt();

		if (keep && mServ->mC.use_penlist_cache) // add to cache
			mCache.Add(penal.mNick); // todo: nick2dbkey
	}

	if (keep)
		return SavePK(true);

	DeletePK();

	if (mServ->mC.use_penlist_cache)
		ReloadCache();

	return true;
}

bool cPenaltyList::RemPenalty(sPenalty &penal)
{
	SetBaseTo(&mModel);
	mModel.mNick = penal.mNick;
	mModel.mOpNick = penal.mOpNick;
	bool keep = false;

	if (LoadPK()) { // existing user
		if (penal.mStartChat < mServ->mTime.Sec())
			mModel.mStartChat = 1;
		else
			keep = true;

		if (penal.mStartCTM < mServ->mTime.Sec())
			mModel.mStartCTM = 1;
		else
			keep = true;

		if (penal.mStartPM < mServ->mTime.Sec())
			mModel.mStartPM = 1;
		else
			keep = true;

		if (penal.mStartSearch < mServ->mTime.Sec())
			mModel.mStartSearch = 1;
		else
			keep = true;

		if (penal.mStopKick < mServ->mTime.Sec())
			mModel.mStopKick = 1;
		else
			keep = true;

		if (penal.mStopShare0 < mServ->mTime.Sec())
			mModel.mStopShare0 = 1;
		else
			keep = true;

		if (penal.mStopReg < mServ->mTime.Sec())
			mModel.mStopReg = 1;
		else
			keep = true;

		if (penal.mStopOpchat < mServ->mTime.Sec())
			mModel.mStopOpchat = 1;
		else
			keep = true;
	}

	if (keep)
		return SavePK(true);

	DeletePK();

	if (mServ->mC.use_penlist_cache)
		ReloadCache();

	return true;
}

void cPenaltyList::ListAll(ostream &os)
{
	cQuery query(mMySQL);
	query.OStream() << "select `nick` from `" << mMySQLTable.mName << '`';
	query.Query();
	unsigned int tot = query.StoreResult();
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
				dif = pen.mStartChat - mServ->mTime.Sec();

				if (dif > 0) {
					//if (sep)
						//os << ", ";

					os << autosprintf(_("Chat: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStartPM > 1) {
				dif = pen.mStartPM - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("PM: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStartSearch > 1) {
				dif = pen.mStartSearch - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Search: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStartCTM > 1) {
				dif = pen.mStartCTM - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Download: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopShare0 > 1) {
				dif = pen.mStopShare0 - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Share: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopReg > 1) {
				dif = pen.mStopReg - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Register: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopOpchat > 1) {
				dif = pen.mStopOpchat - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Operator chat: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
					sep = true;
				}
			}

			if (pen.mStopKick > 1) {
				dif = pen.mStopKick - mServ->mTime.Sec();

				if (dif > 0) {
					if (sep)
						os << ", ";

					os << autosprintf(_("Kick: %s"), cTimePrint(dif).AsPeriod().AsString().c_str());
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
