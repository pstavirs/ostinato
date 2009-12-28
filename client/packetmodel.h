#ifndef _PACKET_MODEL_H
#define _PACKET_MODEL_H

#include <QAbstractItemModel>

class ProtocolListIterator;
class AbstractProtocol;

class PacketModel: public QAbstractItemModel
{

public:
    PacketModel(QObject *parent = 0);
    void setSelectedProtocols(ProtocolListIterator &iter);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, 
        int role = Qt::DisplayRole) const { return QVariant(); } ;
    QModelIndex index (int row, int col, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent(const QModelIndex &index) const;        

private:
    typedef union _IndexId
    {
        quint32    w;
        struct
        {
            quint16    type;
#define ITYP_PROTOCOL    1
#define ITYP_FIELD        2
            quint16    protocol;    // protocol is valid for both ITYPs
        } ws;
    } IndexId;

    QList<const AbstractProtocol*> mSelectedProtocols;
};
#endif

