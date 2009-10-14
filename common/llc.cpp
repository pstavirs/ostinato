#include <qendian.h>
#include <QHostAddress>

#include "llc.h"

LlcConfigForm::LlcConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

LlcProtocol::LlcProtocol(StreamBase *stream, AbstractProtocol *parent)
	: AbstractProtocol(stream, parent)
{
	configForm = NULL;
}

LlcProtocol::~LlcProtocol()
{
	delete configForm;
}

AbstractProtocol* LlcProtocol::createInstance(StreamBase *stream,
	AbstractProtocol *parent)
{
	return new LlcProtocol(stream, parent);
}

quint32 LlcProtocol::protocolNumber() const
{
	return OstProto::Protocol::kLlcFieldNumber;
}

void LlcProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
	protocol.MutableExtension(OstProto::llc)->CopyFrom(data);
	protocol.mutable_protocol_id()->set_id(protocolNumber());
}

void LlcProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
	if (protocol.protocol_id().id() == protocolNumber() &&
			protocol.HasExtension(OstProto::llc))
		data.MergeFrom(protocol.GetExtension(OstProto::llc));
}

QString LlcProtocol::name() const
{
	return QString("802.3 Logical Link Control");
}

QString LlcProtocol::shortName() const
{
	return QString("LLC");
}

int	LlcProtocol::fieldCount() const
{
	return llc_fieldCount;
}

QVariant LlcProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	quint32 id;
	quint8 dsap, ssap, ctl;

	id = payloadProtocolId(ProtocolIdLlc);
	dsap = (id >> 16) & 0xFF;
	ssap = (id >> 8) & 0xFF;
	ctl  = (id >> 0) & 0xFF;

	switch (index)
	{
		case llc_dsap:
			switch(attrib)
			{
				case FieldName:			
					return QString("DSAP");
				case FieldValue:
					return dsap;
				case FieldTextValue:
					return QString("%1").arg(dsap, 2, BASE_HEX, QChar('0'));
				case FieldFrameValue:
					return QByteArray(1, (char)(dsap));
				default:
					break;
			}
			break;
		case llc_ssap:
			switch(attrib)
			{
				case FieldName:			
					return QString("SSAP");
				case FieldValue:
					return ssap;
				case FieldTextValue:
					return QString("%1").arg(ssap, 2, BASE_HEX, QChar('0'));
				case FieldFrameValue:
					return QByteArray(1, (char)(ssap));
				default:
					break;
			}
			break;
		case llc_ctl:
			switch(attrib)
			{
				case FieldName:			
					return QString("Control");
				case FieldValue:
					return ctl;
				case FieldTextValue:
					return QString("%1").arg(ctl, 2, BASE_HEX, QChar('0'));
				case FieldFrameValue:
					return QByteArray(1, (char)(ctl));
				default:
					break;
			}
			break;

		default:
			break;
	}

	return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool LlcProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* LlcProtocol::configWidget()
{
	if (configForm == NULL)
	{
		configForm = new LlcConfigForm;
		loadConfigWidget();
	}
	return configForm;
}

void LlcProtocol::loadConfigWidget()
{
#define uintToHexStr(num, bytes)	\
	QString("%1").arg(num, bytes*2, BASE_HEX, QChar('0'))

	configWidget();

	configForm->leDsap->setText(uintToHexStr(
		fieldData(llc_dsap, FieldValue).toUInt(), 1));
	configForm->leSsap->setText(uintToHexStr(
		fieldData(llc_ssap, FieldValue).toUInt(), 1));
	configForm->leControl->setText(uintToHexStr(
		fieldData(llc_ctl, FieldValue).toUInt(), 1));
#undef uintToHexStr
}

void LlcProtocol::storeConfigWidget()
{
	bool isOk;

	configWidget();

	data.set_dsap(configForm->leDsap->text().toULong(&isOk, BASE_HEX));
	data.set_ssap(configForm->leSsap->text().toULong(&isOk, BASE_HEX));
	data.set_ctl(configForm->leControl->text().toULong(&isOk, BASE_HEX));
}

