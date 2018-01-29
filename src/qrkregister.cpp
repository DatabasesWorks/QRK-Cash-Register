/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2018 Christian Kvasny <chris@ckvsoft.at>
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
#include "qrkregister.h"
#include "givendialog.h"
#include "qrkdelegate.h"
#include "r2bdialog.h"
#include "documentprinter.h"
#include "utils/utils.h"
#include "reports.h"
#include "pluginmanager/pluginmanager.h"
#include "qrkprogress.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "ui_qrkregister.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopWidget>
#include <QToolButton>
#include <QTimer>
#include <QKeyEvent>
#include <QShortcut>
#include <QTextDocument>
#include <QDir>
#include <QtWidgets>
#include <QDebug>

QRKRegister::QRKRegister(QWidget *parent)
    : QWidget(parent), ui(new Ui::QRKRegister), m_currentReceipt(0), m_totallyup(false)
{

    ui->setupUi(this);

    /*Todo: we do not longer need this*/
    ui->totallyupButton->setHidden(true);

    if ( QApplication::desktop()->width() < 1200 )
    {
        ui->cancelRegisterButton->setMinimumWidth(0);
        ui->cashReceipt->setMinimumWidth(0);
        ui->creditcardReceipt->setMinimumWidth(0);
        ui->debitcardReceipt->setMinimumWidth(0);
        ui->totallyupButton->setMinimumWidth(0);
        ui->totallyupExitButton->setMinimumWidth(0);
        ui->plusButton->setMinimumWidth(0);
        ui->minusButton->setMinimumWidth(0);
        ui->receiptToInvoice->setMinimumWidth(0);
        ui->checkReceiptButton->setMinimumWidth(0);
        ui->receiptToInvoice->setMinimumWidth(0);
    }

    ui->checkReceiptButton->setToolTip(tr("Erstellt einen NULL (Kontroll) Beleg für den Prüfer vom BMF"));

    ui->totallyup->setVisible(false);
    ui->safetyDeviceLabel->setVisible(false);
    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit->setDateRange(QDate::fromString(QString("%1-01-01").arg(QDate::currentDate().year()),"yyyy-MM-dd"), QDate::currentDate());

    connect(ui->totallyupButton, &QPushButton::clicked, this, &QRKRegister::totallyupSlot);
    connect(ui->totallyupExitButton, &QPushButton::clicked, this, &QRKRegister::totallyupExitSlot);
    connect(ui->receiptToInvoice, &QPushButton::clicked, this, &QRKRegister::receiptToInvoiceSlot);

    connect(ui->plusButton, &QPushButton::clicked, this, &QRKRegister::plusSlot);
    connect(ui->minusButton, &QPushButton::clicked, this, &QRKRegister::minusSlot);
    connect(ui->cancelRegisterButton, &QPushButton::clicked, this, &QRKRegister::onCancelRegisterButton_clicked);
    connect(ui->checkReceiptButton, &QPushButton::clicked, this, &QRKRegister::createCheckReceipt);

    QShortcut *plusButtonShortcut = new QShortcut(QKeySequence("Insert"), this);
    QShortcut *minusButtonShortcut = new QShortcut(QKeySequence("Delete"), this);

    connect(plusButtonShortcut, &QShortcut::activated, this, &QRKRegister::plusSlot);
    connect(minusButtonShortcut, &QShortcut::activated, this, &QRKRegister::minusSlot);

    ui->buttonGroup->setId(ui->cashReceipt, PAYED_BY_CASH);
    ui->buttonGroup->setId(ui->creditcardReceipt, PAYED_BY_CREDITCARD);
    ui->buttonGroup->setId(ui->debitcardReceipt, PAYED_BY_DEBITCARD);

    connect(ui->buttonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &QRKRegister::onButtonGroup_payNow_clicked);

    m_orderListModel = new ReceiptItemModel(this);

    connect(ui->barcodeLineEdit, &QLineEdit::editingFinished, this, &QRKRegister::barcodeChangedSlot);

    connect(m_orderListModel, &ReceiptItemModel::setButtonGroupEnabled, this, &QRKRegister::setButtonGroupEnabled);
    connect(m_orderListModel, &ReceiptItemModel::finishedItemChanged, this, &QRKRegister::finishedItemChanged);
    connect(m_orderListModel, &ReceiptItemModel::finishedPlus, this, &QRKRegister::finishedPlus);

}

