#ifndef _PORT_H
#define _PORT_H

#include <Qt>
#include <QString>
#include <QList>
#include "stream.h"

class StreamModel;

class Port {

#if 0 // PB
	friend class PortStatsModel;
#endif
	friend class StreamModel;

	//friend class PbHelper;

	// FIXME: non-friend mechanism
	//friend QList<Stream>* StreamModel::currentPortStreamList(void); 

private:
	OstProto::PortConfig	d;

	quint32		mPortId;
	quint32		mPortGroupId;
	QString		mUserAlias;			// user defined

	QList<Stream>	mStreams;

#if 0 // PB
	quint32		mPortId;
	QString		mName;
	QString		mDescription;
	AdminStatus	mAdminStatus;
	OperStatus	mOperStatus;
	ControlMode	mControlMode;
	
	quint32		mPortStats[10];		// FIXME(HI):Hardcoding
#endif

public:
	enum AdminStatus	{ AdminDisable, AdminEnable };
	enum OperStatus		{ OperDown, OperUp };
	enum ControlMode	{ ControlShared, ControlExclusive };

	// FIXME(HIGH): default args is a hack for QList operations on Port
	Port(quint32 id = 0xFFFFFFFF, quint32 pgId = 0xFFFFFFFF);

	quint32 portGroupId() const { return mPortGroupId; }
	const QString& userAlias() const { return mUserAlias; }

	quint32 id() const 
		{ return d.port_id(); }
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

	void updatePortConfig(OstProto::PortConfig *portConfig);

	// FIXME(HIGH): Only for testing
	void insertDummyStreams();
};

#endif
