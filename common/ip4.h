#ifndef _IPV4_H
#define _IPV4_H

#include "abstractprotocol.h"

#include "ip4.pb.h"
#include "ui_ip4.h"

#define IP_FLAG_UNUSED	0x1
#define IP_FLAG_DF		0x2
#define IP_FLAG_MF		0x4


class Ip4ConfigForm : public QWidget, public Ui::ip4
{
	Q_OBJECT
public:
	Ip4ConfigForm(QWidget *parent = 0);
private slots:
	void on_cmbIpSrcAddrMode_currentIndexChanged(int index);
	void on_cmbIpDstAddrMode_currentIndexChanged(int index);
};

class Ip4Protocol : public AbstractProtocol
{
private:
	OstProto::Ip4	data;
	static Ip4ConfigForm	*configForm;
	enum ip4field
	{
		ip4_ver = 0,
		ip4_hdrLen,
		ip4_tos,
		ip4_totLen,
		ip4_id,
		ip4_flags,
		ip4_fragOfs,
		ip4_ttl,
		ip4_proto,
		ip4_cksum,
		ip4_srcAddr,
		ip4_dstAddr,

		ip4_isOverrideVer,
		ip4_isOverrideHdrLen,
		ip4_isOverrideTotLen,
		ip4_isOverrideCksum,

		ip4_srcAddrMode,
		ip4_srcAddrCount,
		ip4_srcAddrMask,

		ip4_dstAddrMode,
		ip4_dstAddrCount,
		ip4_dstAddrMask,

		ip4_fieldCount
	};

public:
	Ip4Protocol(ProtocolList &frameProtoList, 
		OstProto::StreamCore *parent = 0);
	virtual ~Ip4Protocol();

	static AbstractProtocol* createInstance(
		ProtocolList &frameProtoList,
		OstProto::StreamCore *streamCore = 0);

	virtual void protoDataCopyInto(OstProto::Stream &stream);
	virtual void protoDataCopyFrom(const OstProto::Stream &stream);

	virtual QString name() const;
	virtual QString shortName() const;
	virtual quint32 protocolId(ProtocolIdType type) const;
	virtual int	fieldCount() const;

	virtual QVariant fieldData(int index, FieldAttrib attrib,
		   	int streamIndex = 0) const;
	virtual bool setFieldData(int index, const QVariant &value, 
			FieldAttrib attrib = FieldValue);

	virtual QWidget* configWidget();
	virtual void loadConfigWidget();
	virtual void storeConfigWidget();
};


#endif