QRKRegister::~QRKRegister()
{
    delete ui;
    delete m_orderListModel;
}

void QRKRegister::keyPressEvent(QKeyEvent *event) {

    if(event->key() == m_barcodeReaderPrefix)
    {
        ui->barcodeLineEdit->setFocus();
        ui->barcodeLineEdit->clear();
    }
}

bool QRKRegister::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == m_barcodeReaderPrefix)
        {
            ui->barcodeLineEdit->setFocus();
            ui->barcodeLineEdit->clear();
            return true;
        }

    }
    return QObject::eventFilter(obj, event);
}

//--------------------------------------------------------------------------------

void QRKRegister::finishedItemChanged()
{
    ui->orderList->resizeColumnsToContents();
    ui->orderList->horizontalHeader()->setSectionResizeMode(REGISTER_COL_PRODUCT, QHeaderView::Stretch);

    updateOrderSum();
}

//--------------------------------------------------------------------------------

void QRKRegister::init()
{
    initPlugins();

    QrkSettings settings;
    QDate last = Database::getLastReceiptDate();

    ui->barcodeLineEdit->setEnabled(true);
    ui->dateEdit->setDateRange(last, QDate::currentDate());
    ui->debitcardReceipt->setHidden(settings.value("hideDebitcardButton", false).toBool());
    ui->creditcardReceipt->setHidden(settings.value("hideCreditcardButton", false).toBool());

    ui->orderList->setModel(m_orderListModel);

    m_useInputNetPrice = settings.value("useInputNetPrice", false).toBool();
    m_useDiscount = settings.value("useDiscount", false).toBool();
    m_useMaximumItemSold = settings.value("useMaximumItemSold", false).toBool();
    m_useDecimalQuantity = settings.value("useDecimalQuantity", false).toBool();
    m_useGivenDialog = settings.value("useGivenDialog", false).toBool();
    m_barcodeReaderPrefix = settings.value("barcodeReaderPrefix", Qt::Key_F11).toInt();

    m_receiptPrintDialog = settings.value("useReceiptPrintedDialog", true).toBool();

    if (Database::getTaxLocation() == "AT")
        ui->checkReceiptButton->setVisible(true);
    else
        ui->checkReceiptButton->setVisible(false);

    quickGroupButtons();

}

void QRKRegister::safetyDevice(bool active)
{
    if (active) {
        ui->orderList->setStyleSheet("");
        ui->safetyDeviceLabel->setVisible(false);
    } else {
        ui->orderList->setStyleSheet("QTableView{border : 3px solid red}");
        ui->safetyDeviceLabel->setVisible(true);
    }
}

