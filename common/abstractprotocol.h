#ifndef _ABSTRACT_PROTOCOL_H
#define _ABSTRACT_PROTOCOL_H

#include <QString>
#include <QVariant>
#include <QByteArray>
#include <QWidget>
#include <QLinkedList>
#include <QFlags>

//#include "../rpc/pbhelper.h"
#include "protocol.pb.h"

#define BASE_BIN (2)
#define BASE_OCT (8)
#define BASE_DEC (10)
#define BASE_HEX (16)

#define uintToHexStr(num, bytes)	\
	QString("%1").arg(num, bytes*2, BASE_HEX, QChar('0'))

class StreamBase;
class ProtocolListIterator;

class AbstractProtocol
{
	template <int protoNumber, class ProtoA, class ProtoB> 
		friend class ComboProtocol;
	friend class ProtocolListIterator;

private:
	mutable int		metaCount;
	mutable int		protoSize;
	mutable QString protoAbbr;

protected:
	StreamBase			*mpStream;
	AbstractProtocol	*parent;
	AbstractProtocol	*prev;
	AbstractProtocol	*next;

public:
	enum FieldFlag {
		FieldIsNormal = 0x0,
		FieldIsMeta   = 0x1,
		FieldIsCksum  = 0x2
	};
	Q_DECLARE_FLAGS(FieldFlags, FieldFlag);

	enum FieldAttrib {
		FieldName,			//! name
		FieldValue,			//! value in host byte order (user editable)
		FieldTextValue,		//! value as text
		FieldFrameValue,	//! frame encoded value in network byte order
		FieldBitSize,		//! size in bits
	};

	enum ProtocolIdType {
		ProtocolIdLlc,
		ProtocolIdEth,
		ProtocolIdIp,
	};

	enum CksumType {
		CksumIp,
		CksumIpPseudo,
		CksumTcpUdp,

		CksumMax
	};

	AbstractProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
	virtual ~AbstractProtocol();

	static AbstractProtocol* createInstance(StreamBase *stream,
		AbstractProtocol *parent = 0);
	virtual quint32 protocolNumber() const;

	virtual void protoDataCopyInto(OstProto::Protocol &protocol) const = 0;
	virtual void protoDataCopyFrom(const OstProto::Protocol &protocol) = 0;

	virtual QString name() const;
	virtual QString shortName() const;

	virtual quint32 protocolId(ProtocolIdType type) const;
	quint32 payloadProtocolId(ProtocolIdType type) const;

	virtual int	fieldCount() const;
	virtual int	metaFieldCount() const;
	int	frameFieldCount() const;

	virtual FieldFlags fieldFlags(int index) const;
	virtual QVariant fieldData(int index, FieldAttrib attrib,
		int streamIndex = 0) const;
	virtual bool setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib = FieldValue);

	QByteArray protocolFrameValue(int streamIndex = 0,
		bool forCksum = false) const;
	virtual int protocolFrameSize(int streamIndex = 0) const;
	int protocolFrameOffset(int streamIndex = 0) const;
	int protocolFramePayloadSize(int streamIndex = 0) const;

	virtual quint32 protocolFrameCksum(int streamIndex = 0,
		CksumType cksumType = CksumIp) const;
	quint32 protocolFrameHeaderCksum(int streamIndex = 0,
		CksumType cksumType = CksumIp) const;
	quint32 protocolFramePayloadCksum(int streamIndex = 0,
		CksumType cksumType = CksumIp) const;

	virtual QWidget* configWidget() = 0;
	virtual void loadConfigWidget() = 0;
	virtual void storeConfigWidget() = 0;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AbstractProtocol::FieldFlags);

#endif
