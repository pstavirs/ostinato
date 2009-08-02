#ifndef _SAMPLE_H
#define _SAMPLE_H

#include "abstractprotocol.h"

#include "sample.pb.h"
#include "ui_sample.h"

class SampleConfigForm : public QWidget, public Ui::Sample
{
	Q_OBJECT
public:
	SampleConfigForm(QWidget *parent = 0);
private slots:
};

class SampleProtocol : public AbstractProtocol
{
private:
	OstProto::Sample	data;
	SampleConfigForm	*configForm;
	enum samplefield
	{

		sample_fieldCount
	};

public:
	SampleProtocol(StreamBase *stream);
	virtual ~SampleProtocol();

	static AbstractProtocol* createInstance(StreamBase *stream);
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
