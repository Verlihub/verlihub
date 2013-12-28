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

#ifndef NDIRECTCONNECT_NPLUGINCVHPLUGINMGR_H
#define NDIRECTCONNECT_NPLUGINCVHPLUGINMGR_H
#include "cpluginmanager.h"
#include "ccallbacklist.h"
#include "cvhplugin.h"

namespace nVerliHub {
	namespace nSocket {
		class cServerDC;
	};

	class cUser;
	class cDCTag;
	class cUserCollection;

	namespace nProtocol {
		class cMessageDC;
	};

	/*
	* Verlihub's plugin namespace.
	* Contains base classes for plugin related structures specialized for Verlihub.
	*/
	namespace nPlugin {

/*
* Verlihub plugin manager
* @author Daniel Muller
*/

class cVHPluginMgr: public cPluginManager
{
	public:
		cVHPluginMgr(nSocket::cServerDC *, const string pluginDir);
		virtual ~cVHPluginMgr();
		virtual void OnPluginLoad(cPluginBase *pi);
	private:
		nSocket::cServerDC *mServer;
};

/*
* \brief Verlihub CallBackList Base class
*/

class cVHCBL_Base: public cCallBackList
{
	public:
		cVHCBL_Base(cVHPluginMgr *mgr, const char *id): cCallBackList(mgr, string(id)) {}
		// call one verlihub plugins callback
		virtual bool CallOne(cPluginBase *pi) {return CallOne((cVHPlugin*)pi);}
		// call a specific callback for verlihub plugin
		virtual bool CallOne(cVHPlugin *pi) = 0;
};

class cVHCBL_Simple: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpf0TypeFunc)();
	protected:
		tpf0TypeFunc m0TFunc;
	public:
		// constructor
		cVHCBL_Simple(cVHPluginMgr *mgr, const char *id, tpf0TypeFunc pFunc): cVHCBL_Base(mgr, id), m0TFunc(pFunc) {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*m0TFunc)();}
};

template <class Type1> class tVHCBL_1Type: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpf1TypeFunc)(Type1 *);
	protected:
		tpf1TypeFunc m1TFunc;
		Type1 *mData1;
	public:
		// constructor
		tVHCBL_1Type(cVHPluginMgr *mgr, const char *id, tpf1TypeFunc pFunc): cVHCBL_Base(mgr, id), m1TFunc(pFunc) {mData1 = NULL;}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*m1TFunc)(mData1);}

		virtual bool CallAll(Type1 *par1) {
			mData1 = par1;

			if (mData1 != NULL)
				return this->cCallBackList::CallAll();
			else
				return false;
		}
};

template <class Type1> class tVHCBL_R1Type: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpfR1TypeFunc)(Type1);
	protected:
		tpfR1TypeFunc mR1TFunc;
		Type1 mData1;
	public:
		// constructor
		tVHCBL_R1Type(cVHPluginMgr *mgr, const char *id, tpfR1TypeFunc pFunc): cVHCBL_Base(mgr, id), mR1TFunc(pFunc) {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*mR1TFunc)(mData1);}

		virtual bool CallAll(Type1 par1) {
			mData1 = par1;
			return this->cCallBackList::CallAll();
		}
};

template <class Type1, class Type2> class tVHCBL_2Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpf2TypesFunc)(Type1 *, Type2 *);
	protected:
		tpf2TypesFunc m2TFunc;
		Type1 *mData1;
		Type2 *mData2;
	public:
		// constructor
		tVHCBL_2Types(cVHPluginMgr *mgr, const char *id, tpf2TypesFunc pFunc): cVHCBL_Base(mgr, id), m2TFunc(pFunc) {mData1 = NULL; mData2 = NULL;}
		virtual ~tVHCBL_2Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*m2TFunc)(mData1, mData2);}

		virtual bool CallAll(Type1 *par1, Type2 *par2) {
			mData1 = par1;
			mData2 = par2;

			if ((mData1 != NULL) && (mData2 != NULL))
				return this->cCallBackList::CallAll();
			else
				return false;
		}
};

