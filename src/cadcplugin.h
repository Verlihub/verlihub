#ifndef CADCPLUGIN_H
#define CADCPLUGIN_H

namespace nVerliHub {
namespace nSocket { class cConnADC; }
namespace nProtocol { class cMessageADC; struct sADCSession; }
namespace nPlugin {

class cADCPlugin
{
public:
	virtual ~cADCPlugin() {}
	virtual bool OnADCConnect(nSocket::cConnADC *conn) { return true; }
	virtual bool OnADCMessage(nSocket::cConnADC *conn, const nProtocol::cMessageADC *msg) { return true; }
	virtual bool OnADCDisconnect(nSocket::cConnADC *conn) { return true; }
	virtual void OnADCLogin(nSocket::cConnADC *conn, const nProtocol::sADCSession *session) {}
	virtual void OnADCLogout(nSocket::cConnADC *conn, const nProtocol::sADCSession *session) {}
	virtual bool OnADCTimer(long long msec) { return true; }
};

} // namespace nPlugin
} // namespace nVerliHub

#endif
