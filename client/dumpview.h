#include <QtGui> // FIXME: High


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
            const QModelIndex &bottomRight );
    void selectionChanged( const QItemSelection &selected,
    const QItemSelection &deselected );
    void paintEvent(QPaintEvent *event);

private:
    void populateDump(QByteArray &dump, int &selOfs, int &selSize, 
            QModelIndex parent = QModelIndex());
    bool inline isPrintable(char c) 
        {if ((c > 48) && (c < 126)) return true; else return false; }

private:
    QRect        mOffsetPaneTopRect;
    QRect        mDumpPaneTopRect;
    QRect        mAsciiPaneTopRect;
    int            mSelectedRow, mSelectedCol;
    int            mLineHeight;
    int            mCharWidth;
};

