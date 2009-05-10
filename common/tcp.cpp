#include <qendian.h>
#include <QHostAddress>

#include "tcp.h"

TcpConfigForm *TcpProtocol::configForm = NULL;

TcpConfigForm::TcpConfigForm(QWidget *parent)
	: QWidget(parent)
{
	setupUi(this);
}

TcpProtocol::TcpProtocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
	: AbstractProtocol(frameProtoList, parent)
{
	if (configForm == NULL)
		configForm = new TcpConfigForm;
}

TcpProtocol::~TcpProtocol()
{
}

AbstractProtocol* TcpProtocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return new TcpProtocol(frameProtoList, streamCore);
}

void TcpProtocol::protoDataCopyInto(OstProto::Stream &stream)
{
	// FIXME: multiple headers
	stream.MutableExtension(OstProto::tcp)->CopyFrom(data);
}

void TcpProtocol::protoDataCopyFrom(const OstProto::Stream &stream)
{
	// FIXME: multiple headers
	if (stream.HasExtension(OstProto::tcp))
		data.MergeFrom(stream.GetExtension(OstProto::tcp));
}

QString TcpProtocol::name() const
{
	return QString("Transmission Control Protocol");
}

QString TcpProtocol::shortName() const
{
	return QString("TCP");
}

quint32 TcpProtocol::protocolId(ProtocolIdType type) const
{
	switch(type)
	{
		case ProtocolIdIp: return 0x06;
		default: break;
	}

	return AbstractProtocol::protocolId(type);
}

int	TcpProtocol::fieldCount() const
{
	return tcp_fieldCount;
}

