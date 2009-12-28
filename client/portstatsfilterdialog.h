#ifndef _PORT_STATS_FILTER_DIALOG_H
#define _PORT_STATS_FILTER_DIALOG_H

#include <QDialog>
#include <QAbstractItemModel>
#include <QStandardItemModel>
#include "ui_portstatsfilter.h"
#include "portgrouplist.h"

class PortStatsFilterDialog : public QDialog, public Ui::PortStatsFilterDialog
{
    Q_OBJECT

public:
    PortStatsFilterDialog(QWidget *parent = 0);
    QList<uint> getItemList(bool* ok, QAbstractItemModel *model,
        Qt::Orientation orientation = Qt::Vertical, 
        QList<uint> initial = QList<uint>());

private:
    enum ItemRole {
        PositionRole = Qt::UserRole + 1
    };
    QStandardItemModel    mUnselected;
    QStandardItemModel    mSelected;

private slots:
    void on_tbSelectIn_clicked();
    void on_tbSelectOut_clicked();
    void on_lvUnselected_doubleClicked(const QModelIndex &index);
    void on_lvSelected_doubleClicked(const QModelIndex &index);
};

#endif

