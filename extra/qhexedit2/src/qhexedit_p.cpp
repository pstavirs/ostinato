#include <QtGui>

#include "qhexedit_p.h"

const int HEXCHARS_IN_LINE = 47;
const int GAP_ADR_HEX = 10;
const int GAP_HEX_ASCII = 16;
const int BYTES_PER_LINE = 16;

QHexEditPrivate::QHexEditPrivate(QScrollArea *parent) : QWidget(parent)
{
    _scrollArea = parent;
    setAddressWidth(4);
    setAddressOffset(0);
    setAddressArea(true);
    setAsciiArea(true);
    setHighlighting(true);
    setOverwriteMode(true);
    setAddressAreaColor(QColor(Qt::lightGray).lighter(110));
    setHighlightingColor(QColor(Qt::yellow).lighter(160));

    setFont(QFont("Mono", 10));
    connect(&_cursorTimer, SIGNAL(timeout()), this, SLOT(updateCursor()));

    _cursorTimer.setInterval(500);
    _cursorTimer.start();

    setFocusPolicy(Qt::StrongFocus);
}

void QHexEditPrivate::setAddressOffset(int offset)
{
    _addressOffset = offset;
    adjust();
}

int QHexEditPrivate::addressOffset()
{
    return _addressOffset;
}

void QHexEditPrivate::setData(const QByteArray &data)
{
    _data = data;
    _originalData = data;
    adjust();
    setCursorPos(0);
    setFocus();
}

QByteArray QHexEditPrivate::data()
{
    return _data;
}

void QHexEditPrivate::setAddressAreaColor(const QColor &color)
{
    _addressAreaColor = color;
    update();
}

QColor QHexEditPrivate::addressAreaColor()
{
    return _addressAreaColor;
}

void QHexEditPrivate::setHighlightingColor(const QColor &color)
{
    _highlightingColor = color;
    update();
}

QColor QHexEditPrivate::highlightingColor()
{
    return _highlightingColor;
}

void QHexEditPrivate::setOverwriteMode(bool overwriteMode)
{
    if (overwriteMode != _overwriteMode)
    {
        emit overwriteModeChanged(overwriteMode);
        _overwriteMode = overwriteMode;
        adjust();
    }
}

bool QHexEditPrivate::overwriteMode()
{
    return _overwriteMode;
}

void QHexEditPrivate::insert(int i, const QByteArray & ba)
{
    _data.insert(i, ba);
    _originalData.insert(i, ba);
}

void QHexEditPrivate::insert(int i, char ch)
{
    _data.insert(i, ch);
    _originalData.insert(i, ch);
}

void QHexEditPrivate::remove(int index, int len)
{
    _data.remove(index, len);
    _originalData.remove(index, len);
}

void QHexEditPrivate::setAddressArea(bool addressArea)
{
    _addressArea = addressArea;
    adjust();
    setCursorPos(_cursorPosition);
}

void QHexEditPrivate::setAddressWidth(int addressWidth)
{
    if ((addressWidth >= 0) and (addressWidth<=6))
    {
        _addressNumbers = addressWidth;
        adjust();
        setCursorPos(_cursorPosition);
    }
}

void QHexEditPrivate::setAsciiArea(bool asciiArea)
{
    _asciiArea = asciiArea;
    adjust();
}

void QHexEditPrivate::setFont(const QFont &font)
{
    QWidget::setFont(font);
    adjust();
}

void QHexEditPrivate::setHighlighting(bool mode)
{
    _highlighting = mode;
    update();
}

