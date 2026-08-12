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
	nProtocol::cMessageADC *mMsg;
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
	nSocket::cConnADC *conn, nProtocol::cMessageADC *msg)
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

} // namespace nPlugin
} // namespace nVerliHub
