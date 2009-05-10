#include <qendian.h>
#include <QHostAddress>

//#include "../client/stream.h"
#include "payload.h"

#define SZ_FCS		4

PayloadConfigForm *PayloadProtocol::configForm = NULL;

PayloadConfigForm::PayloadConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

void PayloadConfigForm::on_cmbPatternMode_currentIndexChanged(int index)
{
	switch(index)
	{
		case OstProto::Payload::e_dp_fixed_word:
			lePattern->setEnabled(true);
			break;
		case OstProto::Payload::e_dp_inc_byte:
		case OstProto::Payload::e_dp_dec_byte:
		case OstProto::Payload::e_dp_random:
			lePattern->setDisabled(true);
			break;
		default:
			qWarning("Unhandled/Unknown PatternMode = %d",index);
	}
}

PayloadProtocol::PayloadProtocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
	: AbstractProtocol(frameProtoList, parent)
{
	if (configForm == NULL)
		configForm = new PayloadConfigForm;
}

PayloadProtocol::~PayloadProtocol()
{
}

AbstractProtocol* PayloadProtocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return new PayloadProtocol(frameProtoList, streamCore);
}

void PayloadProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::payload)->CopyFrom(data);
}

void PayloadProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::payload))
		data.MergeFrom(stream.GetExtension(OstProto::payload));
}

QString PayloadProtocol::name() const
{
	return QString("Payload Data");
}

QString PayloadProtocol::shortName() const
{
	return QString("DATA");
}

int	PayloadProtocol::fieldCount() const
{
	return payload_fieldCount;
}

QVariant PayloadProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case payload_dataPattern:
			switch(attrib)
			{
				case FieldName:			
					return QString("Data");
				case FieldValue:
					return data.pattern();
				case FieldTextValue:
					return QString(fieldData(index, FieldFrameValue, 
							streamIndex).toByteArray().toHex());
				case FieldFrameValue:
				{
					QByteArray fv;
					int dataLen;

					dataLen = stream->frame_len() - protocolFrameOffset();
					dataLen -= SZ_FCS;
					fv.resize(dataLen+4);
					switch(data.pattern_mode())
					{
						case OstProto::Payload::e_dp_fixed_word:
							for (int i = 0; i < (dataLen/4)+1; i++)
								qToBigEndian((quint32) data.pattern(), 
									(uchar*)(fv.data()+(i*4)) );
							break;
						case OstProto::Payload::e_dp_inc_byte:
							for (int i = 0; i < dataLen; i++)
								fv[i] = i % (0xFF + 1);
							break;
						case OstProto::Payload::e_dp_dec_byte:
							for (int i = 0; i < dataLen; i++)
								fv[i] = 0xFF - (i % (0xFF + 1));
							break;
						case OstProto::Payload::e_dp_random:
							for (int i = 0; i < dataLen; i++)
								fv[i] =  qrand() % (0xFF + 1);
							break;
						default:
							qWarning("Unhandled data pattern %d", 
								data.pattern_mode());
					}
					fv.resize(dataLen);
					return fv;
				}
				default:
					break;
			}
			break;

		// Meta fields

		case payload_dataPatternMode:
			switch(attrib)
			{
				case FieldIsMeta:
					return true;
				default:
					break;
			}
			break;

		default:
			break;
	}

	return AbstractProtocol::fieldData(index, attrib, streamIndex);
}

bool PayloadProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* PayloadProtocol::configWidget()
{
	return configForm;
	//return new PayloadConfigForm;
}

void PayloadProtocol::loadConfigWidget()
{
	configForm->cmbPatternMode->setCurrentIndex(data.pattern_mode());
	configForm->lePattern->setText(uintToHexStr(data.pattern(), 4));
}

void PayloadProtocol::storeConfigWidget()
{
	bool isOk;

	data.set_pattern_mode((OstProto::Payload::DataPatternMode) 
		configForm->cmbPatternMode->currentIndex());
	data.set_pattern(configForm->lePattern->text().remove(QChar(' ')).toULong(&isOk, 16));
}

