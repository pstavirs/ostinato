#ifndef _PORT_H
#define _PORT_H

#include <Qt>
#include <QString>
#include <QList>
#include "stream.h"

class Port {
	
	friend class StreamModel;
	friend class PortStatsModel;

public:
	enum AdminStatus	{ AdminDisable, AdminEnable };
	enum OperStatus		{ OperDown, OperUp };
	enum ControlMode	{ ControlShared, ControlExclusive };

private:
	quint32		mPortId;
	quint32		mPortGroupId;
	QList<Stream>	mStreams;
	QString		mName;
	QString		mDescription;
	QString		mUserAlias;			// user defined
	AdminStatus	mAdminStatus;
	OperStatus	mOperStatus;
	ControlMode	mControlMode;
	
	quint32		mPortStats[10];		// FIXME(HI):Hardcoding

public:
	// FIXME(HIGH): default args is a hack for QList operations on Port
	Port(quint32 id = 0xFFFFFFFF, quint32 pgId = 0xFFFFFFFF);

	quint32 id() const { return mPortId; }
	quint32 portGroupId() const { return mPortGroupId; }
	const QString& name() const { return mName; }
	const QString& description() const { return mDescription; }
	const QString& userAlias() const { return mUserAlias; }

	void setName(QString &name) { mName = name; }
	void setName(const char* name) { mName = QString(name); }
	void setDescription(QString &description) { mDescription = description; }
	void setDescription(const char *description) 
		{ mDescription = QString(description); }

	//void setAdminEnable(AdminStatus status) { mAdminStatus = status; }
	void setAlias(QString &alias) { mUserAlias = alias; }
	//void setExclusive(bool flag);
	// FIXME(HIGH): Only for testing
	void insertDummyStreams();
};

#endif
