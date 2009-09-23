#ifndef _COMBO_PROTOCOL_H
#define _COMBO_PROTOCOL_H

#include "abstractprotocol.h"

template <class ProtoA, class ProtoB>
class ComboProtocol : public AbstractProtocol
{
private:
	ProtoA	*protoA;
	ProtoB	*protoB;
	QWidget *configForm;

public:
	ComboProtocol(StreamBase *stream)
	{
		protoA = new ProtoA(stream);
		protoB = new ProtoB(stream);
		configForm = NULL;
	}
	virtual ~ComboProtocol()
	{
		delete protoA;
		delete protoB;
		delete configForm;
	}

	static ComboProtocol* createInstance(StreamBase *stream)
	{
		return new ComboProtocol<ProtoA, ProtoB>(stream);
	}

	virtual quint32 protocolNumber() const
	{
		return 0; //FIXME
	}

	virtual void protoDataCopyInto(OstProto::Protocol &protocol) const
	{
		// FIXME
	}
	virtual void protoDataCopyFrom(const OstProto::Protocol &protocol)
	{
		// FIXME
	}

	virtual QString name() const
	{
		return protoA->name() + "/" + protoB->name();
	}
	virtual QString shortName() const
	{
		return protoA->shortName() + "/" + protoB->shortName();
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
		if (index < protoA->fieldCount())
			return protoA->fieldFlags(index);
		else
			return protoB->fieldFlags(index);
	}
	virtual QVariant fieldData(int index, FieldAttrib attrib,
		int streamIndex = 0) const
	{
		if (index < protoA->fieldCount())
			return protoA->fieldData(index);
		else
			return protoB->fieldData(index);
	}
	virtual bool setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib = FieldValue)
	{
		if (index < protoA->fieldCount())
			return protoA->fieldData(index);
		else
			return protoB->fieldData(index);
	}

#if 0
	QByteArray protocolFrameValue(int streamIndex = 0,
		bool forCksum = false) const;
	virtual int protocolFrameSize() const;
	int protocolFrameOffset() const;
	int protocolFramePayloadSize() const;

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
			QVBoxLayout	*layout = new VBoxLayout;

			configForm = new QWidget;
			layout->addWidget(protoA->configWidget());
			layout->addWidget(protoB->configWidget());
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
