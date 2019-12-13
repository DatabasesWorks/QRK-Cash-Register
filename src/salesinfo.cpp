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
#include "database.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "utils/utils.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "preferences/qrksettings.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QSqlError>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QTextDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QTime>

#include "ui_salesinfo.h"

SalesInfo::SalesInfo(SALESINFO what, QWidget *parent) :
    QDialog(parent, Qt::Tool),
    ui(new Ui::SalesInfo)
{
    ui->setupUi(this);
    ui->day->setHidden(true);
    ui->month->setHidden(true);
    for (int i = 1; i <= 12; i++)
        ui->month->addItem(QLocale().monthName(i),i);

    ui->month->setCurrentIndex(QDate::currentDate().month() -1);

    ui->year->setHidden(true);
    ui->year->setMinimum(Database::getFirstReceiptDate().year());
    ui->year->setMaximum(Database::getLastReceiptDate().year());
    ui->year->setValue(ui->year->maximum());

    m_what = what;
    m_timeoffset = QTime(0,0,0).secsTo(Database::getCurfewTime());

    connect(ui->day, &QDateEdit::dateChanged, this, &SalesInfo::setDateRange);
    connect(ui->month, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SalesInfo::setDateRange);
    connect(ui->year, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SalesInfo::setDateRange);
    connect(ui->printButton, &QPushButton::clicked, this, &SalesInfo::printInfo);
    connect(ui->closePushButton, &QPushButton::clicked, this,&SalesInfo::close);

    ui->day->setDate(QDate::currentDate());
}

SalesInfo::~SalesInfo()
{
    delete ui;
}

void SalesInfo::setDateRange()
{
    blockSignals(true);
    ui->day->setTime(QTime(0,0,0));

    if (m_what == SALESINFO_DAY) {
        ui->day->setHidden(false);
        m_from = ui->day->date().toString(Qt::ISODate);
        m_to = ui->day->dateTime().addDays(1).addSecs(m_timeoffset -1).toString(Qt::ISODate);
    }
    else if (m_what == SALESINFO_MONTH) {
        ui->month->setHidden(false);
        int month = ui->month->currentIndex() +1;
        QDateTime dt = QDateTime::fromString(QString("01.%1.%2 00:00").arg(month).arg(ui->year->value()), "dd.M.yyyy hh:mm").addSecs(m_timeoffset);
        m_from = dt.toString(Qt::ISODate);
        m_to = dt.addMonths(1).addSecs(-1).toString(Qt::ISODate);
    }
    else if (m_what == SALESINFO_YEAR) {
        ui->year->setHidden(false);
        QDateTime dt = QDateTime::fromString(QString("01.01.%1 00:00").arg(ui->year->value()), "dd.MM.yyyy hh:mm").addSecs(m_timeoffset);
        m_from = dt.toString(Qt::ISODate);
        m_to = dt.addYears(1).addSecs(-1).toString(Qt::ISODate);
    }

    if (m_what == SALESINFO_DAY)
        ui->label->setText(tr("Umsatz von %1 bis %2)").arg(QDateTime::fromString(m_from, Qt::ISODate).toString(Qt::LocaleDate)).arg(QDateTime::fromString(m_to, Qt::ISODate).toString(Qt::LocaleDate)));
    else
        ui->label->setText(tr("Umsätze von %1 bis %3").arg(QDateTime::fromString(m_from, Qt::ISODate).toString(Qt::LocaleDate)).arg(QDateTime::fromString(m_to, Qt::ISODate).toString(Qt::LocaleDate)));

    loadData();
    blockSignals(false);
}

