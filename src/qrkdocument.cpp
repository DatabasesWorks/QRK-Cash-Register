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

#include "defines.h"
#include "database.h"
#include "databasemanager.h"
#include "qrkdocument.h"
#include "documentprinter.h"
#include "preferences/qrksettings.h"
#include "RK/rk_signaturemodule.h"
#include "reports.h"
#include "qrkdelegate.h"
#include "qrktimedmessagebox.h"
#include "utils/utils.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "ui_qrkdocument.h"

#include <QMessageBox>
#include <QDesktopWidget>
#include <QJsonObject>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

QRKDocument::QRKDocument(QWidget *parent)
    : QWidget(parent), ui(new Ui::QRKDocument)

{

    ui->setupUi(this);

    ui->textBrowser->setHidden(true);

    if ( QApplication::desktop()->width() < 1200 )
    {
        ui->cancelDocumentButton->setMinimumWidth(0);
        ui->cancellationButton->setMinimumWidth(0);
        ui->invoiceCompanyPrintcopyButton->setMinimumWidth(0);
        ui->pushFreeButton->setMinimumWidth(0);
    }

    connect(ui->cancelDocumentButton, &QPushButton::clicked, this, &QRKDocument::cancelDocumentButton_clicked);
    connect(ui->printcopyButton, &QPushButton::clicked, this, &QRKDocument::onPrintcopyButton_clicked);
    connect(ui->invoiceCompanyPrintcopyButton, &QPushButton::clicked, this, &QRKDocument::onInvoiceCompanyButton_clicked);
    connect(ui->cancellationButton, &QPushButton::clicked, this, &QRKDocument::onCancellationButton_clicked);

}

QRKDocument::~QRKDocument()
{
    delete ui;
}

//--------------------------------------------------------------------------------

void QRKDocument::documentList(bool servermode)
{
    m_servermode = servermode;
    QSqlDatabase dbc = Database::database();

    ui->documentLabel->setText("");
    m_documentContentModel = new QSqlQueryModel;

    m_documentListModel = new QSortFilterSqlQueryModel;

    QString driverName = dbc.driverName();
    if ( driverName == "QMYSQL" ) {
        m_documentListModel->setQuery("SELECT receipts.receiptNum, actionTypes.actionText, DATE_FORMAT(receipts.infodate, '%Y-%m-%d') AS infodate, ROUND(receipts.gross,2) AS gross, receipts.timestamp FROM receipts INNER JOIN actionTypes ON receipts.payedBy=actionTypes.actionId", dbc);
    }
    else if ( driverName == "QSQLITE" ) {
        m_documentListModel->setQuery("SELECT receipts.receiptNum, actionTypes.actionText, strftime('%Y-%m-%d',receipts.infodate) AS infodate, ROUND(receipts.gross,2) AS gross, receipts.timestamp FROM receipts INNER JOIN actionTypes ON receipts.payedBy=actionTypes.actionId", dbc);
    }

    m_documentListModel->setFilterColumn("receiptNum");
    m_documentListModel->setFilterFlags(Qt::MatchStartsWith);
    m_documentListModel->setFilter("");
    m_documentListModel->select();
    m_documentListModel->sort(DOCUMENT_COL_RECEIPT, Qt::DescendingOrder);

    m_documentListModel->setHeaderData(DOCUMENT_COL_RECEIPT, Qt::Horizontal, tr("Beleg"));
    m_documentListModel->setHeaderData(DOCUMENT_COL_TYPE, Qt::Horizontal, tr("Type"));
    m_documentListModel->setHeaderData(DOCUMENT_COL_INFO, Qt::Horizontal, tr("Info"));
    m_documentListModel->setHeaderData(DOCUMENT_COL_TOTAL, Qt::Horizontal, tr("Summe"));
    m_documentListModel->setHeaderData(DOCUMENT_COL_DATE, Qt::Horizontal, tr("Erstellungsdatum"));

    if (m_documentListModel->lastError().isValid())
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << m_documentListModel->lastError();

    ui->documentList->setModel(m_documentListModel);
    ui->documentList->setItemDelegateForColumn(DOCUMENT_COL_TOTAL, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));

    ui->documentContent->setModel(m_documentContentModel);
    ui->documentContent->setShowGrid(false);

    ui->documentList->horizontalHeader()->setStretchLastSection(false);
    ui->documentList->resizeColumnsToContents();
    ui->documentContent->resizeColumnsToContents();
    ui->documentList->horizontalHeader()->setStretchLastSection(true);

    ui->documentFilterLabel->setText("Filter " + m_documentListModel->getFilterColumnName());

    connect(m_documentListModel, &QSortFilterSqlQueryModel::sortChanged, this, &QRKDocument::sortChanged);
    connect(ui->documentFilterEdit, &QLineEdit::textChanged, m_documentListModel, &QSortFilterSqlQueryModel::filter);
    connect(ui->documentList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &QRKDocument::onDocumentSelectionChanged);

    ui->cancellationButton->setEnabled(false);
    ui->printcopyButton->setEnabled(false);
    ui->invoiceCompanyPrintcopyButton->setEnabled(false);

    QrkSettings settings;
    m_receiptPrintDialog = settings.value("useReceiptPrintedDialog", true).toBool();

}

