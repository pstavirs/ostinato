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
	static SampleConfigForm	*configForm;
	enum samplefield
	{

		sample_fieldCount
	};

public:
	SampleProtocol(ProtocolList &frameProtoList, 
		OstProto::StreamCore *parent = 0);
	virtual ~SampleProtocol();

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
