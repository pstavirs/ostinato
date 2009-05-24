#include <qendian.h>
#include <QHostAddress>

#include "sample.h"

SampleConfigForm *SampleProtocol::configForm = NULL;

SampleConfigForm::SampleConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);

}

SampleProtocol::SampleProtocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
	: AbstractProtocol(frameProtoList, parent)
{
	if (configForm == NULL)
		configForm = new SampleConfigForm;
}

SampleProtocol::~SampleProtocol()
{
}

AbstractProtocol* SampleProtocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return new SampleProtocol(frameProtoList, streamCore);
}

void SampleProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::sample)->CopyFrom(data);
}

void SampleProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::sample))
		data.MergeFrom(stream.GetExtension(OstProto::sample));
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
	return configForm;
}

void SampleProtocol::loadConfigWidget()
{
}

void SampleProtocol::storeConfigWidget()
{
	bool isOk;
}

