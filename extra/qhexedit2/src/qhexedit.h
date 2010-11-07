#ifndef QHEXEDIT_H
#define QHEXEDIT_H

#include <QtGui>
#include "qhexedit_p.h"

/** \mainpage
QHexEdit is a binary editor widget for Qt.

\version Version 0.4.3
\image html hexedit.png
*/


/*! QHexEdit is a hex editor widget written in C++ for the Qt (Qt4) framework.
It is a simple editor for binary data, just like QPlainTextEdit is for text data.
There are sip configuration files included, so it is easy to create bindings
for PyQt and you can use this widget also in python.

QHexEdit takes the data of a QByteArray (setData()) and shows it. You can use the
mouse or the keyboard to navigate inside the widget. If you hit the keys (0..9, a..f)
you will change the data. Changed data is highlighted and can be accessed via data().

Normaly QHexEdit works in the overwrite Mode. You can set overwriteMode(false) and
insert data. In this case the size of data() increases. It is also possible to delete
bytes under the cursor, here the size of data decreases.

There are some limitations: The size of data has in general to be below 10 megabytes,
otherwise the scroll sliders ard not shown and you can't scroll any more. Copy and
paste functionality is perhaps a subject of a later release.
*/
        class QHexEdit : public QScrollArea
{
    Q_OBJECT
    /*! Property data holds the content of QHexEdit. Call setData() to set the
    content of QHexEdit, data() returns the actual content.
    */
    Q_PROPERTY(QByteArray data READ data WRITE setData)

    /*! Property addressOffset is added to the Numbers of the Address Area.
    A offset in the address area (left side) is sometimes usefull, whe you show
    only a segment of a complete memory picture. With setAddressOffset() you set
    this property - with addressOffset() you get the actual value.
    */
    Q_PROPERTY(int addressOffset READ addressOffset WRITE setAddressOffset)

    /*! Property address area color sets (setAddressAreaColor()) the backgorund
    color of address areas. You can also read the color (addressaAreaColor()).
    */
    Q_PROPERTY(QColor addressAreaColor READ addressAreaColor WRITE setAddressAreaColor)

    /*! Property highlighting color sets (setHighlightingColor()) the backgorund
    color of highlighted text areas. You can also read the color
    (highlightingColor()).
    */
    Q_PROPERTY(QColor highlightingColor READ highlightingColor WRITE setHighlightingColor)

    /*! Porperty overwrite mode sets (setOverwriteMode()) or gets (overwriteMode()) the mode
    in which the editor works. In overwritem mode the user will overwrite existing data.
    */
    Q_PROPERTY(bool overwriteMode READ overwriteMode WRITE setOverwriteMode)

public:
    /*! Creates an instance of QHexEdit.
    \param parent Parent widget of QHexEdit.
    */
    QHexEdit(QWidget *parent = 0);

    /*! Inserts a byte array.
    \param i Index position, where to insert
    \param ba byte array, which is to insert
    */
    void insert(int i, const QByteArray & ba);

    /*! Inserts a char.
    \param i Index position, where to insert
    \param ch Char, which is to insert
    */
    void insert(int i, char ch);

    /*! Removes len bytes from the content.
    \param pos Index position, where to remove
    \param len Amount of bytes to remove
    */
    void remove(int pos, int len=1);

    /*! Set the font of the widget. Please use fixed width fonts like Mono or Courier.*/
    void setFont(const QFont &);

    /*! \cond docNever */
    void setAddressOffset(int offset);
    int addressOffset();
    void setData(QByteArray const &data);
    QByteArray data();
    void setAddressAreaColor(QColor const &color);
    QColor addressAreaColor();
    void setHighlightingColor(QColor const &color);
    QColor highlightingColor();
    void setOverwriteMode(bool);
    bool overwriteMode();
    /*! \endcond docNever */

public slots:

    /*! Set the minimum width of the address area.
      \param addressWidth Width in characters.
      */
    void setAddressWidth(int addressWidth);

    /*! Switch the address area on or off.
      \param addressArea true (show it), false (hide it).
      */
    void setAddressArea(bool addressArea);

    /*! Switch the ascii area on or off.
      \param asciiArea true (show it), false (hide it).
      */
    void setAsciiArea(bool asciiArea);

    /*! Switch the highlighting feature on or of.
      \param mode true (show it), false (hide it).
      */
    void setHighlighting(bool mode);

signals:

    /*! Contains the address, where the cursor is located. */
    void currentAddress(int address);

    /*! The signal is emited every time, the data is changed. */
    void dataChanged();

    /*! The signal is emited every time, the overwrite mode is changed. */
    void overwriteModeChanged(bool state);

private:
    /*! \cond docNever */
    QHexEditPrivate *qHexEdit_p;
    QHBoxLayout *layout;
    QScrollArea *scrollArea;
    /*! \endcond docNever */
};

#endif