//----------------SLOTS-----------------------------------------------------------

void QRKDocument::cancelDocumentButton_clicked()
{
    DatabaseManager::removeCurrentThread("CN");
    emit cancelDocumentButton();
}

void QRKDocument::onDocumentSelectionChanged(const QItemSelection &, const QItemSelection & )
{
    QModelIndexList indexList = ui->documentList->selectionModel()->selectedIndexes();
    int row = -1;
    foreach (QModelIndex index, indexList) {
        row = index.row();
    }

    int receiptNum = m_documentListModel->data(m_documentListModel->index(row, DOCUMENT_COL_RECEIPT, QModelIndex())).toInt();
    QString payedByText = m_documentListModel->data(m_documentListModel->index(row, DOCUMENT_COL_TYPE, QModelIndex())).toString();
    double price = m_documentListModel->data(m_documentListModel->index(row, DOCUMENT_COL_TOTAL, QModelIndex())).toDouble();
    int type = Database::getActionTypeByName(payedByText);

    bool isDamaged = false;
    ui->pixmapLabel->setPixmap(Utils::getQRCode(receiptNum, isDamaged));
    ui->pixmapLabel->show();
    if (RKSignatureModule::isDEPactive() && isDamaged)
        ui->qrcodeTextLabel->setText(tr("Sicherheitseinrichtung ausgefallen"));
    else
        ui->qrcodeTextLabel->setText("");

    ui->cancellationButton->setEnabled(!Database::isCashRegisterInAktive() && type < PAYED_BY_REPORT_EOD);
    ui->invoiceCompanyPrintcopyButton->setEnabled(type < PAYED_BY_REPORT_EOD);

    if (type == PAYED_BY_REPORT_EOD || type == PAYED_BY_REPORT_EOM) { /* actionType Tagesbeleg*/
        // ui->customerTextLabel->setHidden(true);
        ui->documentContent->setHidden(true);
        ui->textBrowser->setHidden(false);
        ui->printcopyButton->setEnabled(true);
        ui->textBrowser->setHtml(Reports::getReport(receiptNum));
        ui->documentLabel->setText(tr("Beleg Nr: %1\t%2\t%3 %4").arg(receiptNum).arg(payedByText).arg(QLocale().toString(price, 'f', 2)).arg(Database::getCurrency()));
        ui->customerTextLabel->setText(tr("-"));

    } else {
        // ui->customerTextLabel->setHidden(false);
        ui->documentContent->setHidden(false);
        ui->textBrowser->setHidden(true);
        ui->printcopyButton->setEnabled(true);

        QString stornoText = "";
        if (Database::getStorno(receiptNum) == 1)
            stornoText = tr("(Stornierter Beleg, siehe Beleg Nr: %1)").arg(Database::getStornoId(receiptNum));
        else if (Database::getStorno(receiptNum) == 2)
            stornoText = tr("(Storno Beleg für Beleg Nr: %1)").arg(Database::getStornoId(receiptNum));

        ui->documentLabel->setText(tr("Beleg Nr: %1\t%2\t%3 %4\t\t%5").arg(receiptNum).arg(payedByText).arg(QLocale().toString(price, 'f', 2)).arg(Database::getCurrency()).arg(stornoText));
        ui->customerTextLabel->setText(tr("Kunden Zusatztext: ") + Database::getCustomerText(receiptNum));

        QSqlDatabase dbc = Database::database();
        int id = ui->documentList->model()->data(m_documentListModel->index(row, REGISTER_COL_COUNT, QModelIndex())).toInt();

        m_documentContentModel->setQuery(QString("SELECT orders.count, products.itemnum, products.name, round(orders.net,2), orders.tax, orders.gross, orders.discount * (-1), ROUND((orders.count * orders.gross) - ((orders.count * orders.gross / 100) * orders.discount),2) AS Price FROM orders INNER JOIN products ON products.id=orders.product WHERE orders.receiptId=%1").arg(id), dbc);
        m_documentContentModel->setHeaderData(REGISTER_COL_COUNT, Qt::Horizontal, tr("Anz."));
        m_documentContentModel->setHeaderData(REGISTER_COL_PRODUCTNUMBER, Qt::Horizontal, tr("Artikelnummer"));
        m_documentContentModel->setHeaderData(REGISTER_COL_PRODUCT, Qt::Horizontal, tr("Artikel"));
        m_documentContentModel->setHeaderData(REGISTER_COL_NET, Qt::Horizontal, tr("E-Netto"));
        m_documentContentModel->setHeaderData(REGISTER_COL_TAX, Qt::Horizontal, tr("MwSt."));
        m_documentContentModel->setHeaderData(REGISTER_COL_SINGLE, Qt::Horizontal, tr("E-Preis"));
        m_documentContentModel->setHeaderData(REGISTER_COL_DISCOUNT, Qt::Horizontal, tr("Rabatt %"));
        m_documentContentModel->setHeaderData(REGISTER_COL_TOTAL, Qt::Horizontal, tr("Preis"));
        ui->documentContent->setItemDelegateForColumn(REGISTER_COL_NET, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));
        ui->documentContent->setItemDelegateForColumn(REGISTER_COL_TAX, new QrkDelegate (QrkDelegate::COMBO_TAX, this));
        ui->documentContent->setItemDelegateForColumn(REGISTER_COL_SINGLE, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));
        ui->documentContent->setItemDelegateForColumn(REGISTER_COL_DISCOUNT, new QrkDelegate (QrkDelegate::DISCOUNT, this));
        ui->documentContent->setItemDelegateForColumn(REGISTER_COL_TOTAL, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));

        ui->documentContent->resizeColumnsToContents();
        ui->documentContent->horizontalHeader()->setSectionResizeMode(REGISTER_COL_PRODUCT, QHeaderView::Stretch);
        //ui->documentContent->setColumnHidden(REGISTER_COL_NET, true);

    }
}

