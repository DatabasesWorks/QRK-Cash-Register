/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2019 Christian Kvasny <chris@ckvsoft.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Button Design, and Idea for the Layout are lean out from LillePOS, Copyright 2010, Martin Koller, kollix@aon.at
 *
*/

#include "salesinfo.h"
#include "qrkdelegate.h"
#include "database.h"
#include "3rdparty/qbcmath/bcmath.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QSqlError>
#include <QLineEdit>
#include <QStandardItemModel>

#include "ui_salesinfo.h"

SalesInfo::SalesInfo(QString from, QString to, QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::SalesInfo)
{
    ui->setupUi(this);


    ui->label->setText(tr("Umsätze %1 bis %2").arg(from).arg(to));

    loadData(from, to);
}

SalesInfo::~SalesInfo()
{
    delete ui;
}

void SalesInfo::loadData(QString from, QString to)
{
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare(Database::getSalesPerPaymentSQLQueryString());
    query.bindValue(":fromDate", from);
    query.bindValue(":toDate", to);

    query.exec();

    /*
     * FIXME: We do this workaroud, while SUM and ROUND will
     * give use the false result. SQL ROUND/SUM give xx.98 from xx.985
     * should be xx.99
     */

    QMap<QString, QMap<double, double> > zm;
    QMap<double, double> map;
    while (query.next())
    {
        QString key = query.value("actionText").toString();
        QBCMath tax(query.value("tax").toString());
        tax.round(2);
        QBCMath total(query.value("total").toString());
        total.round(2);
        if ( zm.contains(key) ) {
            map[tax.toDouble()] += total.toDouble();
            zm[key] = map;
        } else {
            map.clear();
            map[tax.toDouble()] = total.toDouble();
            zm[key] = map;
        }
    }

    QMap<QString, QMap<double, double> >::iterator i;
    QStandardItemModel *table_model = new QStandardItemModel(3, zm.size());

    int row = 0;
    for (i = zm.begin(); i != zm.end(); ++i) {
        QString key = i.key();
        QMap<double, double> tax = i.value();
        QMap<double, double>::iterator j;
        QBCMath k;
        QBCMath v;
        for (j = tax.begin(); j != tax.end(); ++j) {
            k = j.key();
            v = j.value();
            QStandardItem *item1 = new QStandardItem(key);
            table_model->setItem(row, 0, item1);
            QString kk = k.toString();
            QStandardItem *item2 = new QStandardItem(kk);
            table_model->setItem(row, 1, item2);
            QString vv = QBCMath::bcround(v.toString(), 2); //.replace(".",",");
            QStandardItem *item3 = new QStandardItem(vv);
            table_model->setItem(row, 2, item3);
            row++;
        }
    }

    ui->salesView->setModel(table_model);

    table_model->setHeaderData(0, Qt::Horizontal, tr("Bezahlart"));
    table_model->setHeaderData(1, Qt::Horizontal, tr("Steuersatz"));
    table_model->setHeaderData(2, Qt::Horizontal, tr("Summen"));

    ui->salesView->setItemDelegateForColumn(1, new QrkDelegate(QrkDelegate::COMBO_TAX, this));
    ui->salesView->setItemDelegateForColumn(2, new SalesDelegate(this));

    ui->salesView->resizeColumnsToContents();
    ui->salesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->closePushButton, &QPushButton::clicked, this, &SalesInfo::close);
}

SalesDelegate::SalesDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

QString SalesDelegate::displayText(const QVariant &value, const QLocale & /* locale */ ) const
{
    return QString("%1 %2").arg(value.toString()).arg(Database::getCurrency());
}

void SalesDelegate::paint(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    QStyleOptionViewItem myOption = option;
    myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;

    QStyledItemDelegate::paint(painter, myOption, index);
}
