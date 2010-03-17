#ifndef __INT_COMBO_BOX
#define __INT_COMBO_BOX

#include <QComboBox>

class IntComboBox : public QComboBox
{
public:
    IntComboBox(QWidget *parent = 0)
        : QComboBox(parent)
    {
    }
    void addItem(int value, const QString &text) 
    {
        QComboBox::addItem(QString("%1 - %2").arg(value).arg(text), value);
    }
    int currentValue() 
    {
        bool isOk;
        int index = findText(currentText());
        if (index >= 0)
            return itemData(index).toInt();
        else
            return currentText().toInt(&isOk, 0);
    }
    void setValue(int value) 
    {
        int index = findData(value);
        if (index >= 0)
            setCurrentIndex(index);
        else
            setEditText(QString().setNum(value));
    }
};

#endif