void SalesInfo::loadData()
{

    int size;
    QMap<QString, QMap<QString, double> > user = Database::getSalesPerUser(m_from, m_to, size);
    QMap<QString, QMap<QString, double> >::iterator u;
    QBCMath totalsum = 0.0;
    QStandardItemModel *table_model = new QStandardItemModel(3, size);
    int row = 0;
    QString username, prename;
    for (u = user.begin(); u != user.end(); ++u) {
        username = u.key();
        if (username.isEmpty())
            username = tr("n/a");
        QMap<QString, double> zm2 = u.value();
        QMap<QString, double>::iterator i;
        for (i = zm2.begin(); i != zm2.end(); ++i) {
            QString key = i.key();
            QBCMath total(i.value());
            total.round(2);
            QStandardItem *item1 = new QStandardItem(username == prename?"":username);
            table_model->setItem(row, 0, item1);
            QStandardItem *item2 = new QStandardItem(key);
            table_model->setItem(row, 1, item2);
            QStandardItem *item3 = new QStandardItem(total.toLocale());
            table_model->setItem(row, 2, item3);
            row++;

            totalsum += total;
            prename = username;
        }
        totalsum.round(2);
    }

    totalsum.round(2);

    ui->sumLabel->setText(totalsum.toLocale() + " " + Database::getCurrency() );
    ui->salesView->setModel(table_model);

    table_model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    table_model->setHeaderData(1, Qt::Horizontal, tr("Bezahlart"));
    table_model->setHeaderData(2, Qt::Horizontal, tr("Summen"));

    ui->salesView->setItemDelegateForColumn(2, new SalesDelegate(this));

    ui->salesView->resizeColumnsToContents();
    ui->salesView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->closePushButton, &QPushButton::clicked, this, &SalesInfo::close);
}

void SalesInfo::printInfo()
{

    if(!RBAC::Instance()->hasPermission("salesinfo_print", true)) return;

    QrkSettings settings;
    settings.beginGroup("salesinfo");

    QTextDocument doc;
    QString html;
    QTextStream out(&html);

    const int rowCount = ui->salesView->model()->rowCount();
    const int columnCount = ui->salesView->model()->columnCount();

    out <<  "<html>\n<head>\n<meta Content=\"Text/html; charset=UTF-8\">\n"
        <<  QString("<title>%1</title>\n").arg(ui->label->text())
        <<  "</head>\n<body>\n<table border=1 cellspacing=0 cellpadding=2>\n";

    out <<  QString("<b>%1</b>\n").arg(ui->label->text());

    out << "<thead><tr>";
    for (int column = 0; column < columnCount; column++)
        if (!ui->salesView->isColumnHidden(column))
            out << QString("<th>%1</th>").arg(ui->salesView->model()->headerData(column, Qt::Horizontal).toString());
    out << "</tr></thead>\n";

    for (int row = 0; row < rowCount; row++) {
        out << "<tr>";
        for (int column = 0; column < columnCount; column++) {
            if (!ui->salesView->isColumnHidden(column)) {
                QString data = ui->salesView->model()->data(ui->salesView->model()->index(row, column)).toString().simplified();
                if (Utils::isNumber(data)) {
                    data += " " + Database::getCurrency();
                    out << QString("<td align=\"right\">%1</td>").arg((!data.isEmpty()) ? data : QString("&nbsp;"));
                } else {
                    out << QString("<td align=\"right\">%1</td>").arg((!data.isEmpty()) ? data : QString("&nbsp;"));
                }
            }
        }
        out << "</tr>\n";
    }
    out <<  "</table>\n";
    out <<  QString("Summe: <b>%1</b>\n").arg(ui->sumLabel->text());
    out <<  "</body>\n</html>\n";

    doc.setHtml(html);
    QPrinter printer;
    printer.setPrinterName(settings.value("printername", printer.printerName()).toString());
    printer.setOutputFormat(QPrinter::OutputFormat(settings.value("printeroutputformat", printer.outputFormat()).toInt()));
    printer.setOutputFileName(settings.value("printeroutputfilename", printer.outputFileName()).toString());

    QPrintDialog dlg(&printer);
    if (dlg.exec() != QDialog::Accepted)
        return;

    settings.save2Settings("printername", printer.printerName());
    settings.save2Settings("printeroutputformat", int(printer.outputFormat()));
    settings.save2Settings("printeroutputfilename", printer.outputFileName());
    settings.endGroup();

    doc.print(&printer);
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
