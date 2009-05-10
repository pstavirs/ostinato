#include <qendian.h>
#include <QHostAddress>

#include "snap.h"

SnapConfigForm *SnapProtocol::configForm = NULL;

SnapConfigForm::SnapConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

SnapProtocol::SnapProtocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
	: AbstractProtocol(frameProtoList, parent)
{
	if (configForm == NULL)
		configForm = new SnapConfigForm;
}

SnapProtocol::~SnapProtocol()
{
}

AbstractProtocol* SnapProtocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return new SnapProtocol(frameProtoList, streamCore);
}

void SnapProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::snap)->CopyFrom(data);
}

void SnapProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::snap))
		data.MergeFrom(stream.GetExtension(OstProto::snap));
}

QString SnapProtocol::name() const
{
	return QString("SubNetwork Access Protocol");
}

QString SnapProtocol::shortName() const
{
	return QString("SNAP");
}

quint32 SnapProtocol::protocolId(ProtocolIdType type) const
{
	switch(type)
	{
		case ProtocolIdLlc: return 0xAAAA03;
		default: break;
	}

	return AbstractProtocol::protocolId(type);
}

int	SnapProtocol::fieldCount() const
{
	return snap_fieldCount;
}

QVariant SnapProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case snap_oui:
			switch(attrib)
			{
				case FieldName:			
					return QString("OUI");
				case FieldValue:
					return data.oui();
				case FieldTextValue:
					return QString("%1").arg(data.oui(), 6, BASE_HEX, QChar('0'));
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(4);
					qToBigEndian((quint32) data.oui(), (uchar*) fv.data());
					fv.remove(0, 1);
					return fv;
				}
				default:
					break;
			}
			break;
		case snap_type:
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
					return QString("%1").arg(type, 4, BASE_HEX, QChar('0'));
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					type = payloadProtocolId(ProtocolIdEth);
					qToBigEndian(type, (uchar*) fv.data());
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

bool SnapProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* SnapProtocol::configWidget()
{
	return configForm;
}

void SnapProtocol::loadConfigWidget()
{
	configForm->leOui->setText(uintToHexStr(
		fieldData(snap_oui, FieldValue).toUInt(), 3));
	configForm->leType->setText(uintToHexStr(
		fieldData(snap_type, FieldValue).toUInt(), 2));
}

void SnapProtocol::storeConfigWidget()
{
	bool isOk;

	data.set_oui(configForm->leOui->text().toULong(&isOk, BASE_HEX));
	data.set_type(configForm->leType->text().toULong(&isOk, BASE_HEX));
}

