#include <qendian.h>
#include <QHostAddress>

#include "llc.h"

LlcConfigForm *LlcProtocol::configForm = NULL;

LlcConfigForm::LlcConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

LlcProtocol::LlcProtocol(Stream *parent)
	: AbstractProtocol(parent)
{
	if (configForm == NULL)
		configForm = new LlcConfigForm;
}

LlcProtocol::~LlcProtocol()
{
}

void LlcProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::llc)->CopyFrom(data);
}

void LlcProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::llc))
		data.MergeFrom(stream.GetExtension(OstProto::llc));
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
	switch (index)
	{
		case llc_dsap:
			switch(attrib)
			{
				case FieldName:			
					return QString("DSAP");
				case FieldValue:
					return data.dsap();
				case FieldTextValue:
					return QString("%1").arg(data.dsap(), BASE_HEX);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.dsap()));
				default:
					break;
			}
			break;
		case llc_ssap:
			switch(attrib)
			{
				case FieldName:			
					return QString("DSAP");
				case FieldValue:
					return data.ssap();
				case FieldTextValue:
					return QString("%1").arg(data.ssap(), BASE_HEX);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.ssap()));
				default:
					break;
			}
			break;
		case llc_ctl:
			switch(attrib)
			{
				case FieldName:			
					return QString("DSAP");
				case FieldValue:
					return data.ctl();
				case FieldTextValue:
					return QString("%1").arg(data.ctl(), BASE_HEX);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.ctl()));
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
	return configForm;
}

void LlcProtocol::loadConfigWidget()
{
	configForm->leDsap->setText(QString("%1").arg(data.dsap(), 2, BASE_HEX, QChar('0')));
	configForm->leSsap->setText(QString("%1").arg(data.ssap(), 2, BASE_HEX, QChar('0')));
	configForm->leControl->setText(QString("%1").arg(data.ctl(), 2, BASE_HEX, QChar('0')));
}

void LlcProtocol::storeConfigWidget()
{
	bool isOk;

	data.set_dsap(configForm->leDsap->text().toULong(&isOk, BASE_HEX));
	data.set_ssap(configForm->leSsap->text().toULong(&isOk, BASE_HEX));
	data.set_ctl(configForm->leControl->text().toULong(&isOk, BASE_HEX));
}

