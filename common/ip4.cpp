#include <qendian.h>
#include <QHostAddress>

#include "ip4.h"

Ip4ConfigForm *Ip4Protocol::configForm = NULL;

Ip4ConfigForm::Ip4ConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);

	connect(cmbIpSrcAddrMode, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_cmbIpSrcAddrMode_currentIndexChanged(int)));
	connect(cmbIpDstAddrMode, SIGNAL(currentIndexChanged(int)),
			this, SLOT(on_cmbIpDstAddrMode_currentIndexChanged(int)));
}

void Ip4ConfigForm::on_cmbIpSrcAddrMode_currentIndexChanged(int index)
{
	if (index == OstProto::Ip4::e_im_fixed)
	{
		leIpSrcAddrCount->setDisabled(true);
		leIpSrcAddrMask->setDisabled(true);
	}
	else
	{
		leIpSrcAddrCount->setEnabled(true);
		leIpSrcAddrMask->setEnabled(true);
	}
}

void Ip4ConfigForm::on_cmbIpDstAddrMode_currentIndexChanged(int index)
{
	if (index == OstProto::Ip4::e_im_fixed)
	{
		leIpDstAddrCount->setDisabled(true);
		leIpDstAddrMask->setDisabled(true);
	}
	else
	{
		leIpDstAddrCount->setEnabled(true);
		leIpDstAddrMask->setEnabled(true);
	}
}

Ip4Protocol::Ip4Protocol(Stream *parent)
	: AbstractProtocol(parent)
{
	if (configForm == NULL)
		configForm = new Ip4ConfigForm;
}

Ip4Protocol::~Ip4Protocol()
{
}

void Ip4Protocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::ip4)->CopyFrom(data);
}

void Ip4Protocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::ip4))
		data.MergeFrom(stream.GetExtension(OstProto::ip4));
}

QString Ip4Protocol::name() const
{
	return QString("Internet Protocol ver 4");
}

QString Ip4Protocol::shortName() const
{
	return QString("IPv4");
}

int	Ip4Protocol::fieldCount() const
{
	return ip4_fieldCount;
}

