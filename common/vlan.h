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
	VlanConfigForm	*configForm;
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
	VlanProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
	virtual ~VlanProtocol();

	static AbstractProtocol* createInstance(StreamBase *stream,
		AbstractProtocol *parent = 0);
	virtual quint32 protocolNumber() const;

	virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
	virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

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
