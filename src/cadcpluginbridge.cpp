#include "cadcpluginbridge.h"
#include "cadcplugin.h"
#include "cpluginbase.h"
#include "cpluginmanager.h"

namespace nVerliHub {
namespace nPlugin {

namespace {

struct sADCConnectCall
{
	nSocket::cConnADC *mConn;
	bool mAllowed;
};

struct sADCMessageCall
{
	nSocket::cConnADC *mConn;
	const nProtocol::cMessageADC *mMsg;
	bool mAllowed;
};

struct sADCTimerCall
{
	long long mMsec;
	bool mAllowed;
};

bool VisitADCConnect(cPluginBase *base, void *data)
{
	sADCConnectCall *call = static_cast<sADCConnectCall*>(data);
	cADCPlugin *plugin = dynamic_cast<cADCPlugin*>(base);

	if (!plugin)
		return true;

	if (!plugin->OnADCConnect(call->mConn)) {
		call->mAllowed = false;
		return false;
	}

	return true;
}

bool VisitADCMessage(cPluginBase *base, void *data)
{
	sADCMessageCall *call = static_cast<sADCMessageCall*>(data);
	cADCPlugin *plugin = dynamic_cast<cADCPlugin*>(base);

	if (!plugin)
		return true;

	if (!plugin->OnADCMessage(call->mConn, call->mMsg)) {
		call->mAllowed = false;
		return false;
	}

	return true;
}

bool VisitADCDisconnect(cPluginBase *base, void *data)
{
	cADCPlugin *plugin = dynamic_cast<cADCPlugin*>(base);

	if (plugin)
		plugin->OnADCDisconnect(static_cast<nSocket::cConnADC*>(data));

	return true;
}

bool VisitADCTimer(cPluginBase *base, void *data)
{
	sADCTimerCall *call = static_cast<sADCTimerCall*>(data);
	cADCPlugin *plugin = dynamic_cast<cADCPlugin*>(base);

	if (!plugin)
		return true;

	if (!plugin->OnADCTimer(call->mMsec)) {
		call->mAllowed = false;
		return false;
	}

	return true;
}

} // namespace

bool cADCPluginBridge::OnConnect(cPluginManager *manager,
	nSocket::cConnADC *conn)
{
	if (!manager || !conn)
		return true;

	sADCConnectCall call = {conn, true};
	manager->ForEachPlugin(&VisitADCConnect, &call);
	return call.mAllowed;
}

bool cADCPluginBridge::OnMessage(cPluginManager *manager,
	nSocket::cConnADC *conn, const nProtocol::cMessageADC *msg)
{
	if (!manager || !conn || !msg)
		return true;

	sADCMessageCall call = {conn, msg, true};
	manager->ForEachPlugin(&VisitADCMessage, &call);
	return call.mAllowed;
}

void cADCPluginBridge::OnDisconnect(cPluginManager *manager,
	nSocket::cConnADC *conn)
{
	if (manager && conn)
		manager->ForEachPlugin(&VisitADCDisconnect, conn);
}

bool cADCPluginBridge::OnTimer(cPluginManager *manager, long long msec)
{
	if (!manager)
		return true;

	sADCTimerCall call = {msec, true};
	manager->ForEachPlugin(&VisitADCTimer, &call);
	return call.mAllowed;
}

} // namespace nPlugin
} // namespace nVerliHub
