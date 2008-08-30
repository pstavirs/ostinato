#include "dumpview.h"

//public:
DumpView::DumpView(QWidget *parent)
{
	int w, h;

	// NOTE: Monospaced fonts only !!!!!!!!!!!
	setFont(QFont("Courier"));
	w = fontMetrics().width('X');
	h = fontMetrics().height();

	mLineHeight = h;
	mCharWidth = w;

	mSelectedRow = mSelectedCol = -1;

	// calculate width for offset column and the whitespace that follows it
	// 0010   00 00 00 00 00 00 00 00 00   00 00 00 00 00 00 00 00   ........ ........
	mOffsetPaneTopRect = QRect(0, 0, w*4, h);
	mDumpPaneTopRect = QRect(mOffsetPaneTopRect.right()+w*3, 0, 
		w*((8*3-1)+2+(8*3-1)), h);
	mAsciiPaneTopRect = QRect(mDumpPaneTopRect.right()+w*3, 0, 
		w*(8+1+8), h);
	qDebug("DumpView::DumpView");
}

QModelIndex DumpView::indexAt( const QPoint &point ) const
{
#if 0
	int x = point.x();
	int row, col;

	if (x > mAsciiPaneTopRect.left())
	{
		col = (x - mAsciiPaneTopRect.left()) / mCharWidth;
		if (col == 8) // don't select whitespace 
			goto _exit;
		else if (col > 8) // adjust for whitespace
			col--;
	} 
	else if (x > mDumpPaneTopRect.left())
	{
		col = (x - mDumpPaneTopRect.left()) / (mCharWidth*3);
	}
	row = point.y()/mLineHeight;

	if ((col < 16) && (row < ((data.size()+16)/16)))
	{
		selrow = row;
		selcol = col;
	}
	else
		goto _exit;

	// last row check col
	if ((row == (((data.size()+16)/16) - 1)) && (col >= (data.size() % 16)))
		goto _exit;

	qDebug("dumpview::selection(%d, %d)", selrow, selcol);

	offset = selrow * 16 + selcol;
#if 0
	for(int i = 0; i < model()->rowCount(parent); i++)
	{
		QModelIndex index = model()->index(i, 0, parent);

		if (model()->hasChildren(index))
			indexAtOffset(offset, index);	// Non Leaf
		else
			if (
			dump.append(model()->data(index, Qt::UserRole).toByteArray()); // Leaf
		// FIXME: Use RawValueRole instead of UserRole
	}
#endif
}

_exit:
	// Clear existing selection
	selrow = -1;
#endif

	return QModelIndex();
}

void DumpView::scrollTo( const QModelIndex &index, ScrollHint hint )
{
	// FIXME: implement scrolling
}

QRect DumpView::visualRect( const QModelIndex &index ) const
{
	// FIXME: calculate actual rect
	return rect();
}

//protected:
int DumpView::horizontalOffset() const
{
	return horizontalScrollBar()->value();
}

bool DumpView::isIndexHidden( const QModelIndex &index ) const
{
	return false;
}

QModelIndex DumpView::moveCursor( CursorAction cursorAction, 
		Qt::KeyboardModifiers modifiers )
{
	// FIXME(MED): need to implement movement using cursor
	return currentIndex();
}

void DumpView::setSelection( const QRect &rect,
		QItemSelectionModel::SelectionFlags flags )
{
	// FIXME(HI): calculate indexes using rect
	selectionModel()->select(QModelIndex(), flags);
}

int DumpView::verticalOffset() const
{
	return verticalScrollBar()->value();
}

QRegion DumpView::visualRegionForSelection( const QItemSelection &selection ) const
{
	// FIXME(HI)
	return QRegion(rect());
}

//protected slots:
void DumpView::dataChanged( const QModelIndex &topLeft, 
		const QModelIndex &bottomRight )
{
	// FIXME(HI)
	update();
}

void DumpView::selectionChanged( const QItemSelection &selected, 
		const QItemSelection &deselected )
{
	// FIXME(HI)
	update();
}

void DumpView::populateDump(QByteArray &dump, int &selOfs, int &selSize, 
		QModelIndex parent)
{
	// TODO(LOW): Assumption - only single selection - enforce/ensure this

	for(int i = 0; i < model()->rowCount(parent); i++)
	{
		QModelIndex index = model()->index(i, 0, parent);

		if (model()->hasChildren(index))
		{
			// Non Leaf

			// A non-leaf has no dump data of its own but is rather a 
			// container for its children. So we calculate ofs/size based
			// on this fact
			if (selectionModel()->isSelected(index))
			{
				selOfs = dump.size();
				populateDump(dump, selOfs, selSize, index);
				selSize = dump.size() - selOfs;
			}
			else
			{
				populateDump(dump, selOfs, selSize, index);
			}
		}
		else
		{
			// Leaf
			if (selectionModel()->isSelected(index))
			{
				int size, j;

				selOfs = dump.size();
				size = model()->data(index, Qt::UserRole).toByteArray().size();
				
				// Take care of multiple indexes (2 or more) mapping onto 
				// same dump byte(s)
				j = i-1;
				while ((size == 0) && (j >= 0))
				{
					size = model()->data(index.sibling(j,0), Qt::UserRole).
						toByteArray().size();
					selOfs -= size;
					j++;
				}
				selSize = size;
			}
			dump.append(model()->data(index, Qt::UserRole).toByteArray());
		}	
		// FIXME: Use RawValueRole instead of UserRole
	}
}

