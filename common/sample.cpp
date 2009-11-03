#include <qendian.h>
#include <QHostAddress>

#include "sample.h"

/*! \todo (MED) Complete the "sample" protocol and make it compilable so that 
  it can be used as an example for new protocols
 */

SampleConfigForm::SampleConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

SampleProtocol::SampleProtocol(StreamBase *stream, AbstractProtocol *parent);
	: AbstractProtocol(stream, parent)
{
	configForm = NULL;
}

SampleProtocol::~SampleProtocol()
{
	delete configForm;
}

AbstractProtocol* SampleProtocol::createInstance(StreamBase *stream,
	AbstractProtocol *parent)
{
	return new SampleProtocol(stream, parent);
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
		case sample_one:
		{
			switch(attrib)
			{
				case FieldName:			
					return QString("ONE");
				case FieldValue:
					return data.one();
				case FieldTextValue:
					return QString("%1").arg(data.one());
				case FieldFrameValue:
					return QByteArray(1, (char)(data.one() & 0xF0));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;

		}
		case sample_two:
		{
			switch(attrib)
			{
				case FieldName:			
					return QString("TWO");
				case FieldValue:
					return data.two();
				case FieldTextValue:
					return QString("%1").arg(data.two());
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

