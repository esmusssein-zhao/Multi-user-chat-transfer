#ifndef READONLYDELEGATE_H
#define READONLYDELEGATE_H

#include <QWidget>
#include <qitemdelegate.h>

//Set a tableview row/column uneditable
class ReadOnlyDelegate: public QItemDelegate
{
public:
    ReadOnlyDelegate(QWidget *parent = NULL):QItemDelegate(parent){}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,const QModelIndex &index) const override;
};

#endif // READONLYDELEGATE_H
