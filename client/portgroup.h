#ifndef _PORT_GROUP_H
#define _PORT_GROUP_H

#include "port.h"
#include <QHostAddress>
#include <QTcpSocket>

/* TODO
HIGH
MED
LOW
- Allow hostnames in addition to IP Address as "server address"
*/

#define DEFAULT_SERVER_PORT		7878

class PortGroup : public QObject {
	Q_OBJECT

private:
	quint32			mPortGroupId;
	static quint32	mPortGroupAllocId;
	QString			mUserAlias;			// user defined

	QTcpSocket		*mpSocket;
	QHostAddress	mServerAddress;
	quint16			mServerPort;
public: // FIXME(HIGH): member access
	QList<Port>		mPorts;

public:
	PortGroup(QHostAddress ip = QHostAddress::LocalHost, 
		quint16 port = DEFAULT_SERVER_PORT); 
	~PortGroup();

	void connectToHost(); 
	void connectToHost(QHostAddress ip, quint16 port); 
	void disconnectFromHost();

	int numPorts() const { return mPorts.size(); }
	quint32 id() const { return mPortGroupId; } 
	const QHostAddress& serverAddress() const { return mServerAddress; } 
	quint16 serverPort() const { return mServerPort; } 
	const QString& userAlias() const { return mUserAlias; } 
	QAbstractSocket::SocketState state() const { return mpSocket->state(); }	

	void setUserAlias(QString alias) { mUserAlias = alias; };

signals:
	void portGroupDataChanged(PortGroup* portGroup);
	void portListAboutToBeChanged(quint32 portGroupId);
	void portListChanged(quint32 portGroupId);

private slots:
	void on_mpSocket_stateChanged();
	void when_connected();
	void when_disconnected();
	void when_error(QAbstractSocket::SocketError socketError);
	void when_dataAvail();

private:
	void ProcessCapabilityInfo(const char *msg, qint32 size);
	void ProcessMsg(const char *msg, quint32 size);
};

#endif
