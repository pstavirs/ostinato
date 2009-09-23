#include <qendian.h>
#include <QHostAddress>

#include "sample.h"

SampleConfigForm::SampleConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

SampleProtocol::SampleProtocol(StreamBase *stream);
	: AbstractProtocol(stream)
{
	configForm = NULL;
}

SampleProtocol::~SampleProtocol()
{
	delete configForm;
}

AbstractProtocol* SampleProtocol::createInstance(StreamBase *stream)
{
	return new SampleProtocol(frameProtoList, streamCore);
}

quint32 SampleProtocol::protocolNumber() const
{
	return OstProto::Protocol::kSampleFieldNumber;
}

void SampleProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const
{
	protocol.MutableExtension(OstProto::sample)->CopyFrom(data);
	protocol.mutable_protocol_id()->set_id(protocolNumber())
}

void SampleProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol)
{
	if (protocol.protocol_id()->id() == protocolNumber() &&
			protocol.HasExtension(OstProto::sample))
		data.MergeFrom(protocol.GetExtension(OstProto::sample));
}

QString SampleProtocol::name() const
{
	return QString("Sample");
}

QString SampleProtocol::shortName() const
{
	return QString("Sample");
}

int	SampleProtocol::fieldCount() const
{
	return sample_fieldCount;
}

AbstractProtocol::FieldFlags SampleProtocol::fieldFlags(int index) const
{
	AbstractProtocol::FieldFlags flags;

	flags = AbstractProtocol::fieldFlags(index);

	switch (index)
	{
		case sample_normal:
			break;

		case sample_cksum:
			flags |= FieldIsCksum;
			break;

		case sample_meta:
			flags |= FieldIsMeta;
			break;

		default:
			break;
	}

	return flags;
}

QVariant SampleProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case sample_FIXME:
		{
			switch(attrib)
			{
				case FieldName:			
					return QString("FIXME");
				case FieldValue:
					return data.FIXME();
				case FieldTextValue:
					return QString("%1").arg(data.FIXME());
				case FieldFrameValue:
					return QByteArray(1, (char)(data.FIXME() & 0xF0));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;

		}
		case sample_FIXME:
		{
			switch(attrib)
			{
				case FieldName:			
					return QString("FIXME");
				case FieldValue:
					return FIXME;
				case FieldTextValue:
					return QString("%1").arg(FIXME);
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(0);
					qToBigEndian(FIXME, (uchar*) fv.data()); 
					return fv;
				}
					return QByteArray(1, (char)(FIXME() & 0xF0));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;
		}
		// Meta fields

		case sample_FIXME:
		default:
			break;
	}

	return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool SampleProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* SampleProtocol::configWidget()
{
	if (configForm == NULL)
	{
		configForm = new SampleConfigForm;
		loadConfigWidget();
	}

	return configForm;
}

void SampleProtocol::loadConfigWidget()
{
	configWidget();
}

void SampleProtocol::storeConfigWidget()
{
	bool isOk;

	configWidget();
}

