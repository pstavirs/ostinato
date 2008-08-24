#ifndef _PORT_H
#define _PORT_H

#include <Qt>
#include <QString>
#include <QList>
#include "stream.h"

//class StreamModel;

class Port {

	//friend class StreamModel;

private:
	static uint		mAllocStreamId;
	OstProto::Port	d;

	// FIXME(HI): consider removing mPortId as it is duplicated inside 'd'
	quint32		mPortId;
	quint32		mPortGroupId;
	QString		mUserAlias;			// user defined

	QList<quint32>	mLastSyncStreamList;
	QList<Stream>	mStreams;		// sorted by stream's ordinal value

	uint newStreamId();
	void updateStreamOrdinalsFromIndex();
	void reorderStreamsByOrdinals();
public:
	enum AdminStatus	{ AdminDisable, AdminEnable };
	enum OperStatus		{ OperDown, OperUp };
	enum ControlMode	{ ControlShared, ControlExclusive };

	// FIXME(HIGH): default args is a hack for QList operations on Port
	Port(quint32 id = 0xFFFFFFFF, quint32 pgId = 0xFFFFFFFF);

	quint32 portGroupId() const { return mPortGroupId; }
	const QString& userAlias() const { return mUserAlias; }

	quint32 id() const 
		{ return d.port_id().id(); }
	const QString name() const 
		{ return QString().fromStdString(d.name()); }
	const QString description() const 
		{ return QString().fromStdString(d.description()); }
	AdminStatus adminStatus() 
		{ return (d.is_enabled()?AdminEnable:AdminDisable); }
	OperStatus operStatus()
		{ return (d.is_oper_up()?OperUp:OperDown); }
	ControlMode controlMode() 
		{ return (d.is_exclusive_control()?ControlExclusive:ControlShared); }

#if 0
	void setName(QString &name) { d.name; }
	void setName(const char* name) { mName = QString(name); }
	void setDescription(QString &description) { mDescription = description; }
	void setDescription(const char *description) 
		{ mDescription = QString(description); }
#endif
	//void setAdminEnable(AdminStatus status) { mAdminStatus = status; }
	void setAlias(QString &alias) { mUserAlias = alias; }
	//void setExclusive(bool flag);

	int numStreams() { return mStreams.size(); }
	Stream& streamByIndex(int index)
	{
		Q_ASSERT(index < mStreams.size());
		return mStreams[index];
	}

	// FIXME(MED): naming inconsistency - PortConfig/Stream; also retVal
	void updatePortConfig(OstProto::Port *port);
	
	//! Used by StreamModel
	//@{
	bool newStreamAt(int index);
	bool deleteStreamAt(int index);
	//@}

	//! Used by MyService::Stub to update from config received from server
	//@{
	bool insertStream(uint streamId);
	bool updateStream(uint streamId, OstProto::Stream *stream);
	//@}

	void getDeletedStreamsSinceLastSync(OstProto::StreamIdList &streamIdList);
	void getNewStreamsSinceLastSync(OstProto::StreamIdList &streamIdList);
	void getModifiedStreamsSinceLastSync(
		OstProto::StreamConfigList &streamConfigList);

	void when_syncComplete();
};

#endif
