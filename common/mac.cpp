#include <qendian.h>
#include <QHostAddress>

#include "mac.h"

MacConfigForm *MacProtocol::configForm = NULL;

MacConfigForm::MacConfigForm(QWidget *parent)
	: QWidget(parent)
{
	QRegExp reMac("([0-9,a-f,A-F]{2,2}[:-]){5,5}[0-9,a-f,A-F]{2,2}");

	setupUi(this);
	leDstMac->setValidator(new QRegExpValidator(reMac, this));
	leSrcMac->setValidator(new QRegExpValidator(reMac, this));
	leDstMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
	leSrcMacCount->setValidator(new QIntValidator(1, MAX_MAC_ITER_COUNT, this));
}

void MacConfigForm::on_cmbDstMacMode_currentIndexChanged(int index)
{
	if (index == OstProto::Mac::e_mm_fixed)
	{
		leDstMacCount->setEnabled(false);
		leDstMacStep->setEnabled(false);
	}
	else
	{
		leDstMacCount->setEnabled(true);
		leDstMacStep->setEnabled(true);
	}
}

void MacConfigForm::on_cmbSrcMacMode_currentIndexChanged(int index)
{
	if (index == OstProto::Mac::e_mm_fixed)
	{
		leSrcMacCount->setEnabled(false);
		leSrcMacStep->setEnabled(false);
	}
	else
	{
		leSrcMacCount->setEnabled(true);
		leSrcMacStep->setEnabled(true);
	}
}


MacProtocol::MacProtocol(Stream *parent)
	: AbstractProtocol(parent)
{
	if (configForm == NULL)
		configForm = new MacConfigForm;
}

MacProtocol::~MacProtocol()
{
}

void MacProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::mac)->CopyFrom(data);
}

void MacProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::mac))
		data.MergeFrom(stream.GetExtension(OstProto::mac));
}

QString MacProtocol::name() const
{
	return QString("Media Access Protocol");
}

QString MacProtocol::shortName() const
{
	return QString("MAC");
}

int	MacProtocol::fieldCount() const
{
	return mac_fieldCount;
}

QVariant MacProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case mac_dstAddr:
			switch(attrib)
			{
				case FieldName:			
					return QString("Desination");
				case FieldValue:
					return data.dst_mac();
				case FieldTextValue:
					return QString("%1").arg(data.dst_mac(), 12, BASE_HEX, 
						QChar('0'));
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(8);
					qToBigEndian((quint64) data.dst_mac(), (uchar*) fv.data());
					fv.remove(0, 2);
					return fv;
				}
				default:
					break;
			}
			break;

		case mac_srcAddr:
			switch(attrib)
			{
				case FieldName:			
					return QString("Source");
				case FieldValue:
					return data.src_mac();
				case FieldTextValue:
					return QString("%1").arg(data.src_mac(), 12, BASE_HEX, 
						QChar('0'));
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(8);
					qToBigEndian((quint64) data.src_mac(), (uchar*) fv.data());
					fv.remove(0, 2);
					return fv;
				}
				default:
					break;
			}
			break;

		// Meta fields
		case mac_dstMacMode:
		case mac_dstMacCount:
		case mac_dstMacStep:
		case mac_srcMacMode:
		case mac_srcMacCount:
		case mac_srcMacStep:
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

bool MacProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* MacProtocol::configWidget()
{
	return configForm;
}

void MacProtocol::loadConfigWidget()
{
#define uintToHexStr(num, str, size) QString().setNum(num, 16)
	configForm->leDstMac->setText(uintToHexStr(data.dst_mac(), str, 6));
	configForm->cmbDstMacMode->setCurrentIndex(data.dst_mac_mode());
	configForm->leDstMacCount->setText(QString().setNum(data.dst_mac_count()));
	configForm->leDstMacStep->setText(QString().setNum(data.dst_mac_step()));

	configForm->leSrcMac->setText(uintToHexStr(data.src_mac(), QString(), 6));
	configForm->cmbSrcMacMode->setCurrentIndex(data.src_mac_mode());
	configForm->leSrcMacCount->setText(QString().setNum(data.src_mac_count()));
	configForm->leSrcMacStep->setText(QString().setNum(data.src_mac_step()));
}

void MacProtocol::storeConfigWidget()
{
	bool isOk;

	data.set_dst_mac(configForm->leDstMac->text().remove(QChar(' ')).
			toULongLong(&isOk, 16));
	data.set_dst_mac_mode((OstProto::Mac::MacAddrMode) configForm->
			cmbDstMacMode->currentIndex());
	data.set_dst_mac_count(configForm->leDstMacCount->text().toULong(&isOk));
	data.set_dst_mac_step(configForm->leDstMacStep->text().toULong(&isOk));

	data.set_src_mac(configForm->leSrcMac->text().remove(QChar(' ')).
			toULongLong(&isOk, 16));
	data.set_src_mac_mode((OstProto::Mac::MacAddrMode) configForm->
			cmbSrcMacMode->currentIndex());
	data.set_src_mac_count(configForm->leSrcMacCount->text().toULong(&isOk));
	data.set_src_mac_step(configForm->leSrcMacStep->text().toULong(&isOk));
}

