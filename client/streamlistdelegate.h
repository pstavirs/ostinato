#ifndef STREAM_LIST_DELEGATE_H
#define STREAM_LIST_DELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>

class StreamListDelegate : public QItemDelegate
{
 Q_OBJECT

public:
	StreamListDelegate(QObject *parent = 0);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
		const QModelIndex &index) const;

	void setEditorData(QWidget *editor, const QModelIndex &index) const;
	void setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const;

	void updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &index) const;

	bool editorEvent(QEvent *event, QAbstractItemModel *model,
		const QStyleOptionViewItem &option, const QModelIndex &index);
};

#endif 