void QRKRegister::quickGroupButtons()
{

    m_buttonGroupGroups = new QButtonGroup(this);

    QLayoutItem *child;
    while ((child = ui->scrollArea->widget()->layout()->takeAt(0)) != 0) {
        delete child->widget(); // delete Layout Item's underlying widget
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok = query.prepare("SELECT id, name, color FROM groups WHERE visible=1");

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    while (query.next())
    {
        QString pbText = query.value(1).toString();
        QPushButton *pb = new QPushButton(pbText, ui->scrollArea);
        pb->setMinimumHeight(60);

        QString backgroundColor = (query.value(2).toString() == "")? "#808080":query.value(2).toString();
        QString best_contrast = Utils::color_best_contrast(backgroundColor);

        pb->setStyleSheet(
                    "QPushButton {"
                    "margin: 3px;"
                    "border-color: black;"
                    "border-style: outset;"
                    "border-radius: 3px;"
                    "border-width: 1px;"
                    "color: " + best_contrast + ";"
                                                "background-color: " + backgroundColor + ";" // qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + backgroundColor + ", stop: 1 #0d5ca6);"
                    "}"
                    "QPushButton:pressed {"
                    "background-color: black;" // qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0d5ca6, stop: 1 " + backgroundColor + ");"
                    "}"
                    );

        ui->scrollArea->widget()->layout()->addWidget(pb);
        qApp->processEvents();
        m_buttonGroupGroups->addButton(pb,query.value(0).toInt());
    }

    QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding );
    ui->scrollArea->widget()->layout()->addItem(spacer);
    connect(m_buttonGroupGroups, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &QRKRegister::quickProductButtons);

}

void QRKRegister::quickProductButtons(int id)
{

    m_buttonGroupProducts = new QButtonGroup(this);

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    bool ok = query.prepare(QString("SELECT color FROM groups WHERE id=%1").arg(id));
    query.exec();
    QString bordercolor = "#808080";
    if (query.next())
        bordercolor = (query.value("color").toString() == "")?bordercolor: query.value(0).toString();

    ok = query.prepare(QString("SELECT id, name, gross, color FROM products WHERE `group`=%1 AND visible=1 ORDER by name").arg(id));

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
    }

    query.exec();

    QWidget *widget = new QWidget(this);
    QGridLayout *gridLayout = new QGridLayout(widget);

    int row = 0;
    int col = 0;

    while (query.next())
    {
        QPushButton *pb = new QPushButton(widget);

        pb->setMinimumHeight(60);
        pb->setMaximumWidth(150);
        QString pbText = Utils::wordWrap(query.value("name").toString(), pb->width(), pb->font());
        pbText = QString("%1\n %2 %3")
                .arg(pbText)
                .arg(QString::number( query.value("gross").toDouble(),'f',2))
                .arg(Database::getShortCurrency());

        pb->setText(pbText);
        pb->setMinimumSize(pb->sizeHint());

        QString backgroundcolor = (query.value("color").toString() == "")?bordercolor: query.value(3).toString();

        QString best_contrast = Utils::color_best_contrast(backgroundcolor);

        pb->setStyleSheet(
                    "QPushButton {"
                    "margin: 1px;"
                    "border-color: " + bordercolor + ";"
                                                     "border-style: outset;"
                                                     "border-radius: 3px;"
                                                     "border-width: 1px;"
                                                     "color: " + best_contrast + ";"
                                                                                 "background-color: " + backgroundcolor + ";" // qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 " + backgroundColor + ", stop: 1 #0d5ca6);"
                    "}"
                    "QPushButton:pressed {"
                    "background-color: " + bordercolor +";" // qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #0d5ca6, stop: 1 " + backgroundColor + ");"
                    "}"
                    );

        m_buttonGroupProducts->addButton(pb,query.value("id").toInt());

        gridLayout->addWidget(pb,row,col);
        col++;
        if (col == 2 ) {
            row++;
            col = 0;
        }
    }

    gridLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(gridLayout);
    ui->scrollAreaProducts->setWidget(widget);
    connect(m_buttonGroupProducts, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &QRKRegister::addProductToOrderList, Qt::QueuedConnection);

}

