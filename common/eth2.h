#ifndef _ETH2_H
#define _ETH2_H

#include "abstractprotocol.h"

#include "eth2.pb.h"
#include "ui_eth2.h"

class Eth2ConfigForm : public QWidget, public Ui::eth2
{
	Q_OBJECT
public:
	Eth2ConfigForm(QWidget *parent = 0);
};

class Eth2Protocol : public AbstractProtocol
{
private:
	OstProto::Eth2	data;
	Eth2ConfigForm	*configForm;
	enum eth2field
	{
		eth2_type = 0,

		eth2_fieldCount
	};

public:
	Eth2Protocol(StreamBase *stream, AbstractProtocol *parent = 0);
	virtual ~Eth2Protocol();

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

	virtual QWidget* configWidget();
	virtual void loadConfigWidget();
	virtual void storeConfigWidget();
};

#endif
