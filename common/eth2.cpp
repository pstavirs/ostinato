#include <qendian.h>
#include <QHostAddress>

#include "eth2.h"

Eth2ConfigForm::Eth2ConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

Eth2Protocol::Eth2Protocol(StreamBase *stream)
	: AbstractProtocol(stream)
{
	configForm = NULL;
}

Eth2Protocol::~Eth2Protocol()
{
	delete configForm;
}

AbstractProtocol* Eth2Protocol::createInstance(StreamBase *stream)
{
	return new Eth2Protocol(stream);
}

quint32 Eth2Protocol::protocolNumber() const
{
	return OstProto::Protocol::kEth2FieldNumber;
}

void Eth2Protocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
	protocol.MutableExtension(OstProto::eth2)->CopyFrom(data);
	protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void Eth2Protocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
	if (protocol.protocol_id().id() == protocolNumber() &&
			protocol.HasExtension(OstProto::eth2))
		data.MergeFrom(protocol.GetExtension(OstProto::eth2));
}

QString Eth2Protocol::name() const
{
	return QString("Ethernet II");
}

QString Eth2Protocol::shortName() const
{
	return QString("Eth II");
}

int	Eth2Protocol::fieldCount() const
{
	return eth2_fieldCount;
}

QVariant Eth2Protocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case eth2_type:
		{
			quint16 type;
			switch(attrib)
			{
				case FieldName:			
					return QString("Type");
				case FieldValue:
					type = payloadProtocolId(ProtocolIdEth);
					return type;
				case FieldTextValue:
					type = payloadProtocolId(ProtocolIdEth);
					return QString("0x%1").arg(type, 4, BASE_HEX, QChar('0'));
				case FieldFrameValue:
				{
					QByteArray fv;
					type = payloadProtocolId(ProtocolIdEth);
					fv.resize(2);
					qToBigEndian((quint16) type, (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;
		}
		default:
			break;
	}

	return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool Eth2Protocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	bool isOk = false;

	if (attrib != FieldValue)
		return false;

	switch (index)
	{
		case eth2_type:
		{
			uint type = value.toUInt(&isOk);
			if (isOk)
				data.set_type(type);
		}
		default:
			break;
	}
	return isOk;
}

QWidget* Eth2Protocol::configWidget()
{
	if (configForm == NULL)
		configForm = new Eth2ConfigForm;
	return configForm;
}

void Eth2Protocol::loadConfigWidget()
{
	configWidget();

	configForm->leType->setText(uintToHexStr(
		fieldData(eth2_type, FieldValue).toUInt(), 2));
}

void Eth2Protocol::storeConfigWidget()
{
	bool isOk;

	configWidget();

	data.set_type(configForm->leType->text().remove(QChar(' ')).toULong(&isOk, 16));
}