void QHexEditPrivate::keyPressEvent(QKeyEvent *event)
{
    bool down = false;
    int charX = (_cursorX - _xPosHex) / _charWidth;
    int posX = (charX / 3) * 2 + (charX % 3);
    int posBa = (_cursorY / _charHeight) * BYTES_PER_LINE + posX / 2;

    int key = int(event->text()[0].toAscii());
    if ((key>='0' && key<='9') || (key>='a' && key <= 'f'))
    {
        // calc address


        // insert char
        if (_overwriteMode == false)
            if ((charX % 3) == 0)
            {
                insert(posBa, char(0));
                adjust();
            }
        QByteArray hexValue = _data.mid(posBa, 1).toHex();
        if ((charX % 3) == 0)
            hexValue[0] = key;
        else
            hexValue[1] = key;
        _data.replace(posBa, 1, QByteArray().fromHex(hexValue));
        emit dataChanged();

        setCursorPos(_cursorPosition + 1);
        down = true;
    }

    // delete char
    if (event->matches(QKeySequence::Delete))
        remove(posBa);
    if (event->key() == Qt::Key_Backspace)
        {
            remove(posBa - 1);
            setCursorPos(_cursorPosition - 2);
        }

    // handle other function keys
    if (event->key() == Qt::Key_Insert)
        setOverwriteMode(!_overwriteMode);

    if (event->matches(QKeySequence::MoveToNextChar))
    {
        setCursorPos(_cursorPosition + 1);
        down = true;
    }
    if (event->matches(QKeySequence::MoveToPreviousChar))
            setCursorPos(_cursorPosition - 1);
    if (event->matches(QKeySequence::MoveToStartOfLine))
            setCursorPos(_cursorPosition - (_cursorPosition % (2 * BYTES_PER_LINE)));
    if (event->matches(QKeySequence::MoveToEndOfLine))
            setCursorPos(_cursorPosition | (2 * BYTES_PER_LINE -1));
    if (event->matches(QKeySequence::MoveToPreviousLine))
            setCursorPos(_cursorPosition - (2 * BYTES_PER_LINE));
    if (event->matches(QKeySequence::MoveToPreviousPage))
            setCursorPos(_cursorPosition - (((_scrollArea->viewport()->height() / _charHeight) - 1) * 2 * BYTES_PER_LINE));
    if (event->matches(QKeySequence::MoveToStartOfDocument))
        setCursorPos(0);
    if (event->matches(QKeySequence::MoveToNextLine))
    {
        setCursorPos(_cursorPosition + (2 * BYTES_PER_LINE));
        down = true;
    }
    if (event->matches(QKeySequence::MoveToEndOfDocument))
    {
        setCursorPos(_data.size() * 2);
        down = true;
    }
    if (event->matches(QKeySequence::MoveToNextPage))
    {
        setCursorPos(_cursorPosition + (((_scrollArea->viewport()->height() / _charHeight) - 1) * 2 * BYTES_PER_LINE));
        down = true;
    }

    // when we move downwards, we have to go a little further
    if (down)
        _scrollArea->ensureVisible(_cursorX, _cursorY, 3, 3 + _charHeight);
    else
        _scrollArea->ensureVisible(_cursorX, _cursorY, 3, 3);
    update();
}

void QHexEditPrivate::mousePressEvent(QMouseEvent * event)
{
    setCursorPos(event->pos());
}

void QHexEditPrivate::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // draw some patterns if needed
    painter.fillRect(event->rect(), this->palette().color(QPalette::Base));
    if (_addressArea)
        painter.fillRect(QRect(_xPosAdr, event->rect().top(), _xPosHex - GAP_ADR_HEX + 2, height()), _addressAreaColor);
    if (_asciiArea)
    {
        int linePos = _xPosAscii - (GAP_HEX_ASCII / 2);
        painter.setPen(Qt::gray);
        painter.drawLine(linePos, event->rect().top(), linePos, height());
    }

    painter.setPen(this->palette().color(QPalette::WindowText));

    // calc position
    int firstLineIdx = ((event->rect().top()/ _charHeight) - _charHeight) * BYTES_PER_LINE;
    if (firstLineIdx < 0)
        firstLineIdx = 0;
    int lastLineIdx = ((event->rect().bottom() / _charHeight) + _charHeight) * BYTES_PER_LINE;
    if (lastLineIdx > _data.size())
        lastLineIdx = _data.size();
    int yPosStart = ((firstLineIdx) / BYTES_PER_LINE) * _charHeight + _charHeight;

    // paint address area
    if (_addressArea)
    {
        for (int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=_charHeight)
        {
            QString address = QString("%1")
                              .arg(lineIdx + _addressOffset, _realAddressNumbers, 16, QChar('0'));
            painter.drawText(_xPosAdr, yPos, address);
        }
    }

    // paint hex area
    QByteArray hexBa(_data.mid(firstLineIdx, lastLineIdx - firstLineIdx + 1).toHex());
    QBrush highLighted = QBrush(_highlightingColor);
    painter.setBackground(highLighted);
    painter.setBackgroundMode(Qt::TransparentMode);
    for (int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=_charHeight)
    {
        QByteArray hex;
        int xPos = _xPosHex;
        for (int colIdx = 0; ((lineIdx + colIdx) < _data.size() and (colIdx < BYTES_PER_LINE)); colIdx++)
        {
            // hilight diff bytes
            if (_highlighting)
            {
                int posBa = lineIdx + colIdx;
                if (posBa >= _originalData.size())
                    painter.setBackgroundMode(Qt::TransparentMode);
                else
                    if (_data[posBa] == _originalData[posBa])
                        painter.setBackgroundMode(Qt::TransparentMode);
                    else
                        painter.setBackgroundMode(Qt::OpaqueMode);
            }

            // render hex value
            if (colIdx == 0)
            {
                hex = hexBa.mid((lineIdx - firstLineIdx) * 2, 2);
                painter.drawText(xPos, yPos, hex);
                xPos += 2 * _charWidth;
            } else {
                hex = hexBa.mid((lineIdx + colIdx - firstLineIdx) * 2, 2).prepend(" ");
                painter.drawText(xPos, yPos, hex);
                xPos += 3 * _charWidth;
            }
        }
    }
    painter.setBackgroundMode(Qt::TransparentMode);

    // paint ascii area
    if (_asciiArea)
    {
        for (int lineIdx = firstLineIdx, yPos = yPosStart; lineIdx < lastLineIdx; lineIdx += BYTES_PER_LINE, yPos +=_charHeight)
        {
            QByteArray ascii = _data.mid(lineIdx, BYTES_PER_LINE);
            for (int idx=0; idx < ascii.size(); idx++)
                if (((char)ascii[idx] < 0x20) or ((char)ascii[idx] > 0x7e))
                    ascii[idx] = '.';
            painter.drawText(_xPosAscii, yPos, ascii);
        }
    }

    // paint cursor
    if ((_data.size() > 0) and _blink)
        painter.fillRect(_cursorX, _cursorY, _cursorWidth, _cursorHeight, this->palette().color(QPalette::WindowText));
}

