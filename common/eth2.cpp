#include <qendian.h>
#include <QHostAddress>

#include "eth2.h"

Eth2ConfigForm *Eth2Protocol::configForm = NULL;

Eth2ConfigForm::Eth2ConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

Eth2Protocol::Eth2Protocol(Stream *parent)
	: AbstractProtocol(parent)
{
	if (configForm == NULL)
		configForm = new Eth2ConfigForm;
}

Eth2Protocol::~Eth2Protocol()
{
}

void Eth2Protocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::eth2)->CopyFrom(data);
}

void Eth2Protocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::eth2))
		data.MergeFrom(stream.GetExtension(OstProto::eth2));
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
			switch(attrib)
			{
				case FieldName:			
					return QString("Type");
				case FieldValue:
					return data.type();
				case FieldTextValue:
					return QString("%1").arg(data.type(), 16);
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.type(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

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
	return configForm;
}

void Eth2Protocol::loadConfigWidget()
{
#define uintToHexStr(num, bytesize) \
   	QString("%1").arg((num), (bytesize)*2 , 16, QChar('0'))

	configForm->leType->setText(uintToHexStr(data.type(), 2));

#undef uintToHexStr
}

void Eth2Protocol::storeConfigWidget()
{
	bool isOk;

	data.set_type(configForm->leType->text().remove(QChar(' ')).toULong(&isOk, 16));
}

