/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include <QAbstractItemView>


class DumpView: public QAbstractItemView
{
public:    
    DumpView(QWidget *parent=0);

    QModelIndex indexAt( const QPoint &point ) const;
    void scrollTo( const QModelIndex &index, ScrollHint hint = EnsureVisible );
    QRect visualRect( const QModelIndex &index ) const;

protected:
    int horizontalOffset() const;
    bool isIndexHidden( const QModelIndex &index ) const;
    QModelIndex moveCursor( CursorAction cursorAction,
    Qt::KeyboardModifiers modifiers );
    void setSelection( const QRect &rect, QItemSelectionModel::SelectionFlags flags );
    int verticalOffset() const;
    QRegion visualRegionForSelection( const QItemSelection &selection ) const;
protected slots:
    void dataChanged( const QModelIndex &topLeft, 
            const QModelIndex &bottomRight,
            const QVector<int> &roles );
    void selectionChanged( const QItemSelection &selected,
    const QItemSelection &deselected );
    void paintEvent(QPaintEvent *event);

private:
    void populateDump(QByteArray &dump, int &selOfs, int &selSize, 
            QModelIndex parent = QModelIndex());
    bool inline isPrintable(char c) 
        {if ((c >= 32) && (c <= 126)) return true; else return false; }

private:
    QRect        mOffsetPaneTopRect;
    QRect        mDumpPaneTopRect;
    QRect        mAsciiPaneTopRect;
    int            mSelectedRow, mSelectedCol;
    int            mLineHeight;
    int            mCharWidth;
};

