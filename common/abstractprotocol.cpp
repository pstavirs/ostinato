#include <qendian.h>
#include "abstractprotocol.h" 

/*!
  \class AbstractProtocol

  // FIXME - update this text
  Bare Minimum set of methods that a subclass needs to reimplement
  - protoDataCopyInto() [pure virtual]
  - protoDataCopyFrom() [pure virtual]
  - fieldCount()

  Any useful protocol should also provide implementations for
  - name()
  - shortName()
  - fieldName()

  Protocols with meta fields should additionally implement
  - metaFieldCount()
  - isMetaField()
*/
AbstractProtocol::AbstractProtocol(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *parent)
		: frameProtocols(frameProtoList)
{
	qDebug("%s: &frameproto = %p/%p (sz:%d)", __FUNCTION__, &frameProtocols, &frameProtoList, frameProtocols.size());
	stream = parent;
	metaCount = -1;
	protoSize = -1;
}

AbstractProtocol::~AbstractProtocol()
{
}

AbstractProtocol* AbstractProtocol::createInstance(
	ProtocolList &frameProtoList,
	OstProto::StreamCore *streamCore)
{
	return NULL;
}

/*!
  \fn virtual void protoDataCopyInto(OstProto::OstProto::StreamCore &stream) = 0;

  Copy the protocol's protobuf into the passed in stream \n
  In the base class this is a pure virtual function. Subclasses should
  implement this function by using - \n
  stream.AddExtension(<ExtId>)->CopyFrom(<protobuf_data>) */

/*
   \fn virtual void protoDataCopyFrom(const OstProto::OstProto::StreamCore &stream) = 0;
   FIXME */

/*! Returns the full name of the protocol \n
  The default implementation returns a null string */
QString AbstractProtocol::name() const
{
	return QString(); 
}

/*! Returns the short name or abbreviation of the protocol \n
  The default implementation forms and returns a abbreviation composed
  of all the upper case chars in name() \n 
  The default implementation caches the abbreviation on its first invocation
  and subsequently returns the cached abbreviation */
QString AbstractProtocol::shortName() const
{
	if (protoAbbr.isNull())
	{
		QString abbr;

		for (int i = 0; i < name().size(); i++)
			if (name().at(i).isUpper()) abbr.append(name().at(i));

		if (abbr.size())
			protoAbbr = abbr;
		else
			protoAbbr = QString("");
	}

	return protoAbbr;
}

/*! Returns the number of fields (both Frame and Meta fields) \n
  The default implementation returns zero */
int	AbstractProtocol::fieldCount() const
{
	return 0;
}

/*! Returns the number of meta fields \n
  The default implementation counts and returns the number of fields for which
  fieldData(index, FieldIsMeta) return true\n
  The default implementation caches the count on its first invocation
  and subsequently returns the cached count */
int	AbstractProtocol::metaFieldCount() const
{
	if (metaCount < 0)
	{
		int c = 0;
		for (int i = 0; i < fieldCount() ; i++) 
			if (fieldData(i, FieldIsMeta).toBool())
				c++;
		metaCount = c;
	}

	return metaCount;
}

/*! Returns the number of frame fields \n
  Convenience method - same as fieldCount() minus metaFieldCount() */
int	AbstractProtocol::frameFieldCount() const
{
	//qDebug("%s:%d, %d", __FUNCTION__, fieldCount(), metaFieldCount());
	return (fieldCount() - metaFieldCount());
}

/*! Returns the requested field attribute data \n
  Protocols which have meta fields that vary a frame field across
  streams may use the streamIndex to return the appropriate field value \n
  Some field attriubutes e.g. FieldName may be invariant across streams\n
  The default implementation returns the fieldValue() converted to string 
  The FieldTextValue attribute may include additional information about
  the field's value e.g. a checksum field may include "(correct)" or 
  "(incorrect)" alongwith the actual checksum value. \n
  The default implementation returns FIXME

  \note If a subclass uses any of the below functions to derive 
  FieldFrameValue, the subclass should handle and return a value for 
  FieldBitSize to prevent endless recrusion -
  - protocolFrameCksum()
  */
QVariant AbstractProtocol::fieldData(int index, FieldAttrib attrib,
		int streamIndex) const
{
	switch (attrib)
	{
		case FieldName:
			return QString();
		case FieldBitSize:
			return fieldData(index, FieldFrameValue, streamIndex).
				toByteArray().size() * 8;
		case FieldValue:
			return 0;
		case FieldFrameValue:
			return QByteArray();
		case FieldTextValue:
			return QString();
		case FieldIsMeta:
			return false;

		default:
			qFatal("%s:%d: unhandled case %d\n", __FUNCTION__, __LINE__,
					attrib);
	}

	return QVariant();
}

/*! Sets the value of a field corresponding to index \n
 Returns true if field is successfully set, false otherwise \n
 The default implementation always returns false */
bool AbstractProtocol::setFieldData(int index, const QVariant &value,
		FieldAttrib attrib)
{
	return false;
}

quint32 AbstractProtocol::protocolId(ProtocolIdType type) const
{
	return 0;
}

