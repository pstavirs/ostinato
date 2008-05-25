#include "portstatsfilterdialog.h"

PortStatsFilterDialog::PortStatsFilterDialog(QWidget *parent)
{
	setupUi(this);

	// TODO(MED): Use ExtendedSelection and use "selected" instead of
	// "current" for selecting in/out
	// TODO(MED): Ensure items are READ-ONLY not editable
	// TODO(MED): Enable "double-click" on items
	lvUnselected->setSelectionMode(QAbstractItemView::SingleSelection);
	lvSelected->setSelectionMode(QAbstractItemView::SingleSelection);

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

	item = mUnselected.takeItem(lvUnselected->currentIndex().row());
	if (mUnselected.removeRow(lvUnselected->currentIndex().row()))
		mSelected.appendRow(item);
}

void PortStatsFilterDialog::on_tbSelectOut_clicked()
{
	QStandardItem	*item;

	item = mSelected.takeItem(lvSelected->currentIndex().row());
	if (mSelected.removeRow(lvSelected->currentIndex().row()))
	{
		mUnselected.appendRow(item);
		mUnselected.sort(0);
	}
}

