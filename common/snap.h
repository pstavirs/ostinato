#ifndef _SNAP_H
#define _SNAP_H

#include "abstractprotocol.h"

#include "snap.pb.h"
#include "ui_snap.h"

class SnapConfigForm : public QWidget, public Ui::snap
{
	Q_OBJECT
public:
	SnapConfigForm(QWidget *parent = 0);
};

class SnapProtocol : public AbstractProtocol
{
private:
	OstProto::Snap	data;
	SnapConfigForm	*configForm;
	enum snapfield
	{
		snap_oui = 0,
		snap_type,

		snap_fieldCount
	};

public:
	SnapProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
	virtual ~SnapProtocol();

	static AbstractProtocol* createInstance(StreamBase *stream,
		AbstractProtocol *parent = 0);
	virtual quint32 protocolNumber() const;

	virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
	virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

	virtual QString name() const;
	virtual QString shortName() const;

	virtual ProtocolIdType protocolIdType() const;
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