quint32 AbstractProtocol::payloadProtocolId(ProtocolIdType type) const
{
	quint32 id = 0xFFFFFFFF;
	QLinkedListIterator<const AbstractProtocol*> iter(frameProtocols);

	if (iter.findNext(this))
	{
		if (iter.hasNext())
			id = iter.next()->protocolId(type);
	}

	qDebug("%s: payloadProtocolId = %u", __FUNCTION__, id);
	return id;
}

int AbstractProtocol::protocolFrameSize() const
{
	if (protoSize < 0)
	{
		int bitsize = 0;

		for (int i = 0; i < fieldCount(); i++)
		{
			if (!fieldData(i, FieldIsMeta).toBool())
				bitsize += fieldData(i, FieldBitSize).toUInt();
		}
		protoSize = (bitsize+7)/8;
	}

	qDebug("%s: protoSize = %d", __FUNCTION__, protoSize);
	return protoSize;
}

int AbstractProtocol::protocolFrameOffset() const
{
	int size = 0;
	QLinkedListIterator<const AbstractProtocol*> iter(frameProtocols);

	if (iter.findNext(this))
	{
		iter.previous();
		while (iter.hasPrevious())
			size += iter.previous()->protocolFrameSize();
	}
	else
		return -1;

	qDebug("%s: ofs = %d", __FUNCTION__, size);
	return size;
}

int AbstractProtocol::protocolFramePayloadSize() const
{
	int size = 0;

	QLinkedListIterator<const AbstractProtocol*> iter(frameProtocols);

	if (iter.findNext(this))
	{
		while (iter.hasNext())
			size += iter.next()->protocolFrameSize();
	}
	else
		return -1;

	qDebug("%s: payloadSize = %d", __FUNCTION__, size);
	return size;
}


/*! Returns a byte array encoding the protocol (and its fields) which can be
  inserted into the stream's frame
  The default implementation forms and returns an ordered concatenation of 
  the FrameValue of all the 'frame' fields of the protocol taking care of fields
  which are not an integral number of bytes\n */
QByteArray AbstractProtocol::protocolFrameValue(int streamIndex) const
{
	QByteArray proto, field;
	int bits, lastbitpos = 0;

	for (int i=0; i < fieldCount() ; i++) 
	{
		if (!fieldData(i, FieldIsMeta).toBool())
		{
			field = fieldData(i, FieldFrameValue, streamIndex).toByteArray();
			bits = fieldData(i, FieldBitSize, streamIndex).toUInt();
			if (bits == 0)
				continue;

			qDebug("<<< %d, %d >>>>", proto.size(), field.size());

			if (bits == field.size() * 8)
			{
				if (lastbitpos == 0)
					proto.append(field);
				else
				{
					Q_ASSERT(field.size() > 0);

					char c = proto[proto.size() - 1];
					proto[proto.size() - 1] =  c | (field.at(0) >> lastbitpos);
					for (int j = 0; j < field.size() - 1; j++)
						proto.append(field.at(j) << lastbitpos |
								field.at(j+1) >> lastbitpos);
				}
			}
			else if (bits < field.size() * 8)
			{
				int u, v;

				u = bits / 8;
				v = bits % 8;
				if (lastbitpos == 0)
				{
					proto.append(field.left(u+1));
					char c = proto[proto.size() - 1];
					proto[proto.size() - 1] = c & (0xFF << (8 - v));
					lastbitpos = v;
				}
				else
				{
					char c = proto[proto.size() - 1];
					proto[proto.size() - 1] = c | (field.at(0) >> lastbitpos);
					for (int j = 0; j < (u - 1); j++)
						proto.append(field.at(j) << lastbitpos |
								field.at(j+1) >> lastbitpos);
					if (u)
						proto.append( field.at(u) & (0xFF << (8 - v)) );
					lastbitpos = (lastbitpos + bits) % 8;
				}
			}
			else // if (bits > field.size() * 8)
			{
				qFatal("bitsize more than FrameValue size. skipping...");
				continue;
			}
		}
	}

	return proto;
}

QVariant AbstractProtocol::protocolFrameCksum() const
{
	QByteArray fv;
	quint16 *ip;
	quint32 len, sum = 0;

	fv = protocolFrameValue(-1);
	ip = (quint16*) fv.constData();
	len = fv.size();

	while(len > 1)
	{
		sum += *ip;
		if(sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);
		ip++;
		len -= 2;
	}

	if (len)
		sum += (unsigned short) *(unsigned char *)ip;

	while(sum>>16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return qFromBigEndian((quint16) ~sum);
}

QVariant AbstractProtocol::protocolFramePayloadCksum() const
{
	int cksum = 0;
	QLinkedListIterator<const AbstractProtocol*> iter(frameProtocols);

	if (iter.findNext(this))
	{
		while (iter.hasNext())
			cksum += iter.next()->protocolFrameCksum().toUInt(); // TODO: chg to partial
	}
	else
		return -1;
#if 0
	while(cksum>>16)
		cksum = (cksum & 0xFFFF) + (cksum >> 16);

	return (quint16) ~cksum;
#endif

	return cksum;
}