template <class Type1, class Type2> class tVHCBL_R2Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpfR2TypesFunc)(Type1, Type2);
	protected:
		tpfR2TypesFunc m2TFunc;
		Type1 mData1;
		Type2 mData2;
	public:
		// constructor
		tVHCBL_R2Types(cVHPluginMgr *mgr, const char *id, tpfR2TypesFunc pFunc): cVHCBL_Base(mgr, id), m2TFunc(pFunc) {}
		virtual ~tVHCBL_R2Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*m2TFunc)(mData1, mData2);}

		virtual bool CallAll(Type1 par1, Type2 par2) {
			mData1 = par1;
			mData2 = par2;
			return this->cCallBackList::CallAll();
		}
};

template <class Type1, class Type2, class Type3> class tVHCBL_3Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpf3TypesFunc)(Type1, Type2, Type3);
	protected:
		tpf3TypesFunc m3TFunc;
		Type1 mData1;
		Type2 mData2;
		Type3 mData3;
	public:
		// constructor
		tVHCBL_3Types(cVHPluginMgr *mgr, const char *id, tpf3TypesFunc pFunc): cVHCBL_Base(mgr, id), m3TFunc(pFunc) {}
		virtual ~tVHCBL_3Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*m3TFunc)(mData1, mData2, mData3);}

		virtual bool CallAll(Type1 par1, Type2 par2, Type3 par3) {
			mData1 = par1;
			mData2 = par2;
			mData3 = par3;
			return this->cCallBackList::CallAll();
		}
};

template <class Type1, class Type2, class Type3> class tVHCBL_R3Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpfR3TypesFunc)(Type1 *, Type2, Type3);
	protected:
		tpfR3TypesFunc mR3TFunc;
		Type1 *mData1;
		Type2 mData2;
		Type3 mData3;
	public:
		// constructor
		tVHCBL_R3Types(cVHPluginMgr *mgr, const char *id, tpfR3TypesFunc pFunc): cVHCBL_Base(mgr, id), mR3TFunc(pFunc) {mData1 = NULL;}
		virtual ~tVHCBL_R3Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*mR3TFunc)(mData1, mData2, mData3);}

		virtual bool CallAll(Type1 *par1, Type2 par2, Type3 par3) {
			mData1 = par1;
			mData2 = par2;
			mData3 = par3;

			if (mData1 != NULL)
				return this->cCallBackList::CallAll();
			else
				return false;
		}
};

template <class Type1, class Type2, class Type3, class Type4> class tVHCBL_4Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpf4TypesFunc)(Type1 *, Type2 *, Type3, Type4);
	protected:
		tpf4TypesFunc m4TFunc;
		Type1 *mData1;
		Type2 *mData2;
		Type3 mData3;
		Type4 mData4;
	public:
		// constructor
		tVHCBL_4Types(cVHPluginMgr *mgr, const char *id, tpf4TypesFunc pFunc): cVHCBL_Base(mgr, id), m4TFunc(pFunc) {mData1 = NULL; mData2 = NULL;}
		virtual ~tVHCBL_4Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*m4TFunc)(mData1, mData2, mData3, mData4);}

		virtual bool CallAll(Type1 *par1, Type2 *par2, Type3 par3, Type4 par4) {
			mData1 = par1;
			mData2 = par2;
			mData3 = par3;
			mData4 = par4;

			if ((mData1 != NULL) && (mData2 != NULL))
				return this->cCallBackList::CallAll();
			else
				return false;
		}
};

template <class Type1, class Type2, class Type3, class Type4> class tVHCBL_R4Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpfR4TypesFunc)(Type1 *, Type2, Type3, Type4);
	protected:
		tpfR4TypesFunc mR4TFunc;
		Type1 *mData1;
		Type2 mData2;
		Type3 mData3;
		Type4 mData4;
	public:
		// constructor
		tVHCBL_R4Types(cVHPluginMgr *mgr, const char *id, tpfR4TypesFunc pFunc): cVHCBL_Base(mgr, id), mR4TFunc(pFunc) {mData1 = NULL;}
		virtual ~tVHCBL_R4Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*mR4TFunc)(mData1, mData2, mData3, mData4);}

		virtual bool CallAll(Type1 *par1, Type2 par2, Type3 par3, Type4 par4) {
			mData1 = par1;
			mData2 = par2;
			mData3 = par3;
			mData4 = par4;

			if (mData1 != NULL)
				return this->cCallBackList::CallAll();
			else
				return false;
		}
};