void QHexEditPrivate::setCursorPos(int position)
{
    // delete cursor
    _blink = false;
    update();

    // cursor in range?
    if (_overwriteMode)
    {
        if (position > (_data.size() * 2 - 1))
            position = _data.size() * 2 - 1;
    } else {
        if (position > (_data.size() * 2))
            position = _data.size() * 2;
    }

    if (position < 0)
        position = 0;

    // calc position
    _cursorPosition = position;
    _cursorY = (position / (2 * BYTES_PER_LINE)) * _charHeight + 4;
    int x = (position % (2 * BYTES_PER_LINE));
    _cursorX = (((x / 2) * 3) + (x % 2)) * _charWidth + _xPosHex;

    // immiadately draw cursor
    _blink = true;
    update();
    emit currentAddress(_cursorPosition/2);
}

void QHexEditPrivate::setCursorPos(QPoint pos)
{
    // find char under cursor
    if ((pos.x() >= _xPosHex) and (pos.x() < (_xPosHex + HEXCHARS_IN_LINE * _charWidth)))
    {
        int x = (pos.x() - _xPosHex) / _charWidth;
        if ((x % 3) == 0)
            x = (x / 3) * 2;
        else
            x = ((x / 3) * 2) + 1;
        int y = (pos.y() / _charHeight) * 2 * BYTES_PER_LINE;
        setCursorPos(x + y);
    }
}

void QHexEditPrivate::updateCursor()
{
    if (_blink)
        _blink = false;
    else
        _blink = true;
    update(_cursorX, _cursorY, _charWidth, _charHeight);
}

void QHexEditPrivate::adjust()
{
    _charWidth = fontMetrics().width(QLatin1Char('9'));
    _charHeight = fontMetrics().height();

    // is addressNumbers wide enought?
    QString test = QString("%1")
                  .arg(_data.size() + _addressOffset, _addressNumbers, 16, QChar('0'));
    _realAddressNumbers = test.size();

    _xPosAdr = 0;
    if (_addressArea)
        _xPosHex = _realAddressNumbers *_charWidth + GAP_ADR_HEX;
    else
        _xPosHex = 0;
    _xPosAscii = _xPosHex + HEXCHARS_IN_LINE * _charWidth + GAP_HEX_ASCII;

    if (_overwriteMode)
        _cursorWidth = _charWidth;
    else
        _cursorWidth = 2;
    _cursorHeight = _charHeight - 3;

    // tell QAbstractScollbar, how big we are
    setMinimumHeight(((_data.size()/16 + 1) * _charHeight) + 3);
    setMinimumWidth(_xPosAscii + (BYTES_PER_LINE * _charWidth));

    update();
}
