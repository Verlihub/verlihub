#ifndef CADCPLUGINBRIDGE_H
#define CADCPLUGINBRIDGE_H

namespace nVerliHub {
namespace nSocket { class cConnADC; }
namespace nProtocol { class cMessageADC; struct sADCSession; }
namespace nPlugin {
class cPluginManager;

class cADCPluginBridge
{
public:
	static bool OnConnect(cPluginManager *manager, nSocket::cConnADC *conn);
	static bool OnMessage(cPluginManager *manager, nSocket::cConnADC *conn,
		const nProtocol::cMessageADC *msg);
	static void OnDisconnect(cPluginManager *manager, nSocket::cConnADC *conn);
	static void OnLogin(cPluginManager *manager, nSocket::cConnADC *conn,
		const nProtocol::sADCSession *session);
	static void OnLogout(cPluginManager *manager, nSocket::cConnADC *conn,
		const nProtocol::sADCSession *session);
	static bool OnTimer(cPluginManager *manager, long long msec);
};

} // namespace nPlugin
} // namespace nVerliHub

#endif