QVariant Ip4Protocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case ip4_ver:
			switch(attrib)
			{
				case FieldName:			
					return QString("Version");
				case FieldValue:
					return (data.ver_hdrlen() >> 4) & 0x0F;
				case FieldTextValue:
					return QString("%1").arg((data.ver_hdrlen() >> 4) & 0x0F);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.ver_hdrlen() & 0xF0));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;
		case ip4_hdrLen:
			switch(attrib)
			{
				case FieldName:			
					return QString("Header Length");
				case FieldValue:
					return data.ver_hdrlen() & 0x0F;
				case FieldTextValue:
					return QString("%1").arg(data.ver_hdrlen() & 0x0F);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.ver_hdrlen() << 4));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;
		case ip4_tos:
			switch(attrib)
			{
				case FieldName:			
					return QString("TOS/DSCP");
				case FieldValue:
					return data.tos();
				case FieldFrameValue:
					return QByteArray(1, (char) data.tos());
				case FieldTextValue:
					return QString("0x%1").
						arg(data.tos(), 2, BASE_HEX, QChar('0'));;
				default:
					break;
			}
			break;
		case ip4_totLen:
			switch(attrib)
			{
				case FieldName:			
					return QString("Total Length");
				case FieldValue:
					return data.totlen();
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.totlen(), (uchar*) fv.data());
					return fv;
				}
				case FieldTextValue:
					return QString("%1").arg(data.totlen());
				default:
					break;
			}
			break;
		case ip4_id:
			switch(attrib)
			{
				case FieldName:			
					return QString("Identification");
				case FieldValue:
					return data.id();
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.id(), (uchar*)fv.data());
					return fv;
				}
				case FieldTextValue:
					return QString("0x%1").
						arg(data.id(), 2, BASE_HEX, QChar('0'));;
				default:
					break;
			}
			break;
		case ip4_flags:
			switch(attrib)
			{
				case FieldName:			
					return QString("Flags");
				case FieldValue:
					return data.flags();
				case FieldFrameValue:
					return QByteArray(1, (char) data.flags());
				case FieldTextValue:
				{
					QString s;
					s.append("Unused:");
					s.append(data.flags() & IP_FLAG_UNUSED ? "1" : "0");
					s.append("  Don't Fragment:");
					s.append(data.flags() & IP_FLAG_DF ? "1" : "0");
					s.append("  More Fragments:");
					s.append(data.flags() & IP_FLAG_MF ? "1" : "0");
					return s;
				}
				case FieldBitSize:
					return 3;
				default:
					break;
			}
			break;
		case ip4_fragOfs:
			switch(attrib)
			{
				case FieldName:			
					return QString("Fragment Offset");
				case FieldValue:
					return data.frag_ofs();
				case FieldFrameValue:
				{
					QByteArray fv;
					// FIXME need to shift for 13 bits
					fv.resize(2);
					qToBigEndian((quint16) data.frag_ofs(), (uchar*) fv.data());
					return fv;
				}
				case FieldTextValue:
					return QString("%1").arg(data.frag_ofs());
				case FieldBitSize:
					return 13;
				default:
					break;
			}
			break;
		case ip4_ttl:
			switch(attrib)
			{
				case FieldName:			
					return QString("Time to Live");
				case FieldValue:
					return data.ttl();
				case FieldFrameValue:
					return QByteArray(1, (char)data.ttl());
				case FieldTextValue:
					return QString("%1").arg(data.ttl());
				default:
					break;
			}
			break;
		case ip4_proto:
			switch(attrib)
			{
				case FieldName:			
					return QString("Protocol");
				case FieldValue:
					return data.proto();
				case FieldFrameValue:
					return QByteArray(1, (char)data.proto());
				case FieldTextValue:
					return  QString("0x%1").
						arg(data.proto(), 2, BASE_HEX, QChar('0'));
				default:
					break;
			}
			break;
		case ip4_cksum:
			switch(attrib)
			{
				case FieldName:			
					return QString("Header Checksum");
				case FieldValue:
					return data.cksum();
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.cksum(), (uchar*) fv.data());
					return fv;
				}
				case FieldTextValue:
					return  QString("0x%1").
						arg(data.cksum(), 4, BASE_HEX, QChar('0'));;
				default:
					break;
			}
			break;
		case ip4_srcAddr:
			switch(attrib)
			{
				case FieldName:			
					return QString("Source");
				case FieldValue:
					return data.src_ip();
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(4);
					qToBigEndian((quint32) data.src_ip(), (uchar*) fv.data());
					return fv;
				}
				case FieldTextValue:
					return QHostAddress(data.src_ip()).toString();
				default:
					break;
			}
			break;
		case ip4_dstAddr:
			switch(attrib)
			{
				case FieldName:			
					return QString("Destination");
				case FieldValue:
					return data.dst_ip();
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(4);
					qToBigEndian((quint32) data.dst_ip(), (uchar*) fv.data());
					return fv;
				}
				case FieldTextValue:
					return QHostAddress(data.dst_ip()).toString();
				default:
					break;
			}
			break;

		// Meta fields

		case ip4_isOverrideVer:
		case ip4_isOverrideHdrLen:
		case ip4_isOverrideTotLen:
		case ip4_isOverrideCksum:

		case ip4_srcAddrMode:
		case ip4_srcAddrCount:
		case ip4_srcAddrMask:

		case ip4_dstAddrMode:
		case ip4_dstAddrCount:
		case ip4_dstAddrMask:
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

bool Ip4Protocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	bool isOk = false;

	if (attrib != FieldValue)
		return false;

	switch (index)
	{
		case ip4_proto:
		{
			uint proto = value.toUInt(&isOk);
			if (isOk)
				data.set_proto(proto);
		}
		default:
			break;
	}
	return isOk;
}


QWidget* Ip4Protocol::configWidget()
{
	return configForm;
}

