#ifndef _HEXLINEEDIT
#define _HEXLINEEDIT

#include <QLineEdit>

class HexLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    // Constructors
    HexLineEdit ( QWidget * parent);

protected:
    void focusOutEvent( QFocusEvent *e );
    //void focusInEvent( QFocusEvent *e );
    //void keyPressEvent( QKeyEvent *e );

signals:
    //void focusIn();
    void focusOut();
};

#endif

