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

#ifndef NDIRECTCONNECTCBANLIST_H
#define NDIRECTCONNECTCBANLIST_H
#include "cconfmysql.h"
#include "cban.h"
#include "ckick.h"
#include <string>
#include <iostream>
#include "thasharray.h"

using std::string;
using std::ostream;

namespace nVerliHub {
	namespace nEnums {
		enum tTempBanType // temporary ban type
		{
			eBT_PASSW = 0, // incorrect password
			eBT_RECON = 1, // too fast reconnect
			eBT_FLOOD = 2, // protocol flood
			eBT_CLONE = 3 // clone detected
		};
	};

	namespace nSocket {
		class cConnDC;
	};

	namespace nTables {
		class cUnBanList;
		/// @addtogroup Core
		/// @{
		/**
		 * The banlist manager that is capable of:
		 * - keeping a list of ban
		 * - adding new ban
		 * - deleting existing ban
		 * - checking if IP address, nickname, share, etc. are banned
		 * Every ban entity is an instance of cBan class
		 * and it interacts with banlist table in a MySQL database.
		 *
		 * The class also keeps a list of temporary bans for IP address
		 * and nickname that is not stored in database.
		 * @author Daniel Muller
		 */
		class cBanList: public nConfig::cConfMySQL
		{
			friend class nVerliHub::nSocket::cServerDC;

			struct sTempBan // temporary ban structure
			{
				sTempBan(long until, const string &reason, unsigned bantype):
					mUntil(until),
					mReason(reason),
					mType(bantype)
				{}

				long mUntil; // expiration time
				string mReason; // reason
				unsigned mType; // type
			};

			public:
				/**
				 * Class constructor.
				 * @param server Pointer to a cServerDC instance.
				 */
				cBanList(nSocket::cServerDC*);

				/**
				 * Class destructor.
				 */

				~cBanList();

				/**
				 * Add a new ban to banlist table.
				 * If the ban already exists, the entry found is updated.
				 * @param ban A cBan instance.
				 */
				void AddBan(cBan &ban);

				/**
				 * Populate a stream to build a WHERE condition for a SELECT statement.
				 * @param os The stream where to write the condition.
				 * @param value The value to be matched.
				 * @param mask The bit-mask of ban type.
				 */
				bool AddTestCondition(ostream &os, const string &value, int mask);

				/**
				 * Delete ban entries that are 30 days old.
				 */
				virtual void Cleanup();

				/**
				 * Delete all ban entries matching the given conditions.
				 * Flags is a bit-mask of tBanFlags enumerations and it controls
				 * how to build the WHERE condition to delete ban entries:
				 * - tBanFlags::eBF_IP: match bans by IP address
				 * - tBanFlags::eBF_NICK: match bans by nickname
				 * Both conditions can be specified.
				 * @param ip The IP address.
				 * @param nick The nickname.
				 * @param mask The bit-mask for conditions.
				 * @return The number of deleted entries.
				 */
				int DeleteAllBansBy(const string &ip, const string &nick, int mask);

				/**
				 * Delete a given ban.
				 * @param ban An instance of cBan class to delete.
				 */
				void DelBan(cBan &ban);

				// add temporary nick or ip ban
				void AddNickTempBan(const string &nick, long until, const string &reason, unsigned bantype);
				void AddIPTempBan(const string &ip, long until, const string &reason, unsigned bantype);
				void AddIPTempBan(unsigned long ip, long until, const string &reason, unsigned bantype);

				// delete temporary nick or ip ban
				void DelNickTempBan(const string &nick);
				void DelIPTempBan(const string &ip);
				void DelIPTempBan(unsigned long ip);

				// show temporary nick or ip ban
				void ShowNickTempBan(ostream &os, const string &nick);
				void ShowIPTempBan(ostream &os, const string &ip);

				// check if nick or ip is temporarily banned
				bool IsNickTempBanned(const string &nick);
				bool IsIPTempBanned(const string &ip);
				bool IsIPTempBanned(unsigned long ip);

				// list of temporary nick and ip bans
				typedef tHashArray<sTempBan*> tTempNickIPBans;
				tTempNickIPBans mTempNickBanlist;
				tTempNickIPBans mTempIPBanlist;

				unsigned int GetTempNickListSize() const
				{
					return mTempNickBanlist.Size();
				}

				unsigned int GetTempNickListCapacity() const
				{
					return mTempNickBanlist.Capacity();
				}

				unsigned int GetTempIPListSize() const
				{
					return mTempIPBanlist.Size();
				}

				unsigned int GetTempIPListCapacity() const
				{
					return mTempIPBanlist.Capacity();
				}

				/**
				 * Extract the level domain substring from the given host.
				 * @param hostname The hostname.
				 * @param result The string where to store the result.
				 * @param level The level domain substring to extract.
				 * @return True if the substring can be extracted or false otherwise.
				 */
				bool GetHostSubstring(const string &hostname, string &dest, int level);

				// convert ipv4 address to number and validate, check against 0.0.0.0 is optional
				static bool Ip2Num(const string &ip, unsigned long &mask, bool zero = true);

				/**
				 * Fetch a list of ban entries sorted by ban time
				 * and output it to the given stream.
				 * @param os The destination stream.
				 * @param count The number of entries to select.
				 */
				void List(ostream &os, int count);

