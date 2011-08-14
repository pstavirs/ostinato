/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "abstractport.h"

#include <QString>
#include <QIODevice>

#include "../common/streambase.h"
#include <math.h>

AbstractPort::AbstractPort(int id, const char *device)
{
    isUsable_ = true;
    data_.mutable_port_id()->set_id(id);
    data_.set_name(device);

    //! \todo (LOW) admin enable/disable of port
    data_.set_is_enabled(true);

    data_.set_is_exclusive_control(false);

    isSendQueueDirty_ = false;
    linkState_ = OstProto::LinkStateUnknown;

    memset((void*) &stats_, 0, sizeof(stats_));
    resetStats();
}

AbstractPort::~AbstractPort()
{
}    

void AbstractPort::init()
{
}    

bool AbstractPort::modify(const OstProto::Port &port)
{
    bool ret = false;

    //! \todo Use reflection to find out which fields are set
    if (port.has_is_exclusive_control())
    {
        bool val = port.is_exclusive_control();

        ret = setExclusiveControl(val);
        if (ret)
            data_.set_is_exclusive_control(val);
    }

    return ret;
}    

StreamBase* AbstractPort::streamAtIndex(int index)
{
    Q_ASSERT(index < streamList_.size());
    return streamList_.at(index);
}

StreamBase* AbstractPort::stream(int streamId)
{
    for (int i = 0; i < streamList_.size(); i++)
    {
        if ((uint)streamId == streamList_.at(i)->id())
            return streamList_.at(i);
    }

    return NULL;
}

bool AbstractPort::addStream(StreamBase *stream)
{
    streamList_.append(stream);
    isSendQueueDirty_ = true;
    return true;
}

bool AbstractPort::deleteStream(int streamId)
{
    for (int i = 0; i < streamList_.size(); i++)
    {
        StreamBase *stream;

        if ((uint)streamId == streamList_.at(i)->id())
        {
            stream = streamList_.takeAt(i);
            delete stream;
            
            isSendQueueDirty_ = true;
            return true;
        }
    }

    return false;
}

void AbstractPort::updatePacketList()
{
    int     len;
    bool    isVariable;
    long    sec = 0; 
    long    nsec = 0;

    qDebug("In %s", __FUNCTION__);

    // First sort the streams by ordinalValue
    qSort(streamList_.begin(), streamList_.end(), StreamBase::StreamLessThan);

    clearPacketList();

    for (int i = 0; i < streamList_.size(); i++)
    {
        if (streamList_[i]->isEnabled())
        {
            long numPackets, numBursts;
            double ibg = 0;
            long ibg1 = 0, ibg2 = 0;
            long nb1 = 0, nb2 = 0;
            double ipg = 0;
            long ipg1 = 0, ipg2 = 0;
            long np1 = 0, np2 = 0;

            switch (streamList_[i]->sendUnit())
            {
            case OstProto::StreamControl::e_su_bursts:
                numBursts = streamList_[i]->numBursts();
                numPackets = streamList_[i]->burstSize();
                if (streamList_[i]->burstRate() > 0)
                {
                    ibg = 1e9/double(streamList_[i]->burstRate());
                    ibg1 = long(ceil(ibg));
                    ibg2 = long(floor(ibg));
                    nb1 = long((ibg - double(ibg2)) * double(numBursts));
                    nb2= numBursts - nb1;
                }
                break;
            case OstProto::StreamControl::e_su_packets:
                numBursts = 1;
                numPackets = streamList_[i]->numPackets();
                if (streamList_[i]->packetRate() > 0)
                {
                    ipg = 1e9/double(streamList_[i]->packetRate());
                    ipg1 = long(ceil(ipg));
                    ipg2 = long(floor(ipg));
                    np1 = long((ipg - double(ipg2)) * double(numPackets));
                    np2= numPackets - np1;
                }
                break;
            default:
                qWarning("Unhandled stream control unit %d",
                    streamList_[i]->sendUnit());
                continue;
            }
            qDebug("numBursts = %ld, numPackets = %ld\n",
                    numBursts, numPackets);
            qDebug("ibg = %g, ibg1/nb1 = %ld/%ld ibg2/nb2 = %ld/%ld\n", 
                    ibg, ibg1, nb1, ibg2, nb2);
            qDebug("ipg = %g, ipg1/np1 = %ld/%ld ipg2/np2 = %ld/%ld\n", 
                    ipg, ipg1, np1, ipg2, np2);

            if (streamList_[i]->isFrameVariable())
            {
                isVariable = true;
                len = 0; // avoid compiler warning; get len value for each pkt
            }
            else
            {
                isVariable = false;
                len = streamList_[i]->frameValue(pktBuf_, sizeof(pktBuf_), 0);
            }

            for (int j = 0; j < numBursts; j++)
            {
                for (int k = 0; k < numPackets; k++)
                {
                    if (isVariable)
                    {
                        len = streamList_[i]->frameValue(pktBuf_, 
                                sizeof(pktBuf_), j * numPackets + k);
                    }
                    if (len <= 0)
                        continue;

                    qDebug("q(%d, %d, %d) sec = %lu nsec = %lu",
                            i, j, k, sec, nsec);

                    appendToPacketList(sec, nsec, pktBuf_, len); 

                    nsec += (k < np1) ? ipg1 : ipg2;
                    while (nsec >= 1e9)
                    {
                        sec++;
                        nsec -= long(1e9);
                    }
                } // for (numPackets)

                nsec += (j < nb1) ? ibg1 : ibg2;
                while (nsec >= 1e9)
                {
                    sec++;
                    nsec -= long(1e9);
                }
            } // for (numBursts)

            switch(streamList_[i]->nextWhat())
            {
                case ::OstProto::StreamControl::e_nw_stop:
                    goto _stop_no_more_pkts;

                case ::OstProto::StreamControl::e_nw_goto_id:
                    /*! \todo (MED): define and use 
                    streamList_[i].d.control().goto_stream_id(); */

                    /*! \todo (MED): assumes goto Id is less than current!!!! 
                     To support goto to any id, do
                     if goto_id > curr_id then 
                         i = goto_id;
                         goto restart;
                     else
                         returnToQIdx = 0;
                     */

                    setPacketListLoopMode(true, streamList_[i]->sendUnit() == 
                          StreamBase::e_su_bursts ? ibg1 : ipg1);
                    goto _stop_no_more_pkts;

                case ::OstProto::StreamControl::e_nw_goto_next:
                    break;

                default:
                    qFatal("---------- %s: Unhandled case (%d) -----------",
                            __FUNCTION__, streamList_[i]->nextWhat() );
                    break;
            }

        } // if (stream is enabled)
    } // for (numStreams)

_stop_no_more_pkts:
    isSendQueueDirty_ = false;
}

void AbstractPort::stats(PortStats *stats)
{
    stats->rxPkts = stats_.rxPkts - epochStats_.rxPkts; 
    stats->rxBytes = stats_.rxBytes - epochStats_.rxBytes; 
    stats->rxPps = stats_.rxPps; 
    stats->rxBps = stats_.rxBps; 

    stats->txPkts = stats_.txPkts - epochStats_.txPkts; 
    stats->txBytes = stats_.txBytes - epochStats_.txBytes; 
    stats->txPps = stats_.txPps; 
    stats->txBps = stats_.txBps; 
}
