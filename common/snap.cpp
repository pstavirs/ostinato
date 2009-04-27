#include <qendian.h>
#include <QHostAddress>

#include "snap.h"

SnapConfigForm *SnapProtocol::configForm = NULL;

SnapConfigForm::SnapConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

SnapProtocol::SnapProtocol(Stream *parent)
	: AbstractProtocol(parent)
{
	if (configForm == NULL)
		configForm = new SnapConfigForm;
}

SnapProtocol::~SnapProtocol()
{
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
#define uintToHexStr(num, str, size) QString().setNum(num, 16)
	configForm->leOui->setText(uintToHexStr(data.oui(), str, 3));
}

void SnapProtocol::storeConfigWidget()
{
	bool isOk;

	data.set_oui(configForm->leOui->text().remove(QChar(' ')).toULong(&isOk, 16));
}