				/**
				 * Load a ban entry into the given cBan instance.
				 * The cBan instance must set the primary key of
				 * the entry in banlis table.
				 * @param ban cBan instance.
				 * @return True if the entry if found or false otherwise.
				 */
				//bool LoadBanByKey(cBan &ban);

				/**
				 * Fill the given cBan instance from the given
				 * user connection.
				 * If connection is valid, the cBan instance will contain the
				 * IP address, host, reason of the ban, the banner nickname,
				 * the nickname of the user and the start and end date of the ban.
				 * @param ban cBan instance where to store the result.
				 * @param connection The user connection.
				 * @param nick_op The banner nickname.
				 * @param reason The reason of the ban.
				 * @param length The time of the ban.
				 * @param mask The bit-mask for ban type.
				 * @see tBanFlags
				 */
				void NewBan(cBan &ban, nSocket::cConnDC *connection, const string &nickOp, const string &reason, unsigned length, unsigned mask);

				/**
				* Fill the given cBan instance from the given
				* cKick class instance.
				* The cBan instance will contain the IP address, host,
				* reason of the ban, the banner nickname, the nickname,
				* email address and share of the user and the start and
				* end date of the ban.
				* @param ban cBan instance where to store the result.
				* @param kick cKick instance.
				* @param length The time of the ban.
				* @param mask The bit-mask for ban type.
				* @see tBanFlags
				*/
				void NewBan(cBan &ban, const cKick &kick, long length, int mask);

				/**
				 * Convert an Internet network address to its Internet
				 * standard format (dotted string) representation.
				 * @param ip A proper address representation.
				 * @return Internet IP address as a string.
				 * @see Ip2Num()
				 */
				static void Num2Ip(unsigned long num, string &ip);

				/**
				 * Remove temporary ban entries for banned IP address
				 * and nickname.
				 * The search is done in both mTempNickBanlist and
				 * mTempIPBanlist lists.
				 * @param before Delete all ban entries that expires before this date.
				 * @return The number of removed entries.
				 */
				int RemoveOldShortTempBans(long before);

				/**
				 * Set unbanlist manager instance.
				 * @see cUnBanList
				 */
				void SetUnBanList(cUnBanList *UnBanList)
				{
					mUnBanList = UnBanList;
				}

				/**
				 * Found a ban entry with the given user connection
				 * and nickname.
				 * The WHERE condition to query a ban entry is build
				 * with AddTestCondition() call and it depends on the
				 * value of the given bit-mask.
				 * If the ban entry found is loaded into cBan instance.
				 * @param ban The cBan instance where to load the ban entry.
				 * @param connection The connection of the user.
				 * @param nickname The nickname.
				 * @param mask The bit-mask for conditions.
				 * @see tBanFlags
				 */
				unsigned int TestBan(cBan &, nSocket::cConnDC *conn, const string &nick, unsigned mask);

				/**
				 * Remove ban entries and create new unban entries for each found ban.
				 * Flags is a bit-mask of tBanFlags enumerations and it controls
				 * how to build the WHERE condition to delete the ban entries.
				 * See AddTestCondition() for more information.
				 * By default ban entries are deleted from banlist table unless you speficy
				 * the deleteEntry param.
				 * @param os The stream where to output the unban entries.
				 * @param value The value for WHERE condition.
				 * @param reason The unban reason.
				 * @param nickOp The unbanner nickname.
				 * @param mask The bit-mask for the condition.
				 * @param deleteEntry Set to true if you want to delete ban entries from banlist table
				 * @return The number of ban entries found
				 * @see AddTestCondition()
				 */
				int Unban(ostream &os, const string &value, const string &reason, const string &nickOp, int mask, bool deleteEntry = true);

				/**
				 * Syncronize the given cBan instance with the
				 * ban entry in banlis table.
				 * @param ban The cBan instance.
				 * @return Always zero.
				 */
				//int UpdateBan(cBan &);

			protected:
				/// cBan instance.
				/// This is the model of the table and
				/// the attribute is used to fetch new ban entry with LoadPK() call
				/// or add a new one with SavePK() call.
				/// @see SavePK()
				/// @see LoadPK()
				cBan mModel;
			private:
				/// Pointer to unbanlist manager.
				cUnBanList *mUnBanList;

				/// Pointer to cServerDC instance.
				nSocket::cServerDC* mS;

		};

		/**
		 * The unbanlist manager that has the same feature of banlist manager.
		 * The same table of cBanList class is used to store a list of unban entries.
		 * Every unban entity is an instance of cUnBan class
		 * and it interacts with banlist table in a MySQL database.
		 * @author Daniel Muller
		 */
		class cUnBanList : public cBanList
		{
			public:
				/**
				 * Class constructor.
				 * @param server Pointer to a cServerDC instance.
				 */
				cUnBanList(nSocket::cServerDC*);

				/**
				 * Class destructor.
				 */
				~cUnBanList();

				/**
				 * Delete unban entries that are 30 days old.
				 */
				virtual void Cleanup();
			protected:
				/// cUnBan instance.
				/// This is the model of the table and
				/// the attribute is used to fetch new unban entry with LoadPK() call
				/// or add a new one with SavePK() call.
				/// @see SavePK()
				/// @see LoadPK()
				cUnBan mModelUn;
		};
		/// @}
	}; // namesapce nTables
}; // namespace nVerliHub

#endif