void Ip4Protocol::loadConfigWidget()
{
#define uintToHexStr(num, str, size) QString().setNum(num, 16)
	configForm->leIpVersion->setText(QString().setNum(data.ver_hdrlen() >> 4));
	configForm->cbIpVersionOverride->setChecked(data.is_override_ver());

	configForm->leIpHdrLen->setText(QString().setNum(data.ver_hdrlen() & 0x0F));
	configForm->cbIpHdrLenOverride->setChecked(data.is_override_hdrlen());
	
	configForm->leIpTos->setText(uintToHexStr(data.tos(), QString(), 1));

	configForm->leIpLength->setText(QString().setNum(data.totlen()));
	configForm->cbIpLengthOverride->setChecked(data.is_override_totlen());

	configForm->leIpId->setText(uintToHexStr(data.id(), QString(), 2));
	configForm->leIpFragOfs->setText(QString().setNum(data.frag_ofs()));
	configForm->cbIpFlagsDf->setChecked((data.flags() & IP_FLAG_DF) > 0);
	configForm->cbIpFlagsMf->setChecked((data.flags() & IP_FLAG_MF) > 0);

	configForm->leIpTtl->setText(QString().setNum(data.ttl()));
	configForm->leIpProto->setText(uintToHexStr(data.proto(), QString(), 1));

	configForm->leIpCksum->setText(uintToHexStr(data.cksum(), QString(), 2));
	configForm->cbIpCksumOverride->setChecked(data.is_override_cksum());

	configForm->leIpSrcAddr->setText(QHostAddress(data.src_ip()).toString());
	configForm->cmbIpSrcAddrMode->setCurrentIndex(data.src_ip_mode());
	configForm->leIpSrcAddrCount->setText(QString().setNum(data.src_ip_count()));
	configForm->leIpSrcAddrMask->setText(QHostAddress(data.src_ip_mask()).toString());

	configForm->leIpDstAddr->setText(QHostAddress(data.dst_ip()).toString());
	configForm->cmbIpDstAddrMode->setCurrentIndex(data.dst_ip_mode());
	configForm->leIpDstAddrCount->setText(QString().setNum(data.dst_ip_count()));
	configForm->leIpDstAddrMask->setText(QHostAddress(data.dst_ip_mask()).toString());
}

void Ip4Protocol::storeConfigWidget()
{
	uint ff = 0;
	bool isOk;

	data.set_is_override_ver(configForm->cbIpVersionOverride->isChecked());
	data.set_ver_hdrlen(((configForm->leIpVersion->text().toULong(&isOk) & 0x0F) << 4) |
			(configForm->leIpHdrLen->text().toULong(&isOk) & 0x0F));
	data.set_is_override_hdrlen(configForm->cbIpHdrLenOverride->isChecked());

	data.set_tos(configForm->leIpTos->text().toULong(&isOk, 16));

	data.set_totlen(configForm->leIpLength->text().toULong(&isOk));
	data.set_is_override_totlen(configForm->cbIpLengthOverride->isChecked());

	data.set_id(configForm->leIpId->text().remove(QChar(' ')).toULong(&isOk, 16));
	data.set_frag_ofs(configForm->leIpFragOfs->text().toULong(&isOk));

	if (configForm->cbIpFlagsDf->isChecked()) ff |= IP_FLAG_DF;
	if (configForm->cbIpFlagsMf->isChecked()) ff |= IP_FLAG_MF;
	data.set_flags(ff);

	data.set_ttl(configForm->leIpTtl->text().toULong(&isOk));
	data.set_proto(configForm->leIpProto->text().remove(QChar(' ')).toULong(&isOk, 16));
	
	data.set_cksum(configForm->leIpCksum->text().remove(QChar(' ')).toULong(&isOk));
	data.set_is_override_cksum(configForm->cbIpCksumOverride->isChecked());

	data.set_src_ip(QHostAddress(configForm->leIpSrcAddr->text()).toIPv4Address());
	data.set_src_ip_mode((OstProto::Ip4_IpAddrMode)configForm->cmbIpSrcAddrMode->currentIndex());
	data.set_src_ip_count(configForm->leIpSrcAddrCount->text().toULong(&isOk));
	data.set_src_ip_mask(QHostAddress(configForm->leIpSrcAddrMask->text()).toIPv4Address());

	data.set_dst_ip(QHostAddress(configForm->leIpDstAddr->text()).toIPv4Address());
	data.set_dst_ip_mode((OstProto::Ip4_IpAddrMode)configForm->cmbIpDstAddrMode->currentIndex());
	data.set_dst_ip_count(configForm->leIpDstAddrCount->text().toULong(&isOk));
}

