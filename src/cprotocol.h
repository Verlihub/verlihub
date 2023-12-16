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

#ifndef NSERVERCPROTOCOL_H
#define NSERVERCPROTOCOL_H
#include <vector>
#include "cobj.h"
#include "casyncconn.h" // added

using namespace std;
namespace nVerliHub {
	namespace nEnums {
		enum { eMSG_UNPARSED=-1 };
	};
 	namespace nSocket{
 		class cAsyncConn;
 	};

	namespace nProtocol {
class cMessageParser : public cObj
{
	public:
		cMessageParser(int);
		virtual ~cMessageParser();

		/** parses the string and sets the state variables */
		virtual int Parse() = 0; // must override
		/** splits message to it's important parts and stores their info in the chunkset mChunks */
		virtual bool SplitChunks() = 0; // must override

		/** reinitialize the structure */
		virtual void ReInit();
		/** return the n'th chunk (as splited by SplitChunks) function */
		virtual string &ChunkString(unsigned int n);

		/** apply the chunkstring ito the main string */
		void ApplyChunk(unsigned int n);
		/** get string */
		string & GetStr();


	public: // Public attributes
		/** The actual message string */
		string mStr;

		/** the type for message chunks */
		typedef pair<int,int> tChunk;
		typedef vector<tChunk> tChunkList;
		typedef tChunkList::iterator tCLIt;
		typedef vector<string *> tStrPtrList;
		//typedef tStrPtrList::iterator tSPLIt;
		/** the list of chunks */
		tChunkList mChunks;
		/** list of string pointers */
		//tStrPtrList mChStrings;
		string *mChStrings;
		/** a bitmap having information about chStringsSet */
		unsigned long mChStrMap;

		/** indicates if the message is tooo long so it can't be receved complete */
		//bool Overfill;
		/** indicates if the message is completely received */
		//bool Received;
		/** error in message indicator */
		bool mError;

		/*
			specifies that message has been modified
			and split chunks should be called again
			to avoid broken protocol messages
		*/
		bool mModified;

		// parsed message type
		int mType;
		// length of the message and base
		size_t mLen;
		size_t mKWSize;

	protected:
		bool SplitOnTwo(size_t start, const string &lim, int cn1, int cn2, size_t len = 0, bool left = true);
		bool SplitOnTwo(const string &lim, int ch, int cn1, int cn2, bool left = true);
		bool SplitOnTwo(size_t start, const char lim, int cn1, int cn2, size_t len = 0, bool left = true);
		bool SplitOnTwo(const char lim, int ch, int cn1, int cn2, bool left = true);
		bool ChunkRedLeft(int cn, int amount);
		bool ChunkRedRight(int cn, int amount);
		bool ChunkIncLeft(int cn, int amount);
		void SetChunk(int n, int start, int len);

		// maximum number of chunks
		int mMaxChunks;
};

/**
 a B*ase class for protocols

 @author Daniel Muller
 */
class cProtocol : public cObj
{
public:
	cProtocol();
	virtual ~cProtocol();
	virtual int TreatMsg(cMessageParser * msg, nSocket::cAsyncConn *conn) = 0;
	virtual cMessageParser *CreateParser() = 0;
	virtual void DeleteParser(cMessageParser *) = 0;
};
	}; // namespace nProtocol
}; // namespace nVerliHub

#endif
