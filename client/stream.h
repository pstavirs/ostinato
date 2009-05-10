#ifndef _STREAM_H
#define _STREAM_H

#include <QtGlobal>
#include <QString>
#include <QList>

#include "../common/protocol.pb.h"
#include "../common/streambase.h"

class Stream : public StreamBase {

	//quint32					mId;

public:
	void loadProtocolWidgets();
	void storeProtocolWidgets();

public:
	enum FrameLengthMode {
		e_fl_fixed,
		e_fl_inc,
		e_fl_dec,
		e_fl_random
	};

	enum SendUnit {
		e_su_packets,
		e_su_bursts
	};

	enum SendMode {
		e_sm_fixed,
		e_sm_continuous
	};

	enum NextWhat {
		e_nw_stop,
		e_nw_goto_next,
		e_nw_goto_id
	};

	// -------------------------------------------------------
	// Methods
	// -------------------------------------------------------
	Stream();
	~Stream();

	// TODO: Below methods move to StreamBase???

	bool operator < (const Stream &s) const
		{ return(mCore->ordinal() < s.mCore->ordinal()); }

	quint32	id()
		{ return mStreamId->id();}
	bool setId(quint32 id)
		{ mStreamId->set_id(id); return true;}

#if 0 // FIXME(HI): needed?
	quint32	portId()
		{ return mCore->port_id();}
	bool setPortId(quint32 id)
		{ mCore->set_port_id(id); return true;}
#endif

	quint32	ordinal()
		{ return mCore->ordinal();}
	bool setOrdinal(quint32	ordinal)
		{ mCore->set_ordinal(ordinal); return true; }

	bool isEnabled() const
		{ return mCore->is_enabled(); }
	bool setIsEnabled(bool flag)
		{ mCore->set_is_enabled(flag); return true; }

	const QString name() const 
		{ return QString().fromStdString(mCore->name()); }
	bool setName(QString name) 
		{ mCore->set_name(name.toStdString()); return true; }

	// Frame Length (includes FCS)
	FrameLengthMode	lenMode()
		{ return (FrameLengthMode) mCore->len_mode(); }
	bool setLenMode(FrameLengthMode	lenMode)
		{ mCore->set_len_mode(
			(OstProto::StreamCore::FrameLengthMode) lenMode); return true; }

	quint16	frameLen()
		{ return mCore->frame_len(); }
	bool setFrameLen(quint16 frameLen)
		{ mCore->set_frame_len(frameLen);  return true; }

	quint16	frameLenMin()
		{ return mCore->frame_len_min(); }
	bool setFrameLenMin(quint16 frameLenMin)
		{ mCore->set_frame_len_min(frameLenMin);  return true; }

	quint16	frameLenMax()
		{ return mCore->frame_len_max(); }
	bool setFrameLenMax(quint16 frameLenMax)
		{ mCore->set_frame_len_max(frameLenMax);  return true; }

	SendUnit sendUnit()
		{ return (SendUnit) mControl->unit(); }
	bool setSendUnit(SendUnit sendUnit)
		{ mControl->set_unit(
			(OstProto::StreamControl::SendUnit) sendUnit); return true; }

	SendMode sendMode()
		{ return (SendMode) mControl->mode(); }
	bool setSendMode(SendMode sendMode)
		{ mControl->set_mode(
			(OstProto::StreamControl::SendMode) sendMode); return true; }

	NextWhat nextWhat()
		{ return (NextWhat) mControl->next(); }
	bool setNextWhat(NextWhat nextWhat)
		{ mControl->set_next(
			(OstProto::StreamControl::NextWhat) nextWhat); return true; }

	quint32 numPackets()
		{ return (quint32) mControl->num_packets(); }
	bool setNumPackets(quint32 numPackets)
		{ mControl->set_num_packets(numPackets); return true; }

	quint32 numBursts()
		{ return (quint32) mControl->num_bursts(); }
	bool setNumBursts(quint32 numBursts)
		{ mControl->set_num_bursts(numBursts); return true; }

	quint32 burstSize()
		{ return (quint32) mControl->packets_per_burst(); }
	bool setBurstSize(quint32 packetsPerBurst)
		{ mControl->set_packets_per_burst(packetsPerBurst); return true; }

	quint32 packetRate()
		{ return (quint32) mControl->packets_per_sec(); }
	bool setPacketRate(quint32 packetsPerSec)
		{ mControl->set_packets_per_sec(packetsPerSec); return true; }

	quint32 burstRate()
		{ return (quint32) mControl->bursts_per_sec(); }
	bool setBurstRate(quint32 burstsPerSec)
		{ mControl->set_bursts_per_sec(burstsPerSec); return true; }
};

#endif
