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
	static Dot3ConfigForm	*configForm;
	enum Dot3field
	{
		dot3_length,

		dot3_fieldCount
	};

public:
	Dot3Protocol(ProtocolList &frameProtoList, 
		OstProto::StreamCore *parent = 0);
	virtual ~Dot3Protocol();

	static AbstractProtocol* createInstance(
		ProtocolList &frameProtoList,
		OstProto::StreamCore *streamCore = 0);

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
