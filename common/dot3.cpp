#include <qendian.h>
#include <QHostAddress>

#include "Dot3.h"

#define SZ_FCS		4

Dot3ConfigForm *Dot3Protocol::configForm = NULL;

Dot3ConfigForm::Dot3ConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

Dot3Protocol::Dot3Protocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
	: AbstractProtocol(frameProtoList, parent)
{
	if (configForm == NULL)
		configForm = new Dot3ConfigForm;
}

Dot3Protocol::~Dot3Protocol()
{
}

AbstractProtocol* Dot3Protocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return new Dot3Protocol(frameProtoList, streamCore);
}

void Dot3Protocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::dot3)->CopyFrom(data);
}

void Dot3Protocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::dot3))
		data.MergeFrom(stream.GetExtension(OstProto::dot3));
}

QString Dot3Protocol::name() const
{
	return QString("802.3");
}

QString Dot3Protocol::shortName() const
{
	return QString("802.3");
}

int	Dot3Protocol::fieldCount() const
{
	return dot3_fieldCount;
}

QVariant Dot3Protocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case dot3_length:
			switch(attrib)
			{
				case FieldName:			
					return QString("Length");
				case FieldValue:
				{
					quint16 len;

					len = stream->frame_len() - SZ_FCS;
					return len;
				}
				case FieldTextValue:
				{
					quint16 len;

					len = stream->frame_len() - SZ_FCS;
					return QString("%1").arg(len);
				}
				case FieldFrameValue:
				{
					quint16 len;
					QByteArray fv;

					len = stream->frame_len() - SZ_FCS;
					fv.resize(2);
					qToBigEndian(len, (uchar*) fv.data());
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

bool Dot3Protocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* Dot3Protocol::configWidget()
{
	return configForm;
}

void Dot3Protocol::loadConfigWidget()
{
	configForm->leLength->setText(
		fieldData(dot3_length, FieldValue).toString());
}

void Dot3Protocol::storeConfigWidget()
{
	bool isOk;

	data.set_length(configForm->leLength->text().toULong(&isOk));
}