template <class Type1, class Type2, class Type3, class Type4> class tVHCBL_X4Types: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpfX4TypesFunc)(Type1, Type2, Type3, Type4);
	protected:
		tpfX4TypesFunc mX4TFunc;
		Type1 mData1;
		Type2 mData2;
		Type3 mData3;
		Type4 mData4;
	public:
		// constructor
		tVHCBL_X4Types(cVHPluginMgr *mgr, const char *id, tpfX4TypesFunc pFunc): cVHCBL_Base(mgr, id), mX4TFunc(pFunc) {}
		virtual ~tVHCBL_X4Types() {}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*mX4TFunc)(mData1, mData2, mData3, mData4);}

		virtual bool CallAll(Type1 par1, Type2 par2, Type3 par3, Type4 par4) {
			mData1 = par1;
			mData2 = par2;
			mData3 = par3;
			mData4 = par4;
			return this->cCallBackList::CallAll();
		}
};

/*
* \brief Verlihub CallBackList with a single connection parameter
*/

class cVHCBL_Connection: public cVHCBL_Base
{
	public:
		typedef bool (cVHPlugin::*tpfConnFunc)(nSocket::cConnDC *);
	protected:
		tpfConnFunc mFunc;
		nSocket::cConnDC *mConn;
	public:
		// constructor
		cVHCBL_Connection(cVHPluginMgr *mgr, const char *id, tpfConnFunc pFunc): cVHCBL_Base(mgr, id), mFunc(pFunc) {mConn = NULL;}
		virtual bool CallOne(cVHPlugin *pi) {return (pi->*mFunc)(mConn);}

		virtual bool CallAll(nSocket::cConnDC *conn) {
			mConn = conn;

			if (mConn != NULL)
				return this->cCallBackList::CallAll();
			else
				return false;
		}
};

typedef tVHCBL_4Types<nSocket::cConnDC, std::string, int, int> cVHCBL_ConnTextIntInt;
typedef tVHCBL_R4Types<cUser, std::string, int, int> cVHCBL_UsrStrIntInt;
typedef tVHCBL_R4Types<cUser, std::string, std::string, std::string> cVHCBL_UsrStrStrStr;
typedef tVHCBL_X4Types<std::string, std::string, std::string, std::string> cVHCBL_StrStrStrStr;
typedef tVHCBL_3Types<cUser *, cUser *, std::string *> cVHCBL_UsrUsrStr;
typedef tVHCBL_3Types<std::string, std::string, std::string> cVHCBL_StrStrStr;
typedef tVHCBL_R3Types<cUser, std::string, int> cVHCBL_UsrStrInt;
typedef tVHCBL_2Types<nSocket::cConnDC, nProtocol::cMessageDC> cVHCBL_Message;
typedef tVHCBL_2Types<cUser, cUser> cVHCBL_UsrUsr;
typedef tVHCBL_2Types<nSocket::cConnDC, cDCTag> cVHCBL_ConnTag;
typedef tVHCBL_2Types<nSocket::cConnDC, std::string> cVHCBL_ConnText;
typedef tVHCBL_2Types<cUser, nTables::cBan> cVHCBL_UsrBan;
typedef tVHCBL_R2Types<std::string, std::string> cVHCBL_Strings;
typedef tVHCBL_1Type<string> cVHCBL_String;
typedef tVHCBL_1Type<cUser> cVHCBL_User;
typedef tVHCBL_R1Type<long> cVHCBL_Long;

	}; // namespace nPlugin
}; // namespace nVerliHub

#endif
