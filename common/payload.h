#ifndef _PAYLOAD_H
#define _PAYLOAD_H

#include "abstractprotocol.h"

#include "payload.pb.h"
#include "ui_payload.h"

class PayloadConfigForm : public QWidget, public Ui::payload
{
	Q_OBJECT
public:
	PayloadConfigForm(QWidget *parent = 0);
private slots:
	void on_cmbPatternMode_currentIndexChanged(int index);
};

class PayloadProtocol : public AbstractProtocol
{
private:
	OstProto::Payload			data;
	static PayloadConfigForm	*configForm;
	enum payloadfield
	{
		payload_dataPattern,

		// Meta fields
		payload_dataPatternMode,

		payload_fieldCount
	};

public:
	PayloadProtocol(ProtocolList &frameProtoList, 
		OstProto::StreamCore *parent = 0);
	virtual ~PayloadProtocol();

	static AbstractProtocol* createInstance(
		ProtocolList &frameProtoList,
		OstProto::StreamCore *streamCore = 0);

	virtual void protoDataCopyInto(OstProto::Stream &stream);
	virtual void protoDataCopyFrom(const OstProto::Stream &stream);

	virtual QString name() const;
	virtual QString shortName() const;

	virtual int protocolFrameSize() const;

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
