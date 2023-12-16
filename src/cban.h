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

#ifndef NDIRECTCONNECTCBAN_H
#define NDIRECTCONNECTCBAN_H

#include "cobj.h"

namespace nVerliHub {
	namespace nEnums {
		/**
		 * Type of the ban.
		 */
		enum tBanFlags {
			/// Nick and IP address are banned.
			eBF_NICKIP = 1 << 0,
			/// IP address is banned.
			eBF_IP  = 1 << 1,
			/// Nick is banned.
			eBF_NICK  = 1 << 2,
			/// Range address is banned.
			eBF_RANGE = 1 << 3,
			/// Host of level 1 is banned.
			eBF_HOST1 = 1 << 4,
			/// Host of level 2 is banned.
			eBF_HOST2 = 1 << 5,
			/// Host of level 3 is banned.
			eBF_HOST3 = 1 << 6,
			/// Share is banned.
			eBF_SHARE = 1 << 7,
			/// Prefix of nick is banned
			eBF_PREFIX = 1 << 8,
			/// Reverse host is banned
			eBF_HOSTR1 = 1 << 9,
			eBF_LAST = 1 << 10 // last item, used for iteration only
		};
	};

	namespace nSocket {
		class cServerDC;
	};

	namespace nTables {
		/// @addtogroup Core
		/// @{
		/**
		* This class represents a ban that is stored into banlist table.
		* It provides basic methods to display the ban.
		*
		* @author Daniel Muller
		*/
		class cBan : public cObj
		{
			public:

				/**
				* Class constructor.
				* @param server Pointer to a cServerDC instance.
				*/
				cBan(class nSocket::cServerDC *);

				/**
				* Class destructor.
				*/
				~cBan();

				/**
				* Display full information about the ban.
				* @param os The output stream.
				*/
				virtual void DisplayComplete(ostream &os);

				/**
				* Display kick information.
				* @param os The output stream.
				*/
				virtual void DisplayKick(ostream &os);

				/**
				* Display ban information into a single line.
				* @param os The output stream.
				*/
				void DisplayInline(ostream &os);

				/**
				* Display ban information about the user.
				* @param os The output stream.
				*/
				virtual void DisplayUser(ostream &);

				/**
				* Return a string describing the type of the ban.
				* @return The type of the ban.
				*/
				const char *GetBanType();

				/**
				* Set the type of the ban.
				* @param type Ban type.
				*/
				void SetType(unsigned type);

				/**
				* Write ban information to the output stream.
				*/
				friend ostream & operator << (ostream &, cBan &);

				/// Banned IP address.
				string mIP;

				/// Banned nick.
				string mNick;

				/// Banned host name.
				string mHost;

				/// Banned share size.
				unsigned __int64 mShare;

				/// Lowest IP in banned IP range.
				unsigned long mRangeMin;

				/// Highest IP in banned IP range.
				unsigned long mRangeMax;

				/// Time of the ban in Unix time format.
				long mDateStart;

				/// End of the ban in Unix time format.
				long mDateEnd;
				long mLastHit; // last ban hit in unix time format

				/// Ban type.
				unsigned mType;

				/// Operator who banned an user.
				string mNickOp;

				/// Ban reason.
				string mReason;

				// additional notes
				string mNoteOp;
				string mNoteUsr;

				/// How ban should be displayed to output stream.
				//int mDisplayType;

				/// Pointer to a cServerDC instance.
				nSocket::cServerDC *mS;
		};

		/**
		* This class represents an unban that is stored into unbanlist table.
		* It provides like cBan basic methods to display the unban.
		*
		* @author Daniel Muller
		*/
		class cUnBan : public cBan
		{
			public:
				/**
				* Class constructor.
				* @param ban An instance of cBan class.
				* @param server Pointer to a cServerDC instance.
				*/
				cUnBan(cBan &ban, nSocket::cServerDC *server);

				/**
				* Class constructor.
				* @param server Pointer to a cServerDC instance.
				*/
				cUnBan(nSocket::cServerDC *server);

				/**
				* Class destructor.
				*/
				~cUnBan();

				/**
				* Display full information about the unban.
				* @param os The output stream.
				*/
				//virtual void DisplayComplete(ostream &os);

				/**
				* Display unban information about the user.
				* @param os The output stream.
				*/
				//virtual void DisplayUser(ostream &);

				/// Time of the unban in Unix time format
				long mDateUnban;

				/// Operator who unbanned an user
				string mUnNickOp;

				/// Unban reason
				string mUnReason;
		};
		/// @}

	}; // namespace nTables
}; // namespace nVerliHub

#endif
