#ifndef SALESINFO_H
#define SALESINFO_H

#include <QDialog>
#include <QStyledItemDelegate>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QStyleOptionViewItem>

namespace Ui {
class SalesInfo;
}

class SalesInfo : public QDialog
{
    Q_OBJECT

public:
    explicit SalesInfo(QString from, QString to, QWidget *parent = 0);
    ~SalesInfo();

private:
    Ui::SalesInfo *ui;
    void loadData(QString from, QString to);
};

class SalesDelegate : public QStyledItemDelegate
{
        Q_OBJECT

    public:
        SalesDelegate(QObject *parent = 0);

        QString displayText(const QVariant &value, const QLocale &locale) const;
        void paint(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

#endif // SALESINFO_H
