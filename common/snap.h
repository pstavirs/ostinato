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
	static SnapConfigForm	*configForm;
	enum snapfield
	{
		snap_oui = 0,

		snap_fieldCount
	};

public:
	SnapProtocol(Stream *parent = 0);
	virtual ~SnapProtocol();

	virtual void protoDataCopyInto(OstProto::Stream &stream);
	virtual void protoDataCopyFrom(const OstProto::Stream &stream);

	virtual QString name() const;
	virtual QString shortName() const;

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