void QRKRegister::addProductToOrderList(int id)
{
    setFocus();
    bool forceOverwrite = false;
    int rc = m_orderListModel->rowCount();
    if (rc == 0) {
        plusSlot();
        rc = m_orderListModel->rowCount();
        forceOverwrite = true;
    }

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT name, tax, gross FROM products WHERE id=%1").arg(id));
    bool ok = query.exec();

    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
        return;
    }

    if (query.next()) {
        QString name = query.value("name").toString();
        QList<QStandardItem*> list = m_orderListModel->findItems(name, Qt::MatchExactly,REGISTER_COL_PRODUCT);
        if (list.count() > 0 && !forceOverwrite) {
            foreach( QStandardItem *item, list )
            {
                int row = item->row();
                double count = m_orderListModel->item(row, REGISTER_COL_COUNT)->text().toInt();
                count++;
                m_orderListModel->item(row, REGISTER_COL_COUNT)->setText( QString::number(count) );
                QModelIndex idx = m_orderListModel->index(row, REGISTER_COL_COUNT);

                ui->orderList->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
                ui->orderList->edit(idx);

                return;
            }
        }

        bool newItem = m_orderListModel->item(rc -1, REGISTER_COL_PRODUCT)->text().isEmpty();
        if (!newItem && !forceOverwrite) {
            plusSlot();
            rc = m_orderListModel->rowCount();
        }

        m_orderListModel->item(rc -1, REGISTER_COL_COUNT)->setText( "1" );
        m_orderListModel->item(rc -1, REGISTER_COL_PRODUCT)->setText( query.value("name").toString() );
        m_orderListModel->item(rc -1, REGISTER_COL_TAX)->setText( query.value("tax").toString() );
        m_orderListModel->item(rc -1, REGISTER_COL_SINGLE)->setText( query.value("gross").toString() );

        QModelIndex idx = m_orderListModel->index(rc -1, REGISTER_COL_COUNT);
        ui->orderList->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);

    }
}

//--------------------------------------------------------------------------------

