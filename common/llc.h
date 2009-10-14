#ifndef _LLC_H
#define _LLC_H

#include <QSize>
#include <qdebug.h>

#include "abstractprotocol.h"

#include "llc.pb.h"
#include "ui_llc.h"

class LlcConfigForm : public QWidget, public Ui::llc
{
	Q_OBJECT
public:
	LlcConfigForm(QWidget *parent = 0);
};

class LlcProtocol : public AbstractProtocol
{
private:
	OstProto::Llc	data;
	LlcConfigForm	*configForm;
	enum llcfield
	{
		llc_dsap = 0,
		llc_ssap,
		llc_ctl,

		llc_fieldCount
	};

public:
	LlcProtocol(StreamBase *stream, AbstractProtocol *parent = 0);
	virtual ~LlcProtocol();

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
