#include <qendian.h>
#include <QHostAddress>

#include "stream.h"

Stream::Stream()
{
	//mId = 0xFFFFFFFF;
	mCore->set_is_enabled(true);

	QList<int> protoList;
	protoList.append(51);
	protoList.append(52);
	setFrameProtocol(protoList);
}

Stream::~Stream()
{
}

void Stream::loadProtocolWidgets()
{
	protocols.loadConfigWidgets();
}

void Stream::storeProtocolWidgets()
{
	protocols.storeConfigWidgets();
}