//--------------------------------------------------------------------------------

void QRKDocument::onCancellationButton_clicked()
{
    if (m_servermode) {
        QMessageBox::information(this, tr("Storno"), tr("Zur Zeit ist kein Storno möglich. ServerMode ist aktiv"));
        emit documentButton_clicked();
        return;
    }

    if(!RBAC::Instance()->hasPermission("documents_cancellation", true)) return;

    ui->cancellationButton->setEnabled(false);
    ui->documentLabel->setText("");

    QModelIndex idx = ui->documentList->currentIndex();
    int row = idx.row();
    if (row < 0)
        return;

    int id = m_documentListModel->data(m_documentListModel->index(row, DOCUMENT_COL_RECEIPT, QModelIndex())).toInt();

    int storno = Database::getStorno(id);
    if (storno)
    {
        QString stornoText = "";
        if (storno == 1)
            stornoText = tr("Beleg mit der Nummer %1 wurde bereits storniert. Siehe Beleg Nr: %2").arg(id).arg(Database::getStornoId(id));
        else
            stornoText = tr("Beleg mit der Nummer %1 ist ein Stornobeleg von Beleg Nummer %2 und kann nicht storniert werden.\nErstellen Sie einen neuen Beleg. (Kassabon)").arg(id).arg(Database::getStornoId(id));

        QMessageBox::warning( this, tr("Storno"), stornoText);
        emit documentButton_clicked();
        return;
    }

    int payedBy = Database::getPayedBy(id);
    Reports rep;
    bool ret = rep.checkEOAny(); //  rep->checkEOAnyServerMode();
    if (! ret) {
        emit documentButton_clicked();
        return;
    }

    ReceiptItemModel reg;
    reg.clear();
    reg.newOrder();
    reg.storno(id);

    m_currentReceipt = reg.createReceipts();
    if ( m_currentReceipt ) {
        reg.setCurrentReceiptNum(m_currentReceipt);
        if ( reg.createOrder(true) ) {
            if ( reg.finishReceipts(payedBy, id) ) {
                emit documentButton_clicked();
                return;
            }
        }
    }
}

