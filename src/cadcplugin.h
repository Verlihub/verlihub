#ifndef CADCPLUGIN_H
#define CADCPLUGIN_H

namespace nVerliHub {
namespace nSocket { class cConnADC; }
namespace nProtocol { class cMessageADC; }
namespace nPlugin {

class cADCPlugin
{
public:
	virtual ~cADCPlugin() {}
	virtual bool OnADCConnect(nSocket::cConnADC *conn) { return true; }
	virtual bool OnADCMessage(nSocket::cConnADC *conn, const nProtocol::cMessageADC *msg) { return true; }
	virtual bool OnADCDisconnect(nSocket::cConnADC *conn) { return true; }
	virtual bool OnADCTimer(long long msec) { return true; }
};

} // namespace nPlugin
} // namespace nVerliHub

#endif
