#ifndef QHEXEDIT_P_H
#define QHEXEDIT_P_H

/** \cond docNever */


#include <QtGui>

class QHexEditPrivate : public QWidget
{
Q_OBJECT

public:
    QHexEditPrivate(QScrollArea *parent);

    void setAddressOffset(int offset);
    int addressOffset();

    void setData(QByteArray const &data);
    QByteArray data();

    void setAddressAreaColor(QColor const &color);
    QColor addressAreaColor();

    void setHighlightingColor(QColor const &color);
    QColor highlightingColor();

    void setOverwriteMode(bool overwriteMode);
    bool overwriteMode();

    void insert(int i, const QByteArray & ba);
    void insert(int i, char ch);
    void remove(int index, int len=1);

    void setAddressArea(bool addressArea);
    void setAddressWidth(int addressWidth);
    void setAsciiArea(bool asciiArea);
    void setHighlighting(bool mode);
    virtual void setFont(const QFont &font);

signals:
    void currentAddress(int address);
    void dataChanged();
    void overwriteModeChanged(bool state);

protected:
    void keyPressEvent(QKeyEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void paintEvent(QPaintEvent *event);
    void setCursorPos(QPoint pos);
    void setCursorPos(int position);

private slots:
    void updateCursor();

private:
    void adjust();

    QColor _addressAreaColor;
    QByteArray _data;
    QByteArray _originalData;
    QColor _highlightingColor;
    QScrollArea *_scrollArea;
    QTimer _cursorTimer;

    bool _blink;
    bool _addressArea;
    bool _asciiArea;
    bool _highlighting;
    bool _overwriteMode;

    int _addressNumbers, _realAddressNumbers;
    int _addressOffset;
    int _charWidth, _charHeight;
    int _cursorX, _cursorY, _cursorWidth, _cursorHeight, _cursorPosition;
    int _xPosAdr, _xPosHex, _xPosAscii;
};

/** \endcond docNever */

#endif

