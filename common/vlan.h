#ifndef _Vlan_H
#define _Vlan_H

#include "abstractprotocol.h"

#include "vlan.pb.h"
#include "ui_vlan.h"

class VlanConfigForm : public QWidget, public Ui::Vlan
{
	Q_OBJECT
public:
	VlanConfigForm(QWidget *parent = 0);
};

class VlanProtocol : public AbstractProtocol
{
private:
	OstProto::Vlan	data;
	static VlanConfigForm	*configForm;
	enum Vlanfield
	{
		vlan_tpid,
		vlan_prio,
		vlan_cfiDei,
		vlan_vlanId,

		// meta-fields
		vlan_isOverrideTpid,

		vlan_fieldCount
	};

public:
	VlanProtocol(ProtocolList &frameProtoList, 
		OstProto::StreamCore *parent = 0);
	virtual ~VlanProtocol();

	static AbstractProtocol* createInstance(
		ProtocolList &frameProtoList,
		OstProto::StreamCore *streamCore = 0);

	virtual void protoDataCopyInto(OstProto::Stream &stream);
	virtual void protoDataCopyFrom(const OstProto::Stream &stream);

	virtual QString name() const;
	virtual QString shortName() const;

	virtual int	fieldCount() const;

	virtual AbstractProtocol::FieldFlags fieldFlags(int index) const;
	virtual QVariant fieldData(int index, FieldAttrib attrib,
		   	int streamIndex = 0) const;
	virtual bool setFieldData(int index, const QVariant &value, 
			FieldAttrib attrib = FieldValue);

	virtual QWidget* configWidget();
	virtual void loadConfigWidget();
	virtual void storeConfigWidget();
};

#endif
