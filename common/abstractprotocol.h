#ifndef _ABSTRACT_PROTOCOL_H
#define _ABSTRACT_PROTOCOL_H

#include <QString>
#include <QVariant>
#include <QByteArray>
#include <QWidget>

#include "../common/protocol.pb.h"

#define BASE_BIN (2)
#define BASE_OCT (8)
#define BASE_DEC (10)
#define BASE_HEX (16)

class Stream;

class AbstractProtocol
{
private:
	mutable int		metaCount;
	mutable QString protoAbbr;

protected:
	Stream	*stream;

public:
	enum FieldAttrib {
		FieldName,			//! name
		FieldValue,			//! value in host byte order (user editable)
		FieldTextValue,		//! value as text
		FieldFrameValue,	//! frame encoded value in network byte order
		FieldBitSize,		//! size in bits
		FieldIsMeta			//! bool indicating if field is meta
	};

	AbstractProtocol(Stream *parent = 0);
	virtual ~AbstractProtocol();

	virtual void protoDataCopyInto(OstProto::Stream &stream) = 0;
	virtual void protoDataCopyFrom(const OstProto::Stream &stream) = 0;

	virtual QString name() const;
	virtual QString shortName() const;

	virtual int	fieldCount() const;
	virtual int	metaFieldCount() const;
	int	frameFieldCount() const;

	virtual QVariant fieldData(int index, FieldAttrib attrib,
		   	int streamIndex = 0) const;
	virtual bool setFieldData(int index, const QVariant &value, 
			FieldAttrib attrib = FieldValue);

	QByteArray protocolFrameValue(int streamIndex = 0) const;

	virtual QWidget* configWidget() = 0;
	virtual void loadConfigWidget() = 0;
	virtual void storeConfigWidget() = 0;
};

#endif