void QRKRegister::updateOrderSum()
{
    double sum = 0;
    bool enabled = true;
    int rows = m_orderListModel->rowCount();
    if (rows == 0)
        setButtonGroupEnabled(false);

    for (int row = 0; row < rows; row++)
    {
        QStringList dTemp = m_orderListModel->data(m_orderListModel->index(row, REGISTER_COL_TOTAL, QModelIndex())).toString().split(" ");
        QString sTemp = m_orderListModel->data(m_orderListModel->index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString();
        if (sTemp.isEmpty())
            enabled = false;

        double d4 = dTemp[0].toDouble();

        sum += d4;
    }

    if (rows > 0)
        setButtonGroupEnabled(enabled);

    ui->sumLabel->setText(tr("%1 %2").arg(QString::number(sum, 'f', 2)).arg(Database::getCurrency()));

}

//--------------------------------------------------------------------------------
bool QRKRegister::finishReceipts(int payedBy, int id, bool isReport)
{
    QRKProgress waitBar;

    waitBar.setText(tr("Beleg wird erstellt."));
    waitBar.setWaitMode();
    waitBar.show();

    qApp->processEvents();

    QDateTime dt = QDateTime::currentDateTime();;
    if (m_totallyup) {
        dt.setDate(ui->dateEdit->date());
        if(QDate::currentDate() == ui->dateEdit->date())
            dt.setTime(QTime::currentTime());
        else
            dt.setTime(QTime::fromString("23:59:59"));
    }

    m_orderListModel->setCustomerText(ui->customerText->text());
    m_orderListModel->setReceiptTime(dt);
    bool ret = m_orderListModel->finishReceipts(payedBy, id, isReport);

    waitBar.close();

    return ret;
}

void QRKRegister::setButtonGroupEnabled(bool enabled)
{
    ui->cashReceipt->setEnabled(enabled);
    ui->creditcardReceipt->setEnabled(enabled);
    ui->debitcardReceipt->setEnabled(enabled);
}

//--------------------------------------------------------------------------------

void QRKRegister::newOrder()
{

    m_orderListModel->newOrder(false);

    QStringList list = Database::getLastReceipt();

    ui->customerText->clear();
    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);
    query.prepare("SELECT text FROM customer GROUP BY text");
    query.exec();

    QStringList completerList;

    while(query.next()){
        QString value = query.value("text").toString();
        completerList << value;
    }

    QCompleter *editorCompleter = new QCompleter(completerList) ;
    editorCompleter->setCaseSensitivity( Qt::CaseInsensitive ) ;
    editorCompleter->setFilterMode( Qt::MatchContains );
    ui->customerText->setCompleter( editorCompleter );

    ui->receiptToInvoice->setEnabled(true && RBAC::Instance()->hasPermission("register_r2b"));

    if (!list.empty()) {
        QString date = list.takeAt(0);
        QString bonNr = list.takeAt(0);
        QString taxName = Database::getActionType(list.takeAt(0).toInt());
        double gross = list.takeAt(0).toDouble();

        if (bonNr.length()) {
            ui->lastReceiptLabel->setText(QString(tr("Letzter Barumsatz: BON Nr. %1, %2, %3: %4 %5"))
                                          .arg(bonNr)
                                          .arg(date)
                                          .arg(taxName)
                                          .arg(QString::number(gross, 'f', 2)).arg(Database::getCurrency()));
        }
    }

    ui->newReceiptLabel->setText(QString(tr("Neuer Beleg")));

    ui->orderList->setAutoScroll(true);
    ui->orderList->setWordWrap(true);
    ui->orderList->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->orderList->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    if (m_useDecimalQuantity)
        ui->orderList->setItemDelegateForColumn(REGISTER_COL_COUNT, new QrkDelegate (QrkDelegate::DOUBLE_SPINBOX, this));
    else
        ui->orderList->setItemDelegateForColumn(REGISTER_COL_COUNT, new QrkDelegate (QrkDelegate::SPINBOX, this));
    ui->orderList->setItemDelegateForColumn(REGISTER_COL_PRODUCT, new QrkDelegate (QrkDelegate::PRODUCTS, this));
    ui->orderList->setItemDelegateForColumn(REGISTER_COL_TAX, new QrkDelegate (QrkDelegate::COMBO_TAX, this));
    ui->orderList->setItemDelegateForColumn(REGISTER_COL_NET, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));
    ui->orderList->setItemDelegateForColumn(REGISTER_COL_SINGLE, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));
    ui->orderList->setItemDelegateForColumn(REGISTER_COL_DISCOUNT, new QrkDelegate (QrkDelegate::DISCOUNT, this));
    ui->orderList->setItemDelegateForColumn(REGISTER_COL_TOTAL, new QrkDelegate (QrkDelegate::NUMBERFORMAT_DOUBLE, this));

    ui->orderList->horizontalHeader()->setSectionResizeMode(REGISTER_COL_PRODUCT, QHeaderView::Stretch);
    ui->orderList->setColumnHidden(REGISTER_COL_NET, !m_useInputNetPrice);
    ui->orderList->setColumnHidden(REGISTER_COL_DISCOUNT, !m_useDiscount);
    ui->orderList->setColumnHidden(REGISTER_COL_SAVE, true); /* TODO: Make usable and add code to Settings */

    plusSlot();

}

void QRKRegister::createCheckReceipt(bool)
{

    QRKProgress waitBar;
    waitBar.setText(tr("Beleg wird erstellt."));
    waitBar.setWaitMode();
    waitBar.show();

    qApp->processEvents();

    Reports rep;
    bool ret = rep.checkEOAny();
    if (!ret) {
        return;
    }

    if (m_orderListModel->createNullReceipt(CONTROL_RECEIPT)) {
        emit finishedReceipt();

        if (m_receiptPrintDialog) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Drucker"));
            msgBox.setText(tr("Kontrollbeleg wurde gedruckt. Nächster Vorgang wird gestartet."));

            QTimer::singleShot(10000, &msgBox, SLOT(reject()));
            msgBox.exec();
        }
    } else {
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: can't create CheckReceipt";
    }
}

