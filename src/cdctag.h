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

#ifndef CDCTAG_H
#define CDCTAG_H
#include <string>
#include "cdcclient.h"
#include "cserverdc.h"

using namespace std;
/// VerliHub namespace that contains all classes.
namespace nVerliHub {
	namespace nEnums {
		/**
		 * Identify the mode used by a user as corresponding in client tag.
		 */
		typedef enum
		{
			/// The user sends no tag in $MyINFO to the hub
			eCM_NOTAG,
			/// The user sends a valid tag in $MyINFO and client is in active mode
			eCM_ACTIVE,
			/// The user sends a valid tag in $MyINFO and client is in passive mode
			eCM_PASSIVE,
			/// The user sends a valid tag in $MyINFO and client is connected
			/// to the hub through a proxy
			eCM_SOCK5,
			// user sends a valid tag in $MyINFO and client is connected in other unknown mode, this mode is marked as passive for now
			eCM_OTHER
		} tClientMode;

		/// Used to signal that an error occurred while validating the tag of the client.
		/// These error codes are returned in response to a cDCTag::ValidateTag() call.
		/// For example the user has too many open hubs or client version is not valid.
		enum {
			/// The client of the user is banned
			eTC_BANNED,
			/// The client of the user is unknown
			eTC_UNKNOWN,
			/// The tag cannot be parsed
			eTC_PARSE,
			/// User has too many open hubs
			eTC_MAX_HUB,
			// user has too few open hubs
			eTC_MIN_HUB,
			// user has too few open hubs as user
			eTC_MIN_HUB_USR,
			// user has too few open hubs as registered user
			eTC_MIN_HUB_REG,
			// user has too few open hubs as operator
			eTC_MIN_HUB_OP,
			/// User has too many open slots
			eTC_MAX_SLOTS,
			/// User has too few open slots
			eTC_MIN_SLOTS,
			// the ratio between open hubs and slots is too low
			eTC_MIN_HS_RATIO,
			// the ratio between open hubs and slots is too high
			eTC_MAX_HS_RATIO,
			/// The client is limiting the upload bandwidth
			/// and the limit is too low
			eTC_MIN_LIMIT,
			/// The upload limit per slot is too low
			eTC_MIN_LS_RATIO,
			/// The client version is not valid
			/// and too old
			eTC_MIN_VERSION,
			/// The client version is not valid
			/// and too recent
			eTC_MAX_VERSION,
			/// The client is connected to the hub through a
			/// proxy and it is not allowed
			eTC_SOCK5,
			/// The client is in passive mode
			/// but passive connections are restricted
			eTC_PASSIVE
		};
	}

	namespace nTables {
		class cDCConf;
		class cConnType;
	};
	using namespace nTables;
	/// @addtogroup Core
	/// @{
	/**
	 * @brief This class represents a tag of a client that is sent in $MyINFO protocol message..
	 * A tag looks like this <++ V:0.00,S:0,H:1>.
	 * Every token of the tag are parsed and st*ored inside
	 * an instance of this class. The parsing process is
	 * triggered by cDCTag::ValidateTag() call.
	 * @author Daniel Muller
	 * @author Davide Simoncelli
	 */
	class cDCTag
	{
		public:
			/**
			 * Class constructor.
			 * @param mS A pointer to cServerDC instance.
			 */
			cDCTag(nSocket::cServerDC *mS);

			/**
			 * Class constructor.
			 * @param mS A pointer to cServerDC instance.
			 * @param c A pointer to cDCClient instance.
			 */
			cDCTag(nSocket::cServerDC *mS, cDCClient *c);

			/**
			 * Class destructor.
			 */
			~cDCTag();

			/**
			 * Validate the tag of the client and identify it.
			 * @param os A stream that contains an error message after validation.
			 * @param conn_type The type of the connection of the user.
			 * @param code An integer that contains an error code after validation.
			 * @return The parsing result
			 */
			bool ValidateTag(ostream &os, cConnDC *conn, int &code);

			/**
			 * Friend << operator to output a cDCTag instance.
			 * @param os A stream that should contains the output.
			 * @param tag An instance of this clas.
			 */
			friend ostream &operator << (ostream&os, cDCTag &tag);

			/// Instance of cServerDC class that is used
			/// to access VerliHub configuration.
			nSocket::cServerDC *mServer;

			/// Instance of cDCClient class that describes
			/// the client of the user.
			cDCClient *client;

			/// The tag of the client.
			string mTag;

			/// The identification of the client inside the tag.
			string mTagID;

			/// The version of the client.
			double mClientVersion;

			/// The number of open hubs.
			int mTotHubs;

			// the number of open hubs as user
			int mHubsUsr;

			// the number of open hubs as registered user
			int mHubsReg;

			// the number of open hubs as operator
			int mHubsOp;

			/// The number of open slots.
			int mSlots;

			/// The upload bandwidth limit.
			int mLimit;

			/// The mode of the client.
			tClientMode mClientMode;

			/// The body of the tag (the string after the mode part)
			string mTagBody;
	};
	/// @}
}; // namespace nVerliHub
#endif
