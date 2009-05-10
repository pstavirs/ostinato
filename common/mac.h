#ifndef _MAC_H
#define _MAC_H

#include "abstractprotocol.h"

#include "mac.pb.h"
#include "ui_mac.h"

#define MAX_MAC_ITER_COUNT  256

class MacConfigForm : public QWidget, public Ui::mac
{
	Q_OBJECT
public:
	MacConfigForm(QWidget *parent = 0);
private slots:
	void on_cmbDstMacMode_currentIndexChanged(int index);
	void on_cmbSrcMacMode_currentIndexChanged(int index);
};

class MacProtocol : public AbstractProtocol
{
private:
	OstProto::Mac	data;
	static MacConfigForm	*configForm;
	enum macfield
	{
		mac_dstAddr = 0,
		mac_srcAddr,

		mac_dstMacMode,
		mac_dstMacCount,
		mac_dstMacStep,
		mac_srcMacMode,
		mac_srcMacCount,
		mac_srcMacStep,

		mac_fieldCount
	};

public:
	MacProtocol(ProtocolList &frameProtoList, 
		OstProto::StreamCore *parent = 0);
	virtual ~MacProtocol();

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