//--------------------------------------------------------------------------------

void QRKDocument::onInvoiceCompanyButton_clicked()
{
    onPrintcopyButton_clicked(true);
}

//--------------------------------------------------------------------------------

void QRKDocument::onPrintcopyButton_clicked(bool isInvoiceCompany)
{
    QModelIndex idx = ui->documentList->currentIndex();
    int row = idx.row();
    int id = m_documentListModel->data(m_documentListModel->index(row, DOCUMENT_COL_RECEIPT, QModelIndex())).toInt();

    if (!id)
        return;


    QString payedByText = m_documentListModel->data(m_documentListModel->index(row, DOCUMENT_COL_TYPE, QModelIndex())).toString();
    int type = Database::getActionTypeByName(payedByText);

    if (type == PAYED_BY_REPORT_EOD || type == PAYED_BY_REPORT_EOM) { /* actionType Tagesbeleg*/
        QString DocumentTitle = QString("BELEG_%1_%2").arg(id).arg(payedByText);
        QTextDocument doc;
        doc.setHtml(Reports::getReport(id));

        QTextCursor cursor(&doc);
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        bool isDamaged;
        QImage img = Utils::getQRCode(id, isDamaged).toImage();
        cursor.insertImage(img);
        if (isDamaged)
            cursor.insertHtml("</br><small>Sicherheitseinrichtung ausgefallen</small>");

        DocumentPrinter p;
        p.printDocument(&doc, DocumentTitle);

        if (m_receiptPrintDialog) {
            QrkTimedMessageBox messageBox(10,
                                   QMessageBox::Information,
                                   tr("Drucker"),
                                   tr("%1 wurde gedruckt.").arg(payedByText),
                                   QMessageBox::Yes | QMessageBox::Default
                                   );

            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
            messageBox.exec();
        }

    } else {

        m_currentReceipt = id;

        ReceiptItemModel reg;
        reg.setCurrentReceiptNum(id);

        QrkSettings settings;
        QJsonObject data = reg.compileData();

        data["isCopy"] = true;
        int storno = Database::getStorno(id);
        if (storno == 2) {
            id = Database::getStornoId(id);
            data["comment"] = (id > 0)? tr("Storno für Beleg Nr: %1").arg(id):settings.value("receiptPrinterHeading", "KASSABON").toString();
        }

        data["isInvoiceCompany"] = isInvoiceCompany;
        data["headerText"] = Database::getCustomerText(id);

        DocumentPrinter p;
        p.printReceipt(data);

        if (m_receiptPrintDialog) {
            QrkTimedMessageBox messageBox(10,
                                   QMessageBox::Information,
                                   tr("Drucker"),
                                   tr("%1 %2 (Kopie) wurde gedruckt.").arg(data.value("comment").toString()).arg(m_currentReceipt),
                                   QMessageBox::Yes | QMessageBox::Default
                                   );

            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
            messageBox.exec();
        }

        emit documentButton_clicked();
    }
}

void QRKDocument::sortChanged()
{
    ui->documentFilterLabel->setText("Filter " + m_documentListModel->getFilterColumnName());
}