//-------------SLOTS--------------------------------------------------------------
void QRKRegister::barcodeChangedSlot()
{

    if (ui->barcodeLineEdit->text().isEmpty())
        return;

    QString barcode = ui->barcodeLineEdit->text();
    ui->barcodeLineEdit->clear();

    bool processed = false;
    if (barcodesInterface) {
        int index = ui->orderList->currentIndex().row();
        processed = barcodesInterface->process(m_orderListModel, index, barcode);
    }

    if (!processed) {
        int id = Database::getProductIdByBarcode(barcode);
        if (id)
            addProductToOrderList(id);
    }
}

//--------------------------------------------------------------------------------

void QRKRegister::plusSlot()
{

    m_isR2B = false;

    if (m_orderListModel->rowCount() > 0)
    {

        if (! ui->plusButton->isEnabled()) {
            ui->plusButton->setEnabled(true);
            return;
        }
    }

    m_orderListModel->plus();
}

void QRKRegister::finishedPlus()
{
    int row = m_orderListModel->rowCount() -1;

    if (m_useMaximumItemSold) {
        QStringList list;
        list = Database::getMaximumItemSold();
        m_orderListModel->setItem(row, REGISTER_COL_PRODUCT, new QStandardItem(list.at(0)));
        m_orderListModel->setItem(row, REGISTER_COL_TAX, new QStandardItem(list.at(1)));
        m_orderListModel->setItem(row, REGISTER_COL_SINGLE, new QStandardItem(list.at(2)));
    }

    ui->orderList->selectRow(row);
    ui->orderList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->orderList->setSelectionBehavior(QAbstractItemView::SelectRows);

    /* TODO: Workaround ... resize set only once will not work
         * but here col REGISTER_COL_PRODUCT will lost QHeaderView::Stretch
         */
    ui->orderList->resizeColumnsToContents();
    ui->orderList->horizontalHeader()->setSectionResizeMode(REGISTER_COL_PRODUCT, QHeaderView::Stretch);

    if (m_orderListModel->rowCount() > 1)
        ui->receiptToInvoice->setEnabled(false);

    ui->plusButton->setEnabled(true);

    QModelIndex idx = m_orderListModel->index(row, REGISTER_COL_COUNT);

    ui->orderList->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
    ui->orderList->edit(idx);

    updateOrderSum();

}

//--------------------------------------------------------------------------------

void QRKRegister::minusSlot()
{

    m_isR2B = false;
    //get selections
    QItemSelection selection = ui->orderList->selectionModel()->selection();

    //find out selected rows
    QList<int> removeRows;
    foreach(QModelIndex index, selection.indexes()) {
        if(!removeRows.contains(index.row())) {
            removeRows.append(index.row());
        }
    }

    //loop through all selected rows
    for(int i=0;i<removeRows.count();++i)
    {
        //decrement all rows after the current - as the row-number will change if we remove the current
        for(int j=i;j<removeRows.count();++j) {
            if(removeRows.at(j) > removeRows.at(i)) {
                removeRows[j]--;
            }
        }
        //remove the selected row
        m_orderListModel->removeRow(removeRows.at(i));
    }

    if (m_orderListModel->rowCount() < 2)
        ui->receiptToInvoice->setEnabled(true && RBAC::Instance()->hasPermission("register_r2b"));

    if (m_orderListModel->rowCount() < 1)
        ui->plusButton->setEnabled(true);

    updateOrderSum();
}

//--------------------------------------------------------------------------------

