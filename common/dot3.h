#ifndef _DOT3_H
#define _DOT3_H

#include "abstractprotocol.h"

#include "dot3.pb.h"
#include "ui_Dot3.h"

class Dot3ConfigForm : public QWidget, public Ui::dot3
{
	Q_OBJECT
public:
	Dot3ConfigForm(QWidget *parent = 0);
};

class Dot3Protocol : public AbstractProtocol
{
private:
	OstProto::Dot3	data;
	Dot3ConfigForm	*configForm;
	enum Dot3field
	{
		dot3_length,

		dot3_fieldCount
	};

public:
	Dot3Protocol(StreamBase *stream, AbstractProtocol *parent = 0);
	virtual ~Dot3Protocol();

	static AbstractProtocol* createInstance(StreamBase *stream,
		AbstractProtocol *parent = 0);
	virtual quint32 protocolNumber() const;

	virtual void protoDataCopyInto(OstProto::Protocol &protocol) const;
	virtual void protoDataCopyFrom(const OstProto::Protocol &protocol);

	virtual QString name() const;
	virtual QString shortName() const;

	virtual int	fieldCount() const;

	virtual QVariant fieldData(int index, FieldAttrib attrib,
		   	int streamIndex = 0) const;
	virtual bool setFieldData(int index, const QVariant &value, 
			FieldAttrib attrib = FieldValue);

	virtual bool isProtocolFrameValueVariable() const;

	virtual QWidget* configWidget();
	virtual void loadConfigWidget();
	virtual void storeConfigWidget();
};

#endif
