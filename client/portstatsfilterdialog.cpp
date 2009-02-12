#include "portstatsfilterdialog.h"

PortStatsFilterDialog::PortStatsFilterDialog(QWidget *parent)
{
	setupUi(this);

	mUnselected.setSortRole(PositionRole);

	lvUnselected->setModel(&mUnselected);
	lvSelected->setModel(&mSelected);
}

QList<uint> PortStatsFilterDialog::getItemList(bool* ok, 
	QAbstractItemModel *model, Qt::Orientation orientation,
	QList<uint> initial)
{
	QList<uint>	ret;

	uint count = (orientation == Qt::Vertical) ? 
		model->rowCount() : model->columnCount();

	*ok = false;

	mUnselected.clear();
	mSelected.clear();

	for (uint i = 0; i < count; i++)
	{
		QStandardItem	*item;
		
		item = new QStandardItem(model->headerData(i, orientation).toString());
		item->setData(i, PositionRole);
		item->setFlags(Qt::ItemIsSelectable
				| Qt::ItemIsDragEnabled
				//| Qt::ItemIsDropEnabled
				| Qt::ItemIsEnabled);

		if (initial.contains(i))
			mSelected.appendRow(item);
		else
			mUnselected.appendRow(item);
	}

	// No need to sort right now 'coz we have inserted items in order

	if (exec() == QDialog::Accepted)
	{
		uint count = mSelected.rowCount();
		for (uint i = 0; i < count; i++)
		{
			QModelIndex index = mSelected.index(i, 0, QModelIndex());
			QStandardItem *item = mSelected.itemFromIndex(index);
			ret.append(item->data(PositionRole).toInt());
		}
		*ok = true;
	}

	return ret;
}

void PortStatsFilterDialog::on_tbSelectIn_clicked()
{
	QStandardItem	*item;
	while (lvUnselected->selectionModel()->selectedIndexes().size())
	{
		item = mUnselected.takeItem(lvUnselected->selectionModel()->
				selectedIndexes().at(0).row());
		if (mUnselected.removeRow(lvUnselected->selectionModel()->
				selectedIndexes().at(0).row()))
			mSelected.appendRow(item);
	}
}

void PortStatsFilterDialog::on_tbSelectOut_clicked()
{
	QStandardItem	*item;

	while (lvSelected->selectionModel()->selectedIndexes().size())
	{
		item = mSelected.takeItem(lvSelected->selectionModel()->
				selectedIndexes().at(0).row());
		if (mSelected.removeRow(lvSelected->selectionModel()->
				selectedIndexes().at(0).row()))
		{
			mUnselected.appendRow(item);
			mUnselected.sort(0);
		}
	}
}

void PortStatsFilterDialog::on_lvUnselected_doubleClicked(const QModelIndex &index)
{
	QStandardItem	*item;

	item = mUnselected.takeItem(lvUnselected->currentIndex().row());
	if (mUnselected.removeRow(lvUnselected->currentIndex().row()))
		mSelected.appendRow(item);
}

void PortStatsFilterDialog::on_lvSelected_doubleClicked(const QModelIndex &index)
{
	QStandardItem	*item;

	item = mSelected.takeItem(lvSelected->currentIndex().row());
	if (mSelected.removeRow(lvSelected->currentIndex().row()))
	{
		mUnselected.appendRow(item);
		mUnselected.sort(0);
	}
}