void QRKRegister::onButtonGroup_payNow_clicked(int payedBy)
{

    setButtonGroupEnabled(false);

    QDate date;
    if (m_totallyup)
        date = ui->dateEdit->date();
    else
        date = QDate::currentDate();

    Reports rep;
    bool ret = rep.checkEOAny(date);
    if (!ret) {
        setButtonGroupEnabled(true);
        return;
    }

    if (m_useGivenDialog && payedBy == PAYED_BY_CASH) {
        double sum = ui->sumLabel->text().replace(Database::getCurrency() ,"").toDouble();
        GivenDialog given(sum, this);
        if (given.exec() == 0) {
            qApp->processEvents();
            setButtonGroupEnabled(true);
            return;
        }
    }

    if (m_orderListModel->rowCount() > 0)
    {
        int rc = m_orderListModel->rowCount();

        QList<QVariant> list;

        for(int row = 0; row < rc; row++) {
            /* TODO: check for Autosave */
            bool checked = m_orderListModel->item(row ,REGISTER_COL_SAVE)->checkState();
            Q_UNUSED(checked);

            list.clear();
            list << m_orderListModel->data(m_orderListModel->index(row, REGISTER_COL_PRODUCT, QModelIndex())).toString()
                 << m_orderListModel->data(m_orderListModel->index(row, REGISTER_COL_TAX, QModelIndex())).toString()
                 << m_orderListModel->data(m_orderListModel->index(row, REGISTER_COL_NET, QModelIndex())).toString()
                 << m_orderListModel->data(m_orderListModel->index(row, REGISTER_COL_SINGLE, QModelIndex())).toString()
                 << "1";

            Database::addProduct(list);
        }
    }

    QSqlDatabase dbc = Database::database();
    dbc.transaction();

    m_currentReceipt = m_orderListModel->createReceipts();
    bool sql_ok = true;
    if ( m_currentReceipt ) {
        if (!ui->customerText->text().isEmpty())
            sql_ok = Database::addCustomerText(m_currentReceipt, ui->customerText->text());

        if ( m_orderListModel->createOrder() ) {
            if ( finishReceipts(payedBy) ){
                emit finishedReceipt();
            } else {
                sql_ok = false;
            }
        } else {
            sql_ok = false;
        }
    }

    if (sql_ok) {
        dbc.commit();
        if (m_receiptPrintDialog) {
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Drucker"));
            msgBox.setText(tr("Beleg %1 wurde gedruckt. Nächster Vorgang wird gestartet.").arg(m_currentReceipt));

            QTimer::singleShot(10000, &msgBox, SLOT(reject()));

            msgBox.exec();
        }
    } else {
        sql_ok = dbc.rollback();
        QMessageBox::warning(this, tr("Fehler"), tr("Datenbank und/oder Signatur Fehler!\nAktueller BON kann nicht erstellt werden. (Rollback: %1).\nÜberprüfen Sie ob genügend Speicherplatz für die Datenbank vorhanden ist. Weitere Hilfe gibt es im Forum. http:://www.ckvsoft.at").arg(sql_ok?tr("durchgeführt"):tr("fehlgeschlagen") ));
        qCritical() << "Function Name: " << Q_FUNC_INFO << " Error: " << dbc.lastError().text();
    }
}

//--------------------------------------------------------------------------------

void QRKRegister::totallyupSlot()
{
    ui->totallyup->setVisible(true);
    m_totallyup = true;
}

//--------------------------------------------------------------------------------

void QRKRegister::totallyupExitSlot()
{
    ui->totallyup->setVisible(false);
    m_totallyup = false;
    ui->dateEdit->setDate(QDate::currentDate());
}

//--------------------------------------------------------------------------------