// TODO(LOW): rewrite this function - it's a mess!
void DumpView::paintEvent(QPaintEvent* event)
{
	QStylePainter	painter(viewport());
	QRect			offsetRect = mOffsetPaneTopRect;
	QRect			dumpRect = mDumpPaneTopRect;
	QRect			asciiRect = mAsciiPaneTopRect;
	QPalette		pal = palette();
	QByteArray		data;
	//QByteArray		ba;
	int				selOfs = -1, selSize;
	int				curSelOfs, curSelSize;

	qDebug("dumpview::paintEvent");

	// FIXME(LOW): unable to set the self widget's font in constructor
	painter.setFont(QFont("Courier"));

	// set a white background
	painter.fillRect(rect(), QBrush(QColor(Qt::white))); 

	if (model())
		populateDump(data, selOfs, selSize);

	// display the offset, dump and ascii panes 8 + 8 bytes on a line
	for (int i = 0; i < data.size(); i+=16)
	{
		QString	dumpStr, asciiStr;	

		//ba = data.mid(i, 16);

		// display offset
		painter.drawItemText(offsetRect, Qt::AlignLeft | Qt::AlignTop, pal,
			true, QString("%1").arg(i, 4, 16, QChar('0')), QPalette::WindowText);
		// construct the dumpStr and asciiStr
		for (int j = i; (j < (i+16)) && (j < data.size()); j++)
		{
			unsigned char c = data.at(j);

			// extra space after 8 bytes
			if (((j+8) % 16) == 0)
			{
				dumpStr.append(" ");
				asciiStr.append(" ");
			}

			dumpStr.append(QString("%1").arg((uint)c, 2, 16, QChar('0')).
				toUpper()).append(" ");

			if (isPrintable(c))
				asciiStr.append(QChar(c));
			else
				asciiStr.append(QChar('.'));
		}

		// display dump
		painter.drawItemText(dumpRect, Qt::AlignLeft | Qt::AlignTop, pal,
			true, dumpStr, QPalette::WindowText);

		// display ascii
		painter.drawItemText(asciiRect, Qt::AlignLeft | Qt::AlignTop, pal,
			true, asciiStr, QPalette::WindowText);

		// if no selection, skip selection painting
		if (selOfs < 0)
			goto _next;

		// Check overlap between current row and selection
		{
			QRect r1(i, 0, qMin(16, data.size()-i), 8);
			QRect s1(selOfs, 0, selSize, 8);
			if (r1.intersects(s1))
			{
				QRect t = r1.intersected(s1);

				curSelOfs = t.x();
				curSelSize = t.width();
			}
			else
				curSelSize = 0;

		}

		// overpaint selection on current row (if any)
		if (curSelSize > 0)
		{
			QRect r;
			QString selectedAsciiStr, selectedDumpStr;

			qDebug("dumpview::paintEvent - Highlighted");

			// construct the dumpStr and asciiStr
			for (int k = curSelOfs; (k < (curSelOfs + curSelSize)); k++)
			{
				unsigned char c = data.at(k);

				// extra space after 8 bytes
				if (((k+8) % 16) == 0)
				{
					// Avoid adding space at the start for fields starting
					// at second column 8 byte boundary
					if (k!=curSelOfs)
					{
						selectedDumpStr.append(" ");
						selectedAsciiStr.append(" ");
					}
				}

				selectedDumpStr.append(QString("%1").arg((uint)c, 2, 16, 
							QChar('0')).toUpper()).append(" ");

				if (isPrintable(c))
					selectedAsciiStr.append(QChar(c));
				else
					selectedAsciiStr.append(QChar('.'));
			}

			// display dump
			r = dumpRect;
			if ((curSelOfs - i) < 8)
				r.translate(mCharWidth*(curSelOfs-i)*3, 0);
			else
				r.translate(mCharWidth*((curSelOfs-i)*3+1), 0);

			// adjust width taking care of selection stretching between
			// the two 8byte columns
			if (( (curSelOfs-i) < 8 ) && ( (curSelOfs-i+curSelSize) > 8 ))
				r.setWidth((curSelSize * 3 + 1) * mCharWidth);
			else
				r.setWidth((curSelSize * 3) * mCharWidth);

			painter.fillRect(r, pal.highlight());
			painter.drawItemText(r, Qt::AlignLeft | Qt::AlignTop, pal,
				true, selectedDumpStr, QPalette::HighlightedText);

			// display ascii
			r = asciiRect;
			if ((curSelOfs - i) < 8)
				r.translate(mCharWidth*(curSelOfs-i)*1, 0);
			else
				r.translate(mCharWidth*((curSelOfs-i)*1+1), 0);

			// adjust width taking care of selection stretching between
			// the two 8byte columns
			if (( (curSelOfs-i) < 8 ) && ( (curSelOfs-i+curSelSize) > 8 ))
				r.setWidth((curSelSize * 1 + 1) * mCharWidth);
			else
				r.setWidth((curSelSize * 1) * mCharWidth);

			painter.fillRect(r, pal.highlight());
			painter.drawItemText(r, Qt::AlignLeft | Qt::AlignTop, pal,
				true, selectedAsciiStr, QPalette::HighlightedText);
		}

_next:	
		// move the rects down
		offsetRect.translate(0, mLineHeight);
		dumpRect.translate(0, mLineHeight);
		asciiRect.translate(0, mLineHeight);
	}	
}

