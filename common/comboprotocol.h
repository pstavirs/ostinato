#ifndef _COMBO_PROTOCOL_H
#define _COMBO_PROTOCOL_H

#include "abstractprotocol.h"

template <int protoNumber, class ProtoA, class ProtoB>
class ComboProtocol : public AbstractProtocol
{
private:
	ProtoA	*protoA;
	ProtoB	*protoB;
	QWidget *configForm;

public:
	ComboProtocol(StreamBase *stream, AbstractProtocol *parent = 0)
		: AbstractProtocol(stream, parent)
	{
		protoA = new ProtoA(stream, this);
		protoB = new ProtoB(stream, this);
		protoA->next = protoB;
		protoB->prev = protoA;
		configForm = NULL;

		qDebug("%s: protoNumber = %d, %p <--> %p", __FUNCTION__,
			protoNumber, protoA, protoB);
	}

	virtual ~ComboProtocol()
	{
		if (configForm)
		{
			protoA->configWidget()->setParent(0);
			protoB->configWidget()->setParent(0);
			delete configForm;
		}
		delete protoA;
		delete protoB;
	}

	static ComboProtocol* createInstance(StreamBase *stream,
		AbstractProtocol *parent)
	{
		return new ComboProtocol<protoNumber, ProtoA, ProtoB>(stream, parent);
	}

	virtual quint32 protocolNumber() const
	{
		return protoNumber;
	}

	virtual void protoDataCopyInto(OstProto::Protocol &protocol) const
	{
		protoA->protoDataCopyInto(protocol);
		protoB->protoDataCopyInto(protocol);
		protocol.mutable_protocol_id()->set_id(protocolNumber());
	}

	virtual void protoDataCopyFrom(const OstProto::Protocol &protocol)
	{
		if (protocol.protocol_id().id() == protocolNumber())
		{
			OstProto::Protocol	proto;

			// NOTE: To use protoX->protoDataCopyFrom() we need to arrange
			// so that it sees its own protocolNumber() - but since the
			// input param 'protocol' is 'const', we make a copy first

			proto.CopyFrom(protocol);

			proto.mutable_protocol_id()->set_id(protoA->protocolNumber());
			protoA->protoDataCopyFrom(proto);

			proto.mutable_protocol_id()->set_id(protoB->protocolNumber());
			protoB->protoDataCopyFrom(proto);
		}
	}

	virtual QString name() const
	{
		return protoA->name() + "/" + protoB->name();
	}
	virtual QString shortName() const
	{
		return protoA->shortName() + "/" + protoB->shortName();
	}

	virtual ProtocolIdType protocolIdType() const
	{
		return protoB->protocolIdType();
	}

	virtual quint32 protocolId(ProtocolIdType type) const
	{
		return protoA->protocolId(type);
	}
	//quint32 payloadProtocolId(ProtocolIdType type) const;

	virtual int	fieldCount() const
	{
		return protoA->fieldCount() + protoB->fieldCount();
	}
	//virtual int	metaFieldCount() const;
	//int	frameFieldCount() const;

	virtual FieldFlags fieldFlags(int index) const
	{
		int cnt = protoA->fieldCount();

		if (index < cnt)
			return protoA->fieldFlags(index);
		else
			return protoB->fieldFlags(index - cnt);
	}
	virtual QVariant fieldData(int index, FieldAttrib attrib,
		int streamIndex = 0) const
	{
		int cnt = protoA->fieldCount();

		if (index < cnt)
			return protoA->fieldData(index, attrib, streamIndex);
		else
			return protoB->fieldData(index - cnt, attrib, streamIndex);
	}
	virtual bool setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib = FieldValue)
	{
		int cnt = protoA->fieldCount();

		if (index < cnt)
			return protoA->setFieldData(index, value, attrib);
		else
			return protoB->setFieldData(index - cnt, value, attrib);
	}

#if 0
	QByteArray protocolFrameValue(int streamIndex = 0,
		bool forCksum = false) const;
	virtual int protocolFrameSize() const;
	int protocolFrameOffset() const;
	int protocolFramePayloadSize() const;
#endif

	virtual bool isProtocolFrameValueVariable() const
	{
		return (protoA->isProtocolFrameValueVariable()
			|| protoB->isProtocolFrameValueVariable());
	}

	virtual bool isProtocolFrameSizeVariable() const
	{
		return (protoA->isProtocolFrameSizeVariable()
			|| protoB->isProtocolFrameSizeVariable());
	}

#if 0
	virtual quint32 protocolFrameCksum(int streamIndex = 0,
		CksumType cksumType = CksumIp) const;
	quint32 protocolFrameHeaderCksum(int streamIndex = 0,
		CksumType cksumType = CksumIp) const;
	quint32 protocolFramePayloadCksum(int streamIndex = 0,
		CksumType cksumType = CksumIp) const;
#endif

	virtual QWidget* configWidget()
	{
		if (configForm == NULL)
		{
			QVBoxLayout	*layout = new QVBoxLayout;

			configForm = new QWidget;
			layout->addWidget(protoA->configWidget());
			layout->addWidget(protoB->configWidget());
			layout->setSpacing(0);
			layout->setContentsMargins(0, 0, 0, 0);
			configForm->setLayout(layout);
		}
		return configForm;
	}
	virtual void loadConfigWidget()
	{
		protoA->loadConfigWidget();
		protoB->loadConfigWidget();
	}
	virtual void storeConfigWidget()
	{
		protoA->storeConfigWidget();
		protoB->storeConfigWidget();
	}
};

#endif
