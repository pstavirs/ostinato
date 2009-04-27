#ifndef _STREAM_H
#define _STREAM_H

#include <QtGlobal>

#include <QString>
#include <QList>
#include "../common/protocol.pb.h"
#include "../common/abstractprotocol.h"

// Convenience Defines FIXME
#define IP_PROTO_ICMP	0x01
#define IP_PROTO_IGMP	0x02
#define IP_PROTO_TCP	0x06
#define IP_PROTO_UDP	0x11

class Stream {

	quint32					mId;
	OstProto::StreamCore	*mCore;
	OstProto::StreamControl	*mControl;

	QList<AbstractProtocol*> mProtocolList;

public:

	void* core() { return mCore; } // FIXME(HI): Debug ONLY
	void loadProtocolWidgets();
	void storeProtocolWidgets();

public:
	enum FrameType {
		e_ft_none,
		e_ft_eth_2,
		e_ft_802_3_raw,
		e_ft_802_3_llc,
		e_ft_snap
	};

	enum DataPatternMode {
		e_dp_fixed_word,
		e_dp_inc_byte,
		e_dp_dec_byte,
		e_dp_random
	};

	enum FrameLengthMode {
		e_fl_fixed,
		e_fl_inc,
		e_fl_dec,
		e_fl_random
	};

	enum L3Proto {
		e_l3_none,
		e_l3_ip,
		e_l3_arp,
	};

	enum L4Proto {
		e_l4_none,
		e_l4_tcp,
		e_l4_udp,
		e_l4_icmp,
		e_l4_igmp,
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

	void protoDataCopyFrom(Stream& stream);

	bool operator < (const Stream &s) const
		{ return(mCore->ordinal() < s.mCore->ordinal()); }


	void getConfig(uint portId, OstProto::Stream &s);
	bool update(OstProto::Stream *stream);

	quint32	id()
		{ return mId;}
	bool setId(quint32 id)
		{ mId = id; return true;}

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

// TODO(HI) : ?????
#if 0
	quint16			dataStartOfs;
#endif

	// Frame Length (includes CRC)
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

	//---------------------------------------------------------------
	// Methods for use by Packet Model
	//---------------------------------------------------------------
	QList<int> frameProtocol();
	void setFrameProtocol(QList<int> protocolList);

	//! Includes ALL protocol headers excluding payload data
	int protocolHeaderSize();
	AbstractProtocol* protocolById(int id);
};

#endif
