#include "dumpview.h"

DumpView::DumpView(QWidget *parent)
{
	int w, h;

	data.resize(73);

	// NOTE: Monospaced fonts only !!!!!!!!!!!
	setFont(QFont("Courier"));
	w = fontMetrics().width('X');
	h = fontMetrics().height();

	mLineHeight = h;
	mCharWidth = w;

	mSelectedRow = mSelectedCol = -1;

	// calculate width for offset column and the whitespace that follows it
	mOffsetPaneTopRect = QRect(0, 0, w*4, h);
	mDumpPaneTopRect = QRect(mOffsetPaneTopRect.right()+w*3, 0, 
		w*((8*3-1)+2+(8*3-1)), h);
	mAsciiPaneTopRect = QRect(mDumpPaneTopRect.right()+w*3, 0, 
		w*(8+1+8), h);
	qDebug("DumpView::DumpView");
}

#if 0
QSize DumpView::sizeHint()	const
{
}
#endif

void DumpView::mousePressEvent(QMouseEvent *event)
{
	int x = event->x();
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
	row = event->y()/mLineHeight;

	if ((col < 16) && (row < ((data.size()+16)/16)))
	{
		mSelectedRow = row;
		mSelectedCol = col;
	}
	else
		goto _exit;

	// last row check col
	if ((row == (((data.size()+16)/16) - 1)) && (col >= (data.size() % 16)))
		goto _exit;

	qDebug("dumpview::selection(%d, %d)", mSelectedRow, mSelectedCol);
	update();
	return;

_exit:
	// Clear existing selection
	mSelectedRow = -1;
	update();
}

void DumpView::paintEvent(QPaintEvent* event)
{
	QStylePainter	painter(this);
	QRect			offsetRect = mOffsetPaneTopRect;
	QRect			dumpRect = mDumpPaneTopRect;
	QRect			asciiRect = mAsciiPaneTopRect;
	QPalette		pal = palette();
	QByteArray		ba;

	//qDebug("dumpview::paintEvent");

	// FIXME(LOW): unable to set the self widget's font in constructor
	painter.setFont(QFont("Courier"));

	// set a white background
	painter.fillRect(rect(), QBrush(QColor(Qt::white))); 

	// display the offset, dump and ascii panes 8 + 8 bytes on a line
	for (int i = 0; i < data.size(); i+=16)
	{	
		QString	dumpStr, asciiStr;	

		ba = data.mid(i, 16);

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

		// overpaint selection (if any)
		if ((i/16) == mSelectedRow)
		{
			QRect r;
			unsigned char c = data.at(mSelectedRow*16+mSelectedCol);
			QString selectedAsciiStr, selectedDumpStr;

			qDebug("dumpview::paintEvent - Highlighted");

			selectedDumpStr.append(QString("%1").arg((uint) c, 2, 16, QChar('0')).toUpper());

			if (isPrintable(c))
				selectedAsciiStr.append(QChar(c));
			else
				selectedAsciiStr.append(QChar('.'));

			// display dump
			r = dumpRect;
			if (mSelectedCol < 8)
				r.translate(mCharWidth*(mSelectedCol*3), 0);
			else
				r.translate(mCharWidth*(mSelectedCol*3+1), 0);
			r.setWidth(mCharWidth*2);
			painter.fillRect(r, pal.highlight());
			painter.drawItemText(r, Qt::AlignLeft | Qt::AlignTop, pal,
				true, selectedDumpStr, QPalette::HighlightedText);

			// display ascii
			r = asciiRect;
			if (mSelectedCol < 8)
				r.translate(mCharWidth*(mSelectedCol), 0);
			else
				r.translate(mCharWidth*(mSelectedCol+1), 0);
			r.setWidth(mCharWidth);
			painter.fillRect(r, pal.highlight());
			painter.drawItemText(r, Qt::AlignLeft | Qt::AlignTop, pal,
				true, selectedAsciiStr, QPalette::HighlightedText);
		}

		// move the rects down
		offsetRect.translate(0, mLineHeight);
		dumpRect.translate(0, mLineHeight);
		asciiRect.translate(0, mLineHeight);
	}	
}
