/*
Copyright (C) 2016 Srivats P.

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

#include "streamstatsmodel.h"

#include "protocol.pb.h"

StreamStatsModel::StreamStatsModel(QObject *parent)
    : QStringListModel(parent)
{
}

void StreamStatsModel::clearStats()
{
    stats_.clear();
    setStringList(stats_);
}

void StreamStatsModel::appendStreamStatsList(
        quint32 /*portGroupId*/,
        const OstProto::StreamStatsList *stats)
{
    stats_.append(stats->DebugString().c_str());
    setStringList(stats_);

    // Prevent receiving any future updates from this sender
    disconnect(sender(), 0, this, 0);
}