void QRKRegister::receiptToInvoiceSlot()
{

    bool checkReceiptButtonSaved = ui->checkReceiptButton->isEnabled();
    bool receiptToInvoiceSaved = ui->receiptToInvoice->isEnabled();
    ui->checkReceiptButton->setEnabled(false);
    ui->receiptToInvoice->setEnabled(false);

    R2BDialog r2b(this);
    if ( r2b.exec() == QDialog::Accepted )
    {
        int rc = m_orderListModel->rowCount();
        if (rc == 0) {
            plusSlot();
            rc++;
        }

        bool productExists = Database::exists("products", r2b.getInvoiceNum());
        if (productExists) {
            QMessageBox msgWarning(QMessageBox::Warning,"","");
            msgWarning.setWindowTitle(QObject::tr("Warnung"));
            msgWarning.setText(QObject::tr("Die angegebene Rechnungsnummer wurde schon verwendet. Sollte es sich um einen Irrtum handeln dann Klicken Sie bitte nochmals den \"BON zu Rechnung #\" Button."));
            msgWarning.exec();
        }

        if (rc == 1) {
            m_isR2B = true;

            m_orderListModel->blockSignals(true);
            m_orderListModel->item(0, REGISTER_COL_COUNT)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_orderListModel->item(0, REGISTER_COL_PRODUCT)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_orderListModel->item(0, REGISTER_COL_NET)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_orderListModel->item(0, REGISTER_COL_TAX)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_orderListModel->item(0, REGISTER_COL_SINGLE)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_orderListModel->item(0, REGISTER_COL_DISCOUNT)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
            m_orderListModel->item(0, REGISTER_COL_TOTAL)->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

            m_orderListModel->item(0, REGISTER_COL_COUNT)->setText( "1" );
            m_orderListModel->item(0, REGISTER_COL_PRODUCT)->setText( r2b.getInvoiceNum() );
            m_orderListModel->item(0, REGISTER_COL_TAX)->setText( "0" );
            m_orderListModel->blockSignals(false);
            m_orderListModel->item(0, REGISTER_COL_SINGLE)->setText( r2b.getInvoiceSum() );

            ui->plusButton->setEnabled(false);

        }
    }
    ui->checkReceiptButton->setEnabled(checkReceiptButtonSaved);
    ui->receiptToInvoice->setEnabled(receiptToInvoiceSaved && RBAC::Instance()->hasPermission("register_r2b"));

}

//--------------------------------------------------------------------------------

void QRKRegister::clearModel()
{
    m_orderListModel->clear();
    m_orderListModel->setRowCount(0);
}

//--------------------------------------------------------------------------------

void QRKRegister::setCurrentReceiptNum(int id)
{
    m_currentReceipt = id;
}

//--------------------------------------------------------------------------------

void QRKRegister::onCancelRegisterButton_clicked()
{
    if (m_orderListModel->rowCount() > 0 ) {
        if (m_orderListModel->item(0, REGISTER_COL_PRODUCT)->text() == "") {
            emit cancelRegisterButton_clicked();
            return;
        }
    } else {
        emit cancelRegisterButton_clicked();
        return;
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(tr("Kassenmodus verlassen? Ein nicht abgeschlossener Bon wird gelöscht!"));
    msgBox.setStandardButtons(QMessageBox::Yes);
    msgBox.addButton(QMessageBox::No);
    msgBox.setButtonText(QMessageBox::Yes, tr("Ja"));
    msgBox.setButtonText(QMessageBox::No, tr("Nein"));
    msgBox.setDefaultButton(QMessageBox::No);

    if(msgBox.exec() == QMessageBox::Yes){
        emit cancelRegisterButton_clicked();
    }
    ui->barcodeLineEdit->setEnabled(false);
}

void QRKRegister::setColumnHidden(int col)
{
    ui->orderList->setColumnHidden(col, true);
}

void QRKRegister::initPlugins()
{
    if (!barcodesInterface) {
        barcodesInterface = qobject_cast<BarcodesInterface *>(PluginManager::instance()->getObjectByName("BarCodes"));
        if (barcodesInterface) {
            connect(barcodesInterface, &BarcodesInterface::minusSlot, this, &QRKRegister::minusSlot, Qt::DirectConnection);
            connect(barcodesInterface, &BarcodesInterface::setColumnHidden, this, &QRKRegister::setColumnHidden, Qt::DirectConnection);
            connect(barcodesInterface, &BarcodesInterface::finishedReceipt, this, &QRKRegister::finishedReceipt, Qt::DirectConnection);
        }
    }
}
