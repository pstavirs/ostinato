#include <QtGui> // FIXME: High

class DumpView: public QWidget	// QAbstractItemView // FIXME
{
public:
	DumpView(QWidget *parent=0);
	// bool setBase(uint base); // valid values: 16, 8, 10, 2 etc.
	// void hideAsciiPane(void);
	// void showAsciiPane(void);
	// void hideOffsetPane(void);
	// void showOffsetPane(void);


	//QSize sizeHint() const;

protected:
	void mousePressEvent(QMouseEvent *event);
	//void mouseMoveEvent(QMouseEvent *event);
	void paintEvent(QPaintEvent *event);

private:
	QString toAscii(QByteArray ba);
	bool inline isPrintable(char c) 
		{if ((c > 48) && (c < 126)) return true; else return false; }

private:
	QRect		mOffsetPaneTopRect;
	QRect		mDumpPaneTopRect;
	QRect		mAsciiPaneTopRect;
	QByteArray	data;
	int			mSelectedRow, mSelectedCol;
	int			mLineHeight;
	int			mCharWidth;
};