QVariant TcpProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (index)
	{
		case tcp_src_port:
			switch(attrib)
			{
				case FieldName:			
					return QString("Source Port");
				case FieldValue:
					return data.src_port();
				case FieldTextValue:
					return QString("%1").arg(data.src_port());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.src_port(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		case tcp_dst_port:
			switch(attrib)
			{
				case FieldName:			
					return QString("Destination Port");
				case FieldValue:
					return data.dst_port();
				case FieldTextValue:
					return QString("%1").arg(data.dst_port());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.dst_port(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		case tcp_seq_num:
			switch(attrib)
			{
				case FieldName:			
					return QString("Sequence Number");
				case FieldValue:
					return data.seq_num();
				case FieldTextValue:
					return QString("%1").arg(data.seq_num());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(4);
					qToBigEndian((quint32) data.seq_num(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		case tcp_ack_num:
			switch(attrib)
			{
				case FieldName:			
					return QString("Sequence Number");
				case FieldValue:
					return data.ack_num();
				case FieldTextValue:
					return QString("%1").arg(data.ack_num());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(4);
					qToBigEndian((quint32) data.ack_num(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		case tcp_hdrlen:
			switch(attrib)
			{
				case FieldName:			
					return QString("Header Length");
				case FieldValue:
					return ((data.hdrlen_rsvd() >> 4) & 0x0F);
				case FieldTextValue:
					return QString("%1").arg((data.hdrlen_rsvd() >> 4) & 0x0F);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.hdrlen_rsvd() & 0xF0));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;

		case tcp_rsvd:
			switch(attrib)
			{
				case FieldName:			
					return QString("Reserved");
				case FieldValue:
					return (data.hdrlen_rsvd() & 0x0F);
				case FieldTextValue:
					return QString("%1").arg(data.hdrlen_rsvd() & 0x0F);
				case FieldFrameValue:
					return QByteArray(1, (char)(data.hdrlen_rsvd() & 0x0F));
				case FieldBitSize:
					return 4;
				default:
					break;
			}
			break;

		case tcp_flags:
			switch(attrib)
			{
				case FieldName:			
					return QString("Flags");
				case FieldValue:
					return (data.flags());
				case FieldTextValue:
				{
					QString s;
					s.append("URG: ");
					s.append(data.flags() & TCP_FLAG_URG ? "1" : "0");
					s.append(" ACK: ");
					s.append(data.flags() & TCP_FLAG_ACK ? "1" : "0");
					s.append(" PSH: ");
					s.append(data.flags() & TCP_FLAG_PSH ? "1" : "0");
					s.append(" RST: ");
					s.append(data.flags() & TCP_FLAG_RST ? "1" : "0");
					s.append(" SYN: ");
					s.append(data.flags() & TCP_FLAG_SYN ? "1" : "0");
					s.append(" FIN: ");
					s.append(data.flags() & TCP_FLAG_FIN ? "1" : "0");
					return s;
				}
				case FieldFrameValue:
					return QByteArray(1, (char)(data.flags() & 0x3F));
				default:
					break;
			}
			break;

		case tcp_window:
			switch(attrib)
			{
				case FieldName:			
					return QString("Window Size");
				case FieldValue:
					return data.window();
				case FieldTextValue:
					return QString("%1").arg(data.window());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.window(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		case tcp_cksum:
			switch(attrib)
			{
				case FieldName:			
					return QString("Checksum");
				case FieldValue:
					return data.cksum();
				case FieldTextValue:
					return QString("%1").arg(data.cksum());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.cksum(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		case tcp_urg_ptr:
			switch(attrib)
			{
				case FieldName:			
					return QString("Urgent Pointer");
				case FieldValue:
					return data.urg_ptr();
				case FieldTextValue:
					return QString("%1").arg(data.urg_ptr());
				case FieldFrameValue:
				{
					QByteArray fv;
					fv.resize(2);
					qToBigEndian((quint16) data.urg_ptr(), (uchar*) fv.data());
					return fv;
				}
				default:
					break;
			}
			break;

		// Meta fields
		case tcp_is_override_hdrlen:
		case tcp_is_override_cksum:
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

bool TcpProtocol::setFieldData(int index, const QVariant &value, 
		FieldAttrib attrib)
{
	// FIXME
	return false;
}


QWidget* TcpProtocol::configWidget()
{
	return configForm;
}

void TcpProtocol::loadConfigWidget()
{
	configForm->leTcpSrcPort->setText(QString().setNum(data.src_port()));
	configForm->leTcpDstPort->setText(QString().setNum(data.dst_port()));

	configForm->leTcpSeqNum->setText(QString().setNum(data.seq_num()));
	configForm->leTcpAckNum->setText(QString().setNum(data.ack_num()));

	configForm->leTcpHdrLen->setText(QString().setNum((data.hdrlen_rsvd() >> 4) & 0x0F));
	configForm->cbTcpHdrLenOverride->setChecked(data.is_override_hdrlen());

	configForm->leTcpWindow->setText(QString().setNum(data.window()));

	configForm->leTcpCksum->setText(QString().setNum(data.cksum()));
	configForm->cbTcpCksumOverride->setChecked(data.is_override_cksum());

	configForm->leTcpUrgentPointer->setText(QString().setNum(data.urg_ptr()));

	configForm->cbTcpFlagsUrg->setChecked((data.flags() & TCP_FLAG_URG) > 0);
	configForm->cbTcpFlagsAck->setChecked((data.flags() & TCP_FLAG_ACK) > 0);
	configForm->cbTcpFlagsPsh->setChecked((data.flags() & TCP_FLAG_PSH) > 0);
	configForm->cbTcpFlagsRst->setChecked((data.flags() & TCP_FLAG_RST) > 0);
	configForm->cbTcpFlagsSyn->setChecked((data.flags() & TCP_FLAG_SYN) > 0);
	configForm->cbTcpFlagsFin->setChecked((data.flags() & TCP_FLAG_FIN) > 0);
}

void TcpProtocol::storeConfigWidget()
{
	bool isOk;
	int ff = 0;

	data.set_src_port(configForm->leTcpSrcPort->text().toULong(&isOk));
	data.set_dst_port(configForm->leTcpDstPort->text().toULong(&isOk));

	data.set_seq_num(configForm->leTcpSeqNum->text().toULong(&isOk));
	data.set_ack_num(configForm->leTcpAckNum->text().toULong(&isOk));

	data.set_hdrlen_rsvd((configForm->leTcpHdrLen->text().toULong(&isOk) << 4) & 0xF0);
	data.set_is_override_hdrlen(configForm->cbTcpHdrLenOverride->isChecked());

	data.set_window(configForm->leTcpWindow->text().toULong(&isOk));

	data.set_cksum(configForm->leTcpCksum->text().remove(QChar(' ')).toULong(&isOk));
	data.set_is_override_cksum(configForm->cbTcpCksumOverride->isChecked());

	data.set_urg_ptr(configForm->leTcpUrgentPointer->text().toULong(&isOk));

	if (configForm->cbTcpFlagsUrg->isChecked()) ff |= TCP_FLAG_URG;
	if (configForm->cbTcpFlagsAck->isChecked()) ff |= TCP_FLAG_ACK;
	if (configForm->cbTcpFlagsPsh->isChecked()) ff |= TCP_FLAG_PSH;
	if (configForm->cbTcpFlagsRst->isChecked()) ff |= TCP_FLAG_RST;
	if (configForm->cbTcpFlagsSyn->isChecked()) ff |= TCP_FLAG_SYN;
	if (configForm->cbTcpFlagsFin->isChecked()) ff |= TCP_FLAG_FIN;
	data.set_flags(ff);
}

