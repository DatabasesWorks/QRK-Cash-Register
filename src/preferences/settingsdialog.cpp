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

#include <QtWidgets>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QPrinterInfo>
#include <QStackedWidget>
#include <QElapsedTimer>
#include <QCompleter>

#include "font/fontselector.h"
#include "documentprinter.h"
#include "settingsdialog.h"
#include "qrkregister.h"
#include "utils/demomode.h"
#include "RK/rk_smartcardinfo.h"
#include "RK/rk_signaturemodulefactory.h"
#include "database.h"
#include "reports.h"
#include "preferences/qrksettings.h"
#include "textedit.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
// #include "pluginmanager/pluginmanager.h"
#include "pluginmanager/pluginview.h"
#include "qrkprogress.h"
#include "qrkpushbutton.h"
#include "qrkmultimedia.h"
#include "customtabstyle.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
    QElapsedTimer timer;
    timer.start();
    QStringList availablePrinters = QPrinterInfo::availablePrinterNames();
    qDebug() << "Function Name: " << Q_FUNC_INFO << "availablePrinters loaded. elapsed: " << timer.elapsed();

    m_journal = new Journal(this);
    m_general = new GeneralTab(this);
    m_master = new MasterDataTab(this);
    m_printer = new PrinterTab(availablePrinters, this);
    m_receiptprinter = new ReceiptPrinterTab(availablePrinters, this);
    m_receiptmain = new ReceiptMainTab(this);
    m_receipt = new ReceiptTab(availablePrinters, this);
    m_receiptenhanced = new ReceiptEnhancedTab(this);
    m_barcode = new BarcodeTab(this);
    m_extra = new ExtraTab(this);
    m_server = new ServerTab(this);
    m_scardreader = new SCardReaderTab(this);
    m_general->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    m_scardreader->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    PluginView *pluginview = new PluginView(this);

    m_tabWidget = new QTabWidget(this);
    QTabWidget *receiptTabWidget = new QTabWidget;
    receiptTabWidget->addTab(m_receiptmain, tr("Kassa"));
    receiptTabWidget->addTab(m_receipt, tr("Kassa Bon"));
    receiptTabWidget->addTab(m_receiptenhanced, tr("Kassa Bon erweitert"));
    receiptTabWidget->addTab(m_barcode, tr("Barcode Lesegerät"));

    // FIXME: Apple do not Display Text with CustomTabStyle and QTabWidget::West
#ifndef __APPLE__
    m_tabWidget->setTabPosition(QTabWidget::West);
    m_tabWidget->tabBar()->setStyle(new CustomTabStyle);
#endif
    m_tabWidget->addTab(m_master, tr("Stammdaten"));
    m_tabWidget->addTab(m_printer, tr("Drucker"));
    m_tabWidget->addTab(m_receiptprinter, tr("BON Drucker"));
    m_tabWidget->addTab(receiptTabWidget, tr("Kassa"));
//    m_tabWidget->addTab(m_receipt, tr("Kassa BON"));
//    m_tabWidget->addTab(m_receiptenhanced, tr("Kassa BON erweitert"));
    m_tabWidget->addTab(m_general, tr("Verzeichnispfade"));
    m_tabWidget->addTab(m_extra, tr("Extra"));
    m_tabWidget->addTab(m_server, tr("Import Server"));
    if (m_master->getShopTaxes() == "AT" && !Database::isCashRegisterInAktive())
        m_tabWidget->addTab(m_scardreader, tr("SignaturErstellungsEinheit"));
    m_tabWidget->addTab(pluginview, tr("Plugins"));

    QrkPushButton *OkPushButton = new QrkPushButton(QIcon(":src/icons/ok.png"), tr("Speichern"));
    QrkPushButton *CancelPushButton = new QrkPushButton(QIcon(":src/icons/cancel.png"), tr("Abbrechen"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding );
    buttonLayout->addItem(spacer);
    buttonLayout->addWidget(CancelPushButton);
    buttonLayout->addWidget(OkPushButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    setLayout(mainLayout);

    setWindowTitle(tr("Einstellungen"));
    setMinimumWidth(700);

    if ( QApplication::desktop()->height() < 650 )
    {
        OkPushButton->setMinimumHeight(0);
        CancelPushButton->setMinimumHeight(0);
        setMinimumWidth(700);
        setFixedHeight(550);
    }

    connect(OkPushButton, &QPushButton::clicked, this, &SettingsDialog::accept);
    connect(CancelPushButton, &QPushButton::clicked, this, &SettingsDialog::close);
    connect(m_master, &MasterDataTab::taxChanged, this, &SettingsDialog::masterTaxChanged);
    connect(m_master, &MasterDataTab::taxChanged, m_general, &GeneralTab::masterTaxChanged);
    connect(m_receiptmain, &ReceiptMainTab::maximumSoldItemChanged, m_extra, &ExtraTab::maximumSoldItemChanged);

    qDebug() << "Function Name: " << Q_FUNC_INFO << "finished elapsed: " << timer.elapsed();
}

void SettingsDialog::masterTaxChanged(QString tax)
{
    if (tax == "AT" && !Database::isCashRegisterInAktive()) {
        m_tabWidget->addTab(m_scardreader, tr("SignaturErstellungsEinheit"));
    } else {
        int idx = m_tabWidget->indexOf(m_scardreader);
        m_tabWidget->removeTab(idx);
    }
}

void SettingsDialog::accept()
{

    QrkSettings settings;

    settings.save2Database("printAdvertisingText", m_receiptenhanced->getAdvertisingText());
    settings.save2Database("printHeader", m_receiptenhanced->getHeader());
    settings.save2Database("printFooter", m_receiptenhanced->getFooter());

    settings.save2Database("shopName", m_master->getShopName());
    settings.save2Database("shopOwner", m_master->getShopOwner());
    settings.save2Database("shopAddress", m_master->getShopAddress());
    settings.save2Database("shopUid", m_master->getShopUid());
    settings.save2Database("shopCashRegisterId", m_master->getShopCashRegisterId());
    settings.save2Database("currency", m_master->getShopCurrency());
    settings.save2Database("taxlocation", m_master->getShopTaxes());
    settings.save2Database("defaulttax", m_receiptmain->getDefaultTax());

    settings.save2Settings("useLogo", m_receipt->getUseLogo());
    settings.save2Settings("logo", m_receipt->getLogo());
    settings.save2Settings("virtualNumPad", m_receiptmain->useVirtualNumPad());

    settings.save2Settings("useAdvertising", m_receiptenhanced->getUseAdvertising());
    settings.save2Settings("advertising", m_receiptenhanced->getAdvertising());

    QString oldDataDir = settings.value("sqliteDataDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data").toString();
    settings.save2Settings("sqliteDataDirectory", m_general->getDataDirectory());

    if (oldDataDir != settings.value("sqliteDataDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data").toString()) {
        Database::reopen();
    }

    settings.save2Settings("importDirectory", m_server->getImportDirectory());
    settings.save2Settings("importCodePage", m_server->getImportCodePage());
    settings.save2Settings("importServerFullscreen", m_server->getServerFullscreen());
    settings.save2Settings("serverCriticalMessageBox", m_server->getServerCriticalMessageBox());

    settings.save2Settings("backupDirectory", m_general->getBackupDirectory());
    settings.save2Settings("keepMaxBackups", m_general->getKeepMaxBackups());
    settings.save2Settings("pdfDirectory", m_general->getPdfDirectory());
    settings.save2Settings("externalDepDirectory", m_general->getExternalDepDirectory());

    if (m_extra->isFontsGroup()) {
        settings.save2Settings("systemfont", m_extra->getSystemFont());
        settings.save2Settings("printerfont", m_extra->getPrinterFont());
        settings.save2Settings("receiptprinterfont", m_extra->getReceiptPrinterFont());
    } else {
        settings.removeSettings("systemfont");
        settings.removeSettings("printerfont");
        settings.removeSettings("receiptprinterfont");
    }

    settings.save2Settings("useInputNetPrice", m_receiptmain->getInputNetPrice());
    settings.save2Settings("useDiscount", m_receiptmain->getDiscount());
    settings.save2Settings("useMaximumItemSold", m_receiptmain->getMaximumItemSold());
    settings.save2Settings("decimalDigits", m_receiptmain->getDecimalDigits());
    settings.save2Settings("useDecimalQuantity", m_receiptmain->getDecimalQuantity());
    settings.save2Settings("useInputProductNumber", m_receiptmain->useInputProductNumber());
    settings.save2Settings("saveRegisterHeaderGeo", m_receiptmain->getRegisterHeaderMovable());
    if (!m_receiptmain->getRegisterHeaderMovable()) {
        settings.beginGroup("Register");
        settings.removeSettings("HorizontalHeaderColumnGeo");
        settings.removeSettings("HorizontalHeaderColumnSta");
        settings.endGroup();
    }

    settings.beginGroup("BarcodeReader");
    settings.save2Settings("barcodeReaderPrefix", m_barcode->getBarcodePrefix());
    settings.save2Settings("barcode_success_sound", m_barcode->getBarcodeSuccessSound());
    settings.save2Settings("barcode_failure_sound", m_barcode->getBarcodeFailureSound());
    settings.save2Settings("barcode_success_enabled", m_barcode->getBarcodeSuccessSoundEnabled());
    settings.save2Settings("barcode_failure_enabled", m_barcode->getBarcodeFailureSoundEnabled());
    settings.save2Settings("barcode_multimedia_path", m_barcode->getMultimediaPath());
    settings.save2Settings("barcode_input_default", m_barcode->getBarcodeAsDefault());
    settings.endGroup();

    settings.save2Settings("hideCreditcardButton", m_receiptmain->hideCreditcardButton());
    settings.save2Settings("hideDebitcardButton", m_receiptmain->hideDebitcardButton());
    settings.save2Settings("hideR2BButton", m_receiptmain->hideR2BButton());
    settings.save2Settings("quickButtonSize", m_receiptmain->getQuickButtonSize());
    settings.save2Settings("ButtonSize", m_receiptmain->getButtonSize());
    settings.save2Settings("numpadButtonSize", m_receiptmain->getNumpadButtonSize());

    settings.save2Settings("useGivenDialog", m_extra->getGivenDialog());
    settings.save2Settings("usePriceChangedDialog", m_extra->getPriceChangedDialog());
    settings.save2Settings("showSalesWidget", m_extra->getSalesWidget());
    settings.save2Settings("useReceiptPrintedDialog", m_extra->getReceiptPrintedDialog());
    settings.save2Settings("report_by_productgroup", m_extra->getProductGroup());
    settings.save2Settings("useMinstockDialog", m_extra->getStockDialog());
    settings.save2Settings("numericGroupSeparator", m_extra->getGroupSeparator());
    settings.save2Settings("firstProductnumber", m_extra->getFirstProductnumber());

    settings.save2Settings("reportPrinterPDF", m_printer->getReportPrinterPDF());
    settings.save2Settings("reportPrinter", m_printer->getReportPrinter());
    settings.save2Settings("paperFormat", m_printer->getPaperFormat());

    settings.save2Settings("invoiceCompanyPrinter", m_printer->getInvoiceCompanyPrinter());
    settings.save2Settings("invoiceCompanyPaperFormat", m_printer->getInvoiceCompanyPaperFormat());
    settings.save2Settings("invoiceCompanyMarginLeft", m_printer->getInvoiceCompanyMarginLeft());
    settings.save2Settings("invoiceCompanyMarginTop", m_printer->getInvoiceCompanyMarginTop());
    settings.save2Settings("invoiceCompanyMarginRight", m_printer->getInvoiceCompanyMarginRight());
    settings.save2Settings("invoiceCompanyMarginBottom", m_printer->getInvoiceCompanyMarginBottom());

    settings.save2Settings("receiptPrinterHeading", m_receipt->getReceiptPrinterHeading());
    settings.save2Settings("receiptPrinter", m_receiptprinter->getReceiptPrinter());
    settings.save2Settings("printCollectionReceipt", m_receipt->getPrintCollectionReceipt());
    settings.save2Settings("collectionPrinter", m_receipt->getCollectionPrinter());
    settings.save2Settings("collectionReceiptCopies", m_receipt->getCollectionReceiptCopies());
    settings.save2Settings("collectionReceiptText", m_receipt->getCollectionReceiptText());
    settings.save2Settings("collectionPrinterPaperFormat", m_receipt->getCollectionPrinterPaperFormat());
    settings.save2Settings("printCompanyNameBold", m_receipt->getPrintCompanyNameBold());
    settings.save2Settings("useReportPrinter", m_receiptprinter->getUseReportPrinter());
    settings.save2Settings("logoRight", m_receipt->getIsLogoRight());
    settings.save2Settings("qrcode", m_receipt->getPrintQRCode());
    settings.save2Settings("qrcodeleft", m_receipt->getPrintQRCodeLeft());
    settings.save2Settings("numberCopies", m_receiptprinter->getNumberCopies());
    settings.save2Settings("paperWidth", m_receiptprinter->getpaperWidth());
    settings.save2Settings("paperHeight", m_receiptprinter->getpaperHeight());
    settings.save2Settings("marginLeft", m_receiptprinter->getmarginLeft());
    settings.save2Settings("marginTop", m_receiptprinter->getmarginTop());
    settings.save2Settings("marginRight", m_receiptprinter->getmarginRight());
    settings.save2Settings("marginBottom", m_receiptprinter->getmarginBottom());

    settings.save2Settings("feedProdukt", m_receiptprinter->getfeedProdukt());
    settings.save2Settings("feedCompanyHeader", m_receiptprinter->getfeedCompanyHeader());
    settings.save2Settings("feedCompanyAddress", m_receiptprinter->getfeedCompanyAddress());
    settings.save2Settings("feedCashRegisterid", m_receiptprinter->getfeedCashRegisterid());
    settings.save2Settings("feedTimestamp", m_receiptprinter->getfeedTimestamp());
    settings.save2Settings("feedTax", m_receiptprinter->getfeedTax());
    settings.save2Settings("feedPrintHeader", m_receiptprinter->getfeedPrintHeader());
    settings.save2Settings("feedHeaderText", m_receiptprinter->getfeedHeaderText());
    settings.save2Settings("feedQRCode", m_receiptprinter->getfeedQRCode());

    m_scardreader->saveSettings();

    /* remove unused settings*/
    settings.removeSettings("dataDirectory");

    QDialog::accept();

}

BarcodeTab::BarcodeTab(QWidget *parent)
    : Widget(parent)
{
    QrkSettings settings;

    /* BarcodePrefixes */
    m_barcodePrefixesComboBox = new QComboBox();
    m_barcodePrefixesComboBox->addItem("F1", Qt::Key_F1);
    m_barcodePrefixesComboBox->addItem("F2", Qt::Key_F2);
    m_barcodePrefixesComboBox->addItem("F3", Qt::Key_F3);
    m_barcodePrefixesComboBox->addItem("F4", Qt::Key_F4);
    m_barcodePrefixesComboBox->addItem("F5", Qt::Key_F5);
    m_barcodePrefixesComboBox->addItem("F6", Qt::Key_F6);
    m_barcodePrefixesComboBox->addItem("F7", Qt::Key_F7);
    m_barcodePrefixesComboBox->addItem("F8", Qt::Key_F8);
    m_barcodePrefixesComboBox->addItem("F9", Qt::Key_F9);
    m_barcodePrefixesComboBox->addItem("F10", Qt::Key_F10);
    m_barcodePrefixesComboBox->addItem("F11", Qt::Key_F11);
    m_barcodePrefixesComboBox->addItem("F12", Qt::Key_F12);

    m_barcodeMultimediaPath = new QLineEdit;
    m_barcodeMultimediaPath->setReadOnly(true);

    QPushButton *barcodeMultimediaPathButton = new QPushButton(QIcon(":src/icons/save.png"), "");
    barcodeMultimediaPathButton->setIconSize(QSize(24,24));

    connect(barcodeMultimediaPathButton, &QPushButton::clicked, this, &BarcodeTab::setMultimediaPath);

    QStringList list = QrkMultimedia::getMultimediaFiles();
    m_barcodeSuccessCombo = new QComboBox();
    m_barcodeSuccessCombo->addItem("success.wav",":src/multimedia/success.wav");
    m_barcodeFailureCombo = new QComboBox();
    m_barcodeFailureCombo->addItem("failure.wav",":src/multimedia/failure.wav");

    foreach(QString filename, list) {
        m_barcodeSuccessCombo->addItem(filename, getMultimediaPath() + QDir::separator() + filename);
        m_barcodeFailureCombo->addItem(filename, getMultimediaPath() + QDir::separator() + filename);
    }

    QPushButton *test_successSound = new QPushButton(QIcon(":src/icons/playsound.png"), "");
    test_successSound->setIconSize(QSize(24,24));
    test_successSound->setProperty("what", BARCODE_SUCCSESS);
    connect(test_successSound, &QPushButton::clicked, this, &BarcodeTab::play);
    QPushButton *test_failureSound = new QPushButton(QIcon(":src/icons/playsound.png"), "");
    test_failureSound->setIconSize(QSize(24,24));
    test_failureSound->setProperty("what", BARCODE_FAILURE);
    connect(test_failureSound, &QPushButton::clicked, this, &BarcodeTab::play);

    m_barcodeSoundSuccess = new QCheckBox();
    m_barcodeSoundFailure = new QCheckBox();

    m_barcodeInputLineEditDefault = new QCheckBox();

    QGroupBox *barcodeGroup = new QGroupBox();
    barcodeGroup->setTitle(tr("Barcode Lesegerät"));

    QGridLayout *barcodeLayout = new QGridLayout;
    barcodeLayout->setAlignment(Qt::AlignLeft);

    barcodeLayout->addWidget( new QLabel(tr("Prefix:")), 1,1,1,1);
    barcodeLayout->addWidget( m_barcodePrefixesComboBox, 1,2,1,1);

    barcodeLayout->addWidget( new QLabel(tr("Barcode-Eingabe als Voreinstellung:")), 2,1,1,1);
    barcodeLayout->addWidget( m_barcodeInputLineEditDefault, 2,2,1,1);

    barcodeLayout->addWidget( new QLabel(tr("Verzeichnis für eigene Töne:")), 3,1,1,1);
    barcodeLayout->addWidget( m_barcodeMultimediaPath, 3,2,1,2);
    barcodeLayout->addWidget( barcodeMultimediaPathButton, 3,4,1,1);

    barcodeLayout->addWidget( new QLabel(tr("Ton für erfolgreichen Scan:")), 4,1,1,1);
    barcodeLayout->addWidget( m_barcodeSoundSuccess, 4,2,1,1);
    barcodeLayout->addWidget( m_barcodeSuccessCombo, 4,3,1,1);
    barcodeLayout->addWidget( test_successSound, 4,4,1,1);

    barcodeLayout->addWidget( new QLabel(tr("Ton für fehlerhaften Scan:")), 5,1,1,1);
    barcodeLayout->addWidget( m_barcodeSoundFailure, 5,2,1,1);
    barcodeLayout->addWidget( m_barcodeFailureCombo, 5,3,1,1);
    barcodeLayout->addWidget( test_failureSound, 5,4,1,1);

    barcodeLayout->setColumnStretch(1,1);
    barcodeLayout->setColumnStretch(3,1);

    barcodeGroup->setLayout(barcodeLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(barcodeGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    settings.beginGroup("BarcodeReader");
    int idx = m_barcodePrefixesComboBox->findData(settings.value("barcodeReaderPrefix", Qt::Key_F11).toInt());
    if ( idx != -1 ) {
        m_barcodePrefixesComboBox->setCurrentIndex(idx);
    }

    idx = m_barcodeSuccessCombo->findData(settings.value("barcode_success_sound", ":src/multimedia/success.wav").toString());
    if ( idx != -1 ) {
        m_barcodeSuccessCombo->setCurrentIndex(idx);
    }
    idx = m_barcodeFailureCombo->findData(settings.value("barcode_failure_sound", ":src/multimedia/failure.wav").toString());
    if ( idx != -1 ) {
        m_barcodeFailureCombo->setCurrentIndex(idx);
    }

    m_barcodeSoundSuccess->setChecked(settings.value("barcode_success_enabled", false).toBool());
    m_barcodeSoundFailure->setChecked(settings.value("barcode_failure_enabled", false).toBool());
    m_barcodeMultimediaPath->setText(settings.value("barcode_multimedia_path", qApp->applicationDirPath()).toString());
    m_barcodeInputLineEditDefault->setChecked(settings.value("barcode_input_default", false).toBool());
    settings.endGroup();

}

bool BarcodeTab::getBarcodeAsDefault()
{
    return m_barcodeInputLineEditDefault->isChecked();
}

QString BarcodeTab::getMultimediaPath()
{
    return m_barcodeMultimediaPath->text();
}

void BarcodeTab::setMultimediaPath()
{
    QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis Auswahl"),
                                                     getMultimediaPath(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    if (!path.isEmpty()) {
        m_barcodeMultimediaPath->setText(path);
        QStringList list = QrkMultimedia::getMultimediaFiles(getMultimediaPath());

        m_barcodeFailureCombo->clear();
        m_barcodeSuccessCombo->clear();
        m_barcodeSuccessCombo->addItem("success.wav",":src/multimedia/success.wav");
        m_barcodeFailureCombo->addItem("failure.wav",":src/multimedia/failure.wav");

        foreach(QString filename, list) {
            m_barcodeSuccessCombo->addItem(filename, getMultimediaPath() + QDir::separator() + filename);
            m_barcodeFailureCombo->addItem(filename, getMultimediaPath() + QDir::separator() + filename);
        }
    }
}

void BarcodeTab::play()
{
    QPushButton* button = static_cast<QPushButton*>(sender());
    int what = button->property("what").toInt();
    switch (what) {
    case BARCODE_SUCCSESS:
        QrkMultimedia::play(getBarcodeSuccessSound());
        break;
    case BARCODE_FAILURE:
        QrkMultimedia::play(getBarcodeFailureSound());
        break;
    default:
        break;
    }

}

int BarcodeTab::getBarcodePrefix()
{
    int idx  = m_barcodePrefixesComboBox->currentIndex();
    return m_barcodePrefixesComboBox->itemData(idx).toInt();
}

QString BarcodeTab::getBarcodeSuccessSound()
{
    return m_barcodeSuccessCombo->currentData().toString();
}

QString BarcodeTab::getBarcodeFailureSound()
{
    return m_barcodeFailureCombo->currentData().toString();
}

bool BarcodeTab::getBarcodeSuccessSoundEnabled()
{
    return m_barcodeSoundSuccess->isChecked();
}

bool BarcodeTab::getBarcodeFailureSoundEnabled()
{
    return m_barcodeSoundFailure->isChecked();
}

ReceiptMainTab::ReceiptMainTab(QWidget *parent)
    : Widget(parent)
{
    QrkSettings settings;

    m_useInputNetPriceCheck = new QCheckBox;
    m_useInputNetPriceCheck->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_useDiscountCheck = new QCheckBox;
    m_useDiscountCheck->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_useMaximumItemSoldCheck = new QCheckBox;
    m_useMaximumItemSoldCheck->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_useDecimalQuantityCheck = new QCheckBox;
//    m_useDecimalQuantityCheck->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_decimalRoundSpin = new QSpinBox;
    m_decimalRoundSpin->setRange(2,3);
    m_decimalRoundSpin->setSuffix(" " + tr("Stellen"));
    m_hideDebitcardCheck = new QCheckBox;
    m_hideCreditcardCheck = new QCheckBox;
    m_hideR2BButtonCheck = new QCheckBox;

    /* Default Taxes*/
    m_defaultTaxComboBox = new QComboBox();
    QSqlDatabase dbc= Database::database();
    QSqlQuery query(dbc);
    query.prepare(QString("SELECT tax FROM taxTypes WHERE taxlocation=:taxlocation ORDER BY id"));
    query.bindValue(":taxlocation", Database::getTaxLocation());
    if(!query.exec()){
      qWarning() << "Function Name: " << Q_FUNC_INFO << " Query:" << Database::getLastExecutedQuery(query);
    }
    while(query.next()){
      m_defaultTaxComboBox->addItem(query.value(0).toString());
    }
    m_defaultTaxComboBox->setCurrentText(Database::getDefaultTax());

    /* virtual numpad*/
    m_useVirtualNumPadCheck = new QCheckBox();
    m_useInputProductNumberCheck = new QCheckBox();
    m_registerHeaderMoveableCheck = new QCheckBox();

    QGroupBox *registerGroup = new QGroupBox();
    registerGroup->setTitle(tr("Kassa"));

    QGridLayout *extraLayout = new QGridLayout;
    extraLayout->setAlignment(Qt::AlignLeft);

    extraLayout->addWidget( new QLabel(tr("Netto Eingabe ermöglichen:")), 1,1,1,1);
    extraLayout->addWidget( m_useInputNetPriceCheck, 1,2,1,1);
    extraLayout->addWidget( new QLabel(tr("Standard Steuersatz:")), 2,1,1,1);
    extraLayout->addWidget( m_defaultTaxComboBox, 2,2,1,1);

    extraLayout->addWidget( new QLabel(tr("Rabatt Eingabe ermöglichen:")), 1,3,1,1);
    extraLayout->addWidget( m_useDiscountCheck, 1,4,1,1);
    extraLayout->addWidget( new QLabel(tr("Kreditkarten Taste verbergen:")), 2,3,1,1);
    extraLayout->addWidget( m_hideCreditcardCheck, 2,4,1,1);
    extraLayout->addWidget( new QLabel(tr("Bankomat Taste verbergen:")), 3,3,1,1);
    extraLayout->addWidget( m_hideDebitcardCheck, 3,4,1,1);
    extraLayout->addWidget( new QLabel(tr("Bon zu Rechnung Taste verbergen:")), 4,3,1,1);
    extraLayout->addWidget( m_hideR2BButtonCheck, 4,4,1,1);

    extraLayout->addWidget( new QLabel(tr("Virtuellen Ziffernblock verwenden:")),3,1,1,1);
    extraLayout->addWidget( m_useVirtualNumPadCheck, 3,2,1,1);

    extraLayout->addWidget( new QLabel(tr("Artikelnummer Eingabe ermöglichen:")),4,1,1,1);
    extraLayout->addWidget( m_useInputProductNumberCheck, 4,2,1,1);
    extraLayout->addWidget( new QLabel(tr("Kopfzeilen verschiebbar:")),5,1,1,1);
    extraLayout->addWidget( m_registerHeaderMoveableCheck, 5,2,1,1);

    extraLayout->addWidget( new QLabel(tr("Meistverkauften Artikel als Standard Artikel verwenden:")), 6,1,1,3);
    extraLayout->addWidget( m_useMaximumItemSoldCheck, 6,4,1,2);

    extraLayout->addWidget( new QLabel(tr("Dezimale Eingabe bei Anzahl Artikel:")),7,1,1,2);
    extraLayout->addWidget( m_useDecimalQuantityCheck, 7,4,1,1);
    extraLayout->addWidget( m_decimalRoundSpin, 7,3,1,1);

    extraLayout->setColumnStretch(1,1);
    extraLayout->setColumnStretch(3,1);

    registerGroup->setLayout(extraLayout);

    QGroupBox *quickButtonGroup = new QGroupBox();
    quickButtonGroup->setTitle(tr("Schaltflächen (Tasten)"));

    QGridLayout *quickButtonLayout = new QGridLayout;
    quickButtonLayout->setAlignment(Qt::AlignLeft);

    m_quickButtonWidth = new QSpinBox;
    m_quickButtonWidth->setMinimum(100);
    m_quickButtonWidth->setMaximum(300);

    m_quickButtonHeight = new QSpinBox;
    m_quickButtonHeight->setMinimum(50);
    m_quickButtonHeight->setMaximum(300);

    m_fixedButtonHeight = new QSpinBox;
    m_fixedButtonHeight->setMinimum(50);
    m_fixedButtonHeight->setMaximum(120);

    m_minimumButtonWidth = new QSpinBox;
    m_minimumButtonWidth->setMinimum(100);
    m_minimumButtonWidth->setMaximum(300);

    m_fixedNumpadButtonHeight = new QSpinBox;
    m_fixedNumpadButtonHeight->setMinimum(30);
    m_fixedNumpadButtonHeight->setMaximum(100);

    m_fixedNumpadButtonWidth = new QSpinBox;
    m_fixedNumpadButtonWidth->setMinimum(30);
    m_fixedNumpadButtonWidth->setMaximum(100);

    quickButtonLayout->addWidget( new QLabel(tr("Tasten Größe:")), 0,1,1,1);
//    quickButtonLayout->addWidget( new QLabel(tr("Breite (px)")), 0,2,1,1);
//    quickButtonLayout->addWidget( m_minimumButtonWidth, 1,2,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("Höhe (px)")), 0,3,1,1);
    quickButtonLayout->addWidget( m_fixedButtonHeight, 1,3,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("(Neustart erforderlich)")), 0,4,1,1);

    quickButtonLayout->addWidget( new QLabel(tr("Schnelltasten Größe:")), 2,1,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("Breite (px)")), 2,2,1,1);
    quickButtonLayout->addWidget( m_quickButtonWidth, 3,2,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("Höhe (px)")), 2,3,1,1);
    quickButtonLayout->addWidget( m_quickButtonHeight, 3,3,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("(Dynamisch)")), 2,4,1,1);

    quickButtonLayout->addWidget( new QLabel(tr("Ziffernblocktasten Größe:")), 4,1,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("Breite (px)")), 4,2,1,1);
    quickButtonLayout->addWidget( m_fixedNumpadButtonWidth, 5,2,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("Höhe (px)")), 4,3,1,1);
    quickButtonLayout->addWidget( m_fixedNumpadButtonHeight, 5,3,1,1);
    quickButtonLayout->addWidget( new QLabel(tr("(Neustart erforderlich)")), 4,4,1,1);

    quickButtonGroup->setLayout(quickButtonLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(registerGroup);
    mainLayout->addWidget(quickButtonGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    m_useInputNetPriceCheck->setChecked(settings.value("useInputNetPrice", false).toBool());
    m_useDiscountCheck->setChecked(settings.value("useDiscount", false).toBool());
    m_useMaximumItemSoldCheck->setChecked(settings.value("useMaximumItemSold", false).toBool());
    m_useDecimalQuantityCheck->setChecked(settings.value("useDecimalQuantity", false).toBool());
    m_decimalRoundSpin->setValue(settings.value("decimalDigits", 2).toInt());
    m_useVirtualNumPadCheck->setChecked(settings.value("virtualNumPad", false).toBool());
    m_useInputProductNumberCheck->setChecked(settings.value("useInputProductNumber", false).toBool());
    m_registerHeaderMoveableCheck->setChecked(settings.value("saveRegisterHeaderGeo", false).toBool());

    m_hideCreditcardCheck->setChecked(settings.value("hideCreditcardButton", false).toBool());
    m_hideDebitcardCheck->setChecked(settings.value("hideDebitcardButton", false).toBool());
    m_hideR2BButtonCheck->setChecked(settings.value("hideR2BButton", false).toBool());

    QSize qbuttonsize = settings.value("quickButtonSize", QSize(150, 80)).toSize();
    m_quickButtonHeight->setValue(qbuttonsize.height());
    m_quickButtonWidth->setValue(qbuttonsize.width());

    QSize buttonsize = settings.value("ButtonSize", QSize(150, 60)).toSize();
    m_fixedButtonHeight->setValue(buttonsize.height());
    m_minimumButtonWidth->setValue(buttonsize.width());

    QSize numpadbuttonsize = settings.value("numpadButtonSize", QSize(40, 40)).toSize();
    m_fixedNumpadButtonHeight->setValue(numpadbuttonsize.height());
    m_fixedNumpadButtonWidth->setValue(numpadbuttonsize.width());

    connect(m_useMaximumItemSoldCheck, &QPushButton::clicked, this, &ReceiptMainTab::maximumSoldItemChanged);

    if (!RBAC::Instance()->hasPermission("settings_edit_extra"))
        disableWidgets();

}

QString ReceiptMainTab::getDefaultTax()
{
    int idx  = m_defaultTaxComboBox->currentIndex();
    return m_defaultTaxComboBox->itemText(idx);
}

bool ReceiptMainTab::getInputNetPrice()
{
    return m_useInputNetPriceCheck->isChecked();
}

bool ReceiptMainTab::getDiscount()
{
    return m_useDiscountCheck->isChecked();
}

bool ReceiptMainTab::getMaximumItemSold()
{
    return m_useMaximumItemSoldCheck->isChecked();
}

int ReceiptMainTab::getDecimalDigits()
{
    return m_decimalRoundSpin->value();
}

QSize ReceiptMainTab::getQuickButtonSize()
{
    return QSize(m_quickButtonWidth->value(), m_quickButtonHeight->value());
}

QSize ReceiptMainTab::getButtonSize()
{
    return QSize(m_minimumButtonWidth->value(), m_fixedButtonHeight->value());
}

QSize ReceiptMainTab::getNumpadButtonSize()
{
    return QSize(m_fixedNumpadButtonWidth->value(), m_fixedNumpadButtonHeight->value());
}

bool ReceiptMainTab::getDecimalQuantity()
{
    return m_useDecimalQuantityCheck->isChecked();
}

bool ReceiptMainTab::hideCreditcardButton()
{
    return m_hideCreditcardCheck->isChecked();
}

bool ReceiptMainTab::hideDebitcardButton()
{
    return m_hideDebitcardCheck->isChecked();
}

bool ReceiptMainTab::hideR2BButton()
{
    return m_hideR2BButtonCheck->isChecked();
}

bool ReceiptMainTab::useVirtualNumPad()
{
    return m_useVirtualNumPadCheck->isChecked();
}

bool ReceiptMainTab::useInputProductNumber()
{
    return m_useInputProductNumberCheck->isChecked();
}

bool ReceiptMainTab::getRegisterHeaderMovable()
{
    return m_registerHeaderMoveableCheck->isChecked();
}

ExtraTab::ExtraTab(QWidget *parent)
    : Widget(parent)
{
    QrkSettings settings;

    m_useProductGroupCheck = new QCheckBox;

    m_usePriceChangedCheck = new QCheckBox;

    m_useGivenDialogCheck = new QCheckBox;

    m_salesWidgetCheck = new QCheckBox;

    m_groupSeparatorCheck = new QCheckBox;
    m_groupSeparatorLabel = new QLabel;
    m_groupSeparatorLabel->setText(QLocale().toString(123456789.12, 'f', 2));

    m_firstProductnumber = new QSpinBox;
    m_firstProductnumber->setMinimum(Database::getNextProductNumber().toInt());
    m_firstProductnumber->setMaximum(INT_MAX);

    m_useReceiptPrintedDialogCheck = new QCheckBox;
    m_useReceiptPrintedDialogCheck->setEnabled(settings.value("useReceiptPrintedDialog", true).toBool());
    m_useReceiptPrintedDialogCheck->setToolTip(tr("Diese Option \"Beleg wurde gedruckt\" kann nur verwendet werden wenn\ndie Option \"Meistverkauften Artikel als Standard Artikel verwenden\" nicht aktiviert ist."));

    m_useStockDialogCheck = new QCheckBox;
    m_useStockDialogCheck->setChecked(settings.value("useMinstockDialog", false).toBool());

    /* Fonts */

    m_systemFont = QApplication::font();

    QList<QString> printerFontList = settings.value("printerfont", "Courier-New,10,100").toString().split(",");
    QList<QString> receiptPrinterFontList = settings.value("receiptprinterfont", "Courier-New,8,100").toString().split(",");

    m_printerFont = printerFontList.at(0);
    m_printerFont.setPointSize(printerFontList.at(1).toInt());
    m_printerFont.setStretch(printerFontList.at(2).toInt());

    m_receiptPrinterFont = receiptPrinterFontList.at(0);
    m_receiptPrinterFont.setPointSize(receiptPrinterFontList.at(1).toInt());
    m_receiptPrinterFont.setStretch(receiptPrinterFontList.at(2).toInt());

    QGridLayout *groupLayout = new QGridLayout;
    groupLayout->setAlignment(Qt::AlignLeft);
    groupLayout->setColumnStretch(1,1);
    groupLayout->setColumnStretch(2,1);

    QGroupBox *dialogGroup = new QGroupBox();
    dialogGroup->setTitle(tr("Dialoge"));
    QFormLayout *dialogLayout = new QFormLayout;
    dialogLayout->setAlignment(Qt::AlignLeft);
    dialogLayout->addRow(tr("Artikelpreis wurde geändert:"), m_usePriceChangedCheck);
    dialogLayout->addRow(tr("Betrag gegeben:"),m_useGivenDialogCheck);
    dialogLayout->addRow(tr("Beleg wurde gedruckt:"), m_useReceiptPrintedDialogCheck);
    dialogLayout->addRow(tr("Mindestbestand wurde erreicht:"), m_useStockDialogCheck);

    dialogGroup->setLayout(dialogLayout);

    QGroupBox *widgetGroup = new QGroupBox();
    widgetGroup->setTitle(tr("Widgets"));
    QFormLayout *widgetLayout = new QFormLayout;
    widgetLayout->setAlignment(Qt::AlignLeft);
    widgetLayout->addRow(tr("Umsatz am Startschirm anzeigen:"),m_salesWidgetCheck);

    dialogGroup->setLayout(dialogLayout);
    widgetGroup->setLayout(widgetLayout);

    groupLayout->addWidget(dialogGroup,1,1);
    groupLayout->addWidget(widgetGroup,1,2);

    QGroupBox *productGroup = new QGroupBox();
    productGroup->setTitle(tr("Tages/Monatsabschluss"));
    QFormLayout *productLayout = new QFormLayout;
    productLayout->setAlignment(Qt::AlignLeft);
    productLayout->addRow(tr("Warengruppen verwenden:"),m_useProductGroupCheck);

    productGroup->setLayout(productLayout);

    QGroupBox *numericGroup = new QGroupBox();
    numericGroup->setTitle(tr("Diverses"));
    QGridLayout *numericLayout = new QGridLayout;
    numericLayout->setAlignment(Qt::AlignLeft);
    numericLayout->setColumnStretch(1,1);
    numericLayout->setColumnStretch(2,1);
    numericLayout->addWidget(new QLabel(tr("Zahlen mit Tausenderpunkt darstellen:")),1,1,1,1);
    numericLayout->addWidget(m_groupSeparatorCheck,1,2,1,1);
    numericLayout->addWidget(new QLabel(tr("Beispiel")),1,3,1,1);
    numericLayout->addWidget(m_groupSeparatorLabel,1,4,1,1);
    numericLayout->addWidget(new QLabel(tr("Artikelnummern starten ab:")),2,1,1,1);
    numericLayout->addWidget(m_firstProductnumber,2,2,1,1);
    numericLayout->addWidget(new QLabel(tr("Kleinstmögliche #:")),2,3,1,1);
    numericLayout->addWidget(new QLabel(Database::getNextProductNumber()),2,4,1,1);

    connect(m_groupSeparatorCheck, &QCheckBox::clicked, this, &ExtraTab::groupSeparatorChanged);

    numericGroup->setLayout(numericLayout);

    m_fontsGroup = new QGroupBox();
    m_fontsGroup->setCheckable(true);
    m_fontsGroup->setTitle(tr("Schriftarten"));
    if (settings.value("systemfont").isNull())
        m_fontsGroup->setChecked(false);

    QGridLayout *fontsLayout = new QGridLayout;
    fontsLayout->setAlignment(Qt::AlignLeft);

    fontsLayout->addWidget( new QLabel(tr("Systemschrift:")), 1,1,1,1);
    fontsLayout->addWidget( new QLabel(tr("Druckerschrift:")), 2,1,1,1);
    fontsLayout->addWidget( new QLabel(tr("BON - Druckerschrift:")), 3,1,1,1);

    m_systemFontButton = new QrkPushButton(m_systemFont.family());
    m_systemFontButton->setFont(m_systemFont);
    m_systemFontSizeLabel = new QLabel(QString::number(m_systemFont.pointSize()));
    m_systemFontStretchLabel = new QLabel(QString::number(m_systemFont.stretch()));

    QFontInfo printerFontInfo(m_printerFont);
    QString sPrinterFontInfo = printerFontInfo.family();

    m_printerFont.setFamily(sPrinterFontInfo);
    m_printerFontButton = new QrkPushButton(sPrinterFontInfo);
    m_printerFontButton->setFont(m_printerFont);
    m_printerFontSizeLabel = new QLabel(QString::number(m_printerFont.pointSize()));
    m_printerFontStretchLabel = new QLabel(QString::number(m_printerFont.stretch()));

    QFontInfo receiptPrinterFontInfo(m_receiptPrinterFont);
    QString sReceiptPrinterFontInfo = receiptPrinterFontInfo.family();

    m_receiptPrinterFont.setFamily(sReceiptPrinterFontInfo);
    m_receiptPrinterFontButton = new QrkPushButton(sReceiptPrinterFontInfo);
    m_receiptPrinterFontButton->setFont(m_receiptPrinterFont);
    m_receiptPrinterFontSizeLabel = new QLabel(QString::number(m_receiptPrinterFont.pointSize()));
    m_receiptPrinterFontStretchLabel = new QLabel(QString::number(m_receiptPrinterFont.stretch()));

    QrkPushButton *printerTestButton = new QrkPushButton(tr("Drucktest"));
    QrkPushButton *receiptPrinterTestButton = new QrkPushButton(tr("Drucktest"));

    fontsLayout->addWidget( m_systemFontButton, 1,2,1,1);
    fontsLayout->addWidget( m_printerFontButton, 2,2,1,1);
    fontsLayout->addWidget( m_receiptPrinterFontButton, 3,2,1,1);

    fontsLayout->addWidget( new QLabel("Größe:"), 1,3,1,1);
    fontsLayout->addWidget( new QLabel("Größe:"), 2,3,1,1);
    fontsLayout->addWidget( new QLabel("Größe:"), 3,3,1,1);

    fontsLayout->addWidget( m_systemFontSizeLabel, 1,4,1,1);
    fontsLayout->addWidget( m_printerFontSizeLabel, 2,4,1,1);
    fontsLayout->addWidget( m_receiptPrinterFontSizeLabel, 3,4,1,1);

    fontsLayout->addWidget( new QLabel("Stretch:"), 1,5,1,1);
    fontsLayout->addWidget( new QLabel("Stretch:"), 2,5,1,1);
    fontsLayout->addWidget( new QLabel("Stretch:"), 3,5,1,1);

    fontsLayout->addWidget( m_systemFontStretchLabel, 1,6,1,1);
    fontsLayout->addWidget( m_printerFontStretchLabel, 2,6,1,1);
    fontsLayout->addWidget( m_receiptPrinterFontStretchLabel, 3,6,1,1);

    fontsLayout->addWidget( printerTestButton, 2,7,1,1);
    fontsLayout->addWidget( receiptPrinterTestButton, 3,7,1,1);

    m_fontsGroup->setLayout(fontsLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(groupLayout);
    mainLayout->addWidget(productGroup);
    mainLayout->addWidget(numericGroup);
    mainLayout->addWidget(m_fontsGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    m_useProductGroupCheck->setChecked(settings.value("report_by_productgroup", false).toBool());
    m_usePriceChangedCheck->setChecked(settings.value("usePriceChangedDialog", true).toBool());
    m_useGivenDialogCheck->setChecked(settings.value("useGivenDialog", false).toBool());
    m_salesWidgetCheck->setChecked(settings.value("showSalesWidget", true).toBool());
    m_groupSeparatorCheck->setChecked(settings.value("numericGroupSeparator", true).toBool());
    m_firstProductnumber->setValue(settings.value("firstProductnumber", Database::getNextProductNumber().toInt()).toInt());

    m_useReceiptPrintedDialogCheck->setDisabled(settings.value("useMaximumItemSold", false).toBool());
    if (m_useReceiptPrintedDialogCheck->isEnabled())
        m_useReceiptPrintedDialogCheck->setChecked(settings.value("useReceiptPrintedDialog", true).toBool());
    else
        m_useReceiptPrintedDialogCheck->setChecked(false);

    connect(m_systemFontButton, &QPushButton::clicked, this, &ExtraTab::systemFontButton_clicked);
    connect(m_printerFontButton, &QPushButton::clicked, this, &ExtraTab::printerFontButton_clicked);
    connect(m_receiptPrinterFontButton, &QPushButton::clicked, this, &ExtraTab::receiptPrinterFontButton_clicked);

    connect(printerTestButton, &QPushButton::clicked, this, &ExtraTab::printerTestButton_clicked);
    connect(receiptPrinterTestButton, &QPushButton::clicked, this, &ExtraTab::receiptPrinterTestButton_clicked);

    connect(m_fontsGroup, &QGroupBox::toggled, this, &ExtraTab::fontsGroup_toggled);

    if (!RBAC::Instance()->hasPermission("settings_edit_extra"))
        disableWidgets();

}

int ExtraTab::getFirstProductnumber()
{
    return m_firstProductnumber->value();
}

void ExtraTab::groupSeparatorChanged(bool checked)
{
    if (checked) {
        QLocale l(QLocale().system().language());
        QLocale::setDefault(l);
    } else {
        QLocale l(QLocale().system().language(), QLocale().system().country());
        QLocale::setDefault(l);
    }
    m_groupSeparatorLabel->setText(QLocale().toString(123456789.12, 'f', 2));
}

bool ExtraTab::getGroupSeparator()
{
    return m_groupSeparatorCheck->isChecked();
}

bool ExtraTab::getProductGroup()
{
    return m_useProductGroupCheck->isChecked();
}

void ExtraTab::maximumSoldItemChanged(bool enabled)
{
    QrkSettings settings;
    m_useReceiptPrintedDialogCheck->setDisabled(enabled);
    if (enabled)
        m_useReceiptPrintedDialogCheck->setChecked(!enabled);
    else
        m_useReceiptPrintedDialogCheck->setChecked(settings.value("useReceiptPrintedDialog", true).toBool());
}

void ExtraTab::fontsGroup_toggled(bool toggled)
{
    if (!toggled) {
        QFont font;
        font.setFamily(font.defaultFamily());
        QApplication::setFont(font);
    }
}

bool ExtraTab::isFontsGroup()
{
    return m_fontsGroup->isChecked();
}

QString ExtraTab::getSystemFont()
{
    return QString("%1,%2,%3").arg(m_systemFont.family()).arg(m_systemFont.pointSize()).arg(m_systemFont.stretch());
}

QString ExtraTab::getPrinterFont()
{
    return QString("%1,%2,%3").arg(m_printerFont.family()).arg(m_printerFont.pointSize()).arg(m_printerFont.stretch());
}

QString ExtraTab::getReceiptPrinterFont()
{
    return QString("%1,%2,%3").arg(m_receiptPrinterFont.family()).arg(m_receiptPrinterFont.pointSize()).arg(m_receiptPrinterFont.stretch());
}

void ExtraTab::printerTestButton_clicked(bool)
{
    DocumentPrinter p;
    p.printTestDocument(m_printerFont);
}

void ExtraTab::receiptPrinterTestButton_clicked(bool)
{
    ReceiptItemModel reg;

    int id = Database::getLastReceiptNum();
    reg.setCurrentReceiptNum(id);

    QJsonObject data = reg.compileData();

    data["isTestPrint"] = true;
    data["comment"] = "DRUCKTEST BELEG";
    data["headerText"] = Database::getCustomerText(id);

    DocumentPrinter p;
    p.printReceipt(data);
}

void ExtraTab::systemFontButton_clicked(bool)
{
    FontSelector fontSelect(m_systemFont);
    if ( fontSelect.exec() == FontSelector::Accepted ) {
        m_systemFont = fontSelect.getFont();
        m_systemFontButton->setText(m_systemFont.family());
        m_systemFontSizeLabel->setText(QString::number(m_systemFont.pointSize()));
        m_systemFontStretchLabel->setText(QString::number(m_systemFont.stretch()));
        QApplication::setFont(m_systemFont);
    }
}

void ExtraTab::printerFontButton_clicked(bool)
{
    FontSelector fontSelect(m_printerFont);
    if ( fontSelect.exec() == FontSelector::Accepted ) {
        m_printerFont = fontSelect.getFont();
        m_printerFontButton->setText(m_printerFont.family());
        m_printerFontButton->setFont(m_printerFont);
        m_printerFontSizeLabel->setText(QString::number(m_printerFont.pointSize()));
        m_printerFontStretchLabel->setText(QString::number(m_printerFont.stretch()));
    }
}

void ExtraTab::receiptPrinterFontButton_clicked(bool)
{
    FontSelector fontSelect(m_receiptPrinterFont);
    if ( fontSelect.exec() == FontSelector::Accepted ) {
        m_receiptPrinterFont = fontSelect.getFont();
        m_receiptPrinterFontButton->setText(m_receiptPrinterFont.family());
        m_receiptPrinterFontButton->setFont(m_receiptPrinterFont);
        m_receiptPrinterFontSizeLabel->setText(QString::number(m_receiptPrinterFont.pointSize()));
        m_receiptPrinterFontStretchLabel->setText(QString::number(m_receiptPrinterFont.stretch()));
    }
}

bool ExtraTab::getPriceChangedDialog()
{
    return m_usePriceChangedCheck->isChecked();
}

bool ExtraTab::getGivenDialog()
{
    return m_useGivenDialogCheck->isChecked();
}

bool ExtraTab::getStockDialog()
{
    return m_useStockDialogCheck->isChecked();
}

bool ExtraTab::getSalesWidget()
{
    return m_salesWidgetCheck->isChecked();
}

bool ExtraTab::getReceiptPrintedDialog()
{
    return m_useReceiptPrintedDialogCheck->isChecked();
}

ServerTab::ServerTab(QWidget *parent)
    : Widget(parent)
{
    m_importDirectoryEdit = new QLineEdit();
    m_importDirectoryEdit->setEnabled(false);
    m_codePageCombo = new QComboBox();
    m_codePageCombo->addItem("UTF-8",0);
    m_codePageCombo->addItem("Windows-1252",1);
    m_codePageCombo->addItem("IBM-850",2);
    m_serverFullscreen = new QCheckBox();
    m_serverCriticalMessageBox = new QCheckBox();

    QPushButton *importDirectoryButton = new QPushButton;

    QIcon icon = QIcon(":src/icons/save.png");
    QSize size = QSize(24,24);

    importDirectoryButton->setIcon(icon);
    importDirectoryButton->setIconSize(size);
//    importDirectoryButton->setText(tr("Auswahl"));

    QGroupBox *serverGroup = new QGroupBox;
    QGridLayout *serverLayout = new QGridLayout;
    serverLayout->addWidget(new QLabel(tr("Server Mode Import Verzeichnis:")), 1,1);
    serverLayout->addWidget(new QLabel(tr("Import Zeichensatz:")), 2,1);
    serverLayout->addWidget(new QLabel(tr("Import Info Vollbild:")), 3,1);
    serverLayout->addWidget(new QLabel(tr("Fehlermeldungsdialog:")), 4,1);

    serverLayout->addWidget(m_importDirectoryEdit, 1,2);
    serverLayout->addWidget(m_codePageCombo, 2,2);
    serverLayout->addWidget(m_serverFullscreen, 3,2);
    serverLayout->addWidget(m_serverCriticalMessageBox, 4,2);

    serverLayout->addWidget(importDirectoryButton, 1,3);
    serverGroup->setLayout(serverLayout);

    connect(importDirectoryButton, &QPushButton::clicked, this, &ServerTab::importDirectoryButton_clicked);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(serverGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    QrkSettings settings;
    m_importDirectoryEdit->setText(settings.value("importDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString());
    m_codePageCombo->setCurrentText(settings.value("importCodePage", "UTF-8").toString());
    m_serverFullscreen->setChecked(settings.value("importServerFullscreen", false).toBool());
    m_serverCriticalMessageBox->setChecked(settings.value("serverCriticalMessageBox", true).toBool());

    if (!RBAC::Instance()->hasPermission("settings_edit_extra"))
        disableWidgets();

}

void ServerTab::importDirectoryButton_clicked()
{

    QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis Auswahl"),
                                                     getImportDirectory(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks);

    if (!path.isEmpty())
        m_importDirectoryEdit->setText(path);
}

QString ServerTab::getImportDirectory()
{
    return m_importDirectoryEdit->text();
}

QString ServerTab::getImportCodePage()
{
    int idx  = m_codePageCombo->currentIndex();
    return m_codePageCombo->itemText(idx);
}

bool ServerTab::getServerFullscreen()
{
    return m_serverFullscreen->isChecked();
}

bool ServerTab::getServerCriticalMessageBox()
{
    return m_serverCriticalMessageBox->isChecked();
}

GeneralTab::GeneralTab(QWidget *parent)
    : Widget(parent)
{
    m_dataDirectoryEdit = new QLineEdit();
    m_dataDirectoryEdit->setReadOnly(true);
    m_backupDirectoryEdit = new QLineEdit();
    m_backupDirectoryEdit->setReadOnly(true);
    m_keepMaxBackupSpinBox = new QSpinBox();
    m_keepMaxBackupSpinBox->setMinimum(-1);
    m_pdfDirectoryEdit = new QLineEdit();
    m_pdfDirectoryEdit->setReadOnly(true);
    m_externalDepDirectoryEdit = new QLineEdit();
    m_externalDepDirectoryEdit->setReadOnly(true);

    QPushButton *dataDirectoryButton = new QPushButton;
    QPushButton *backupDirectoryButton = new QPushButton;
    QPushButton *pdfDirectoryButton = new QPushButton;
    QPushButton *externalDepDirectoryButton = new QPushButton;

    QIcon icon = QIcon(":src/icons/save.png");
    QSize size = QSize(24,24);

    dataDirectoryButton->setIcon(icon);
    dataDirectoryButton->setIconSize(size);
//    dataDirectoryButton->setText(tr("Auswahl"));

    backupDirectoryButton->setIcon(icon);
    backupDirectoryButton->setIconSize(size);
//    backupDirectoryButton->setText(tr("Auswahl"));

    pdfDirectoryButton->setIcon(icon);
    pdfDirectoryButton->setIconSize(size);
//    pdfDirectoryButton->setText(tr("Auswahl"));

    externalDepDirectoryButton->setIcon(icon);
    externalDepDirectoryButton->setIconSize(size);
//    externalDepDirectoryButton->setText(tr("Auswahl"));

    QTextBrowser *externalDepInfoLabel =  new QTextBrowser();
    externalDepInfoLabel->setAlignment(Qt::AlignTop);
    externalDepInfoLabel->setText(tr("Das vollständige DEP-7 ist zumindest vierteljährlich auf einem elektronischen, externen "
                                     "Medium unveränderbar zu sichern. Als geeignete Medien gelten beispielsweise "
                                     "schreibgeschützte (abgeschlossene) externe Festplatten, USB-Sticks und Speicher externer "
                                     "Server, die vor unberechtigten Datenzugriffen geschützt sind. Die Unveränderbarkeit des "
                                     "Inhaltes der Daten ist durch die Signatur bzw. das Siegel und insbesondere durch die "
                                     "signierten Monatsbelege gegeben. Jede Sicherung ist gemäß § 132 BAO aufzubewahren."
                                     ));

    QLabel *externalDepDescriptionLabel = new QLabel(tr("DEP-7 Information:"));
    externalDepDescriptionLabel->setAlignment(Qt::AlignTop);
    QGroupBox *pathGroup = new QGroupBox;
    QGridLayout *pathLayout = new QGridLayout;
    pathLayout->addWidget(new QLabel(tr("Daten Verzeichnis:")), 1,1);
    pathLayout->addWidget(new QLabel(tr("Backup Verzeichnis:")), 2,1);
    pathLayout->addWidget(new QLabel(tr("maximale Anzahl von Backups behalten:")), 3,2);
    pathLayout->addWidget(new QLabel(tr("Pdf Verzeichnis:")), 4,1);

    pathLayout->addWidget(m_dataDirectoryEdit, 1,2);
    pathLayout->addWidget(m_backupDirectoryEdit, 2,2);
    pathLayout->addWidget(m_keepMaxBackupSpinBox, 3,3);
    pathLayout->addWidget(m_pdfDirectoryEdit, 4,2);

    pathLayout->addWidget(dataDirectoryButton, 1,3);
    pathLayout->addWidget(backupDirectoryButton, 2,3);
    pathLayout->addWidget(pdfDirectoryButton, 4,3);

    pathGroup->setLayout(pathLayout);

    m_externalDepGroup = new QGroupBox;
    QGridLayout *externalDepLayout = new QGridLayout;
    externalDepLayout->addWidget(externalDepDescriptionLabel, 1,1);
    externalDepLayout->addWidget(externalDepInfoLabel, 1,2,1,2);
    externalDepLayout->addWidget(new QLabel(tr("Externes DEP-7 Backup Verzeichnis:")), 2,1);
    externalDepLayout->addWidget(m_externalDepDirectoryEdit, 2,2);
    externalDepLayout->addWidget(externalDepDirectoryButton, 2,3);
    m_externalDepGroup->setLayout(externalDepLayout);

    connect(backupDirectoryButton, &QPushButton::clicked, this, &GeneralTab::backupDirectoryButton_clicked);
    connect(pdfDirectoryButton, &QPushButton::clicked, this, &GeneralTab::pdfDirectoryButton_clicked);
    connect(dataDirectoryButton, &QPushButton::clicked, this, &GeneralTab::dataDirectoryButton_clicked);
    connect(externalDepDirectoryButton, &QPushButton::clicked, this, &GeneralTab::externalDepDirectoryButton_clicked);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(pathGroup);
    mainLayout->addWidget(m_externalDepGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    QrkSettings settings;
    m_dataDirectoryEdit->setText(settings.value("sqliteDataDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data").toString());
    m_backupDirectoryEdit->setText(settings.value("backupDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString());
    m_keepMaxBackupSpinBox->setValue(settings.value("keepMaxBackups", -1).toInt());
    m_pdfDirectoryEdit->setText(settings.value("pdfDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+ "/pdf").toString());
    m_externalDepDirectoryEdit->setText(settings.value("externalDepDirectory", "").toString());

    masterTaxChanged(Database::getTaxLocation());

    if (!RBAC::Instance()->hasPermission("settings_edit_paths"))
        disableWidgets();

}

void GeneralTab::masterTaxChanged(QString tax)
{
    if (tax == "AT") {
        m_externalDepGroup->setVisible(true);
    } else {
        m_externalDepGroup->setVisible(false);
    }
    QrkSettings settings;
    settings.save2Database("taxlocation", tax);
}

void GeneralTab::backupDirectoryButton_clicked()
{

    QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis Auswahl"),
                                                     getBackupDirectory(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);

    if (!path.isEmpty())
        m_backupDirectoryEdit->setText(path);

}

void GeneralTab::pdfDirectoryButton_clicked()
{

    QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis Auswahl"),
                                                     getPdfDirectory(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);

    if (!path.isEmpty())
        m_pdfDirectoryEdit->setText(path);

}

void GeneralTab::dataDirectoryButton_clicked()
{

    QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis Auswahl"),
                                                     getDataDirectory(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);

    if (!path.isEmpty()) {
        if ( !moveDataFiles( getDataDirectory(), path)) {
            QMessageBox::critical(this,"Änderung des Daten Verzeichnis!",
                                  "Achtung!\n"
                                  "Das Verschieben der Daten ist Fehlgeschlagen.\n"
                                  "Sie müssen die Daten manuell verschieben und danach QRK neu starten."
                                  , "Ok");

        }

        m_dataDirectoryEdit->setText(path);
    }

}

bool GeneralTab::moveDataFiles( QString fromDir, QString toDir)
{

    QDir from(fromDir);
    QDir to(toDir);

    QStringList filter;
    filter << "*.db";
    QStringList fileList = from.entryList(filter, QDir::Files);
    foreach(QString fileName, fileList)
    {
        QFileInfo fi(from, fileName);
        if (fi.isFile())  {
            if (QFile::copy(fi.absoluteFilePath(), to.absoluteFilePath(fi.fileName()))) {
                if (!QFile::remove(fi.absoluteFilePath ())) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    return true;
}

void GeneralTab::externalDepDirectoryButton_clicked()
{

    QString path = QFileDialog::getExistingDirectory(this, tr("Verzeichnis Auswahl"),
                                                     getExternalDepDirectory(),
                                                     QFileDialog::ShowDirsOnly
                                                     | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);

    if (!path.isEmpty())
        m_externalDepDirectoryEdit->setText(path);
}

QString GeneralTab::getBackupDirectory()
{
    return m_backupDirectoryEdit->text();
}

int GeneralTab::getKeepMaxBackups()
{
    return m_keepMaxBackupSpinBox->value();
}

QString GeneralTab::getPdfDirectory()
{
    return m_pdfDirectoryEdit->text();
}

QString GeneralTab::getDataDirectory()
{
    return m_dataDirectoryEdit->text();
}

QString GeneralTab::getExternalDepDirectory()
{
    return m_externalDepDirectoryEdit->text();
}

MasterDataTab::MasterDataTab(QWidget *parent)
    : Widget(parent)
{

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    m_shopName = new QLineEdit;
    m_shopOwner = new QLineEdit;
    m_shopAddress = new TextEdit;
    m_shopAddress->setToolTip("");
    m_shopUid = new QLineEdit;
    m_shopCashRegisterId = new QLineEdit;
    m_taxlocation = new QComboBox;
    m_currency = new QComboBox;

    connect(m_shopCashRegisterId, &QLineEdit::editingFinished, this, &MasterDataTab::cashRegisterIdChanged);

    QRegExp re("^[^_]+$");
    QRegExpValidator *v = new QRegExpValidator(re, this);
    m_shopCashRegisterId->setValidator(v);
    m_shopCashRegisterId->setPlaceholderText(tr("z.B. Firmenname-1, QRK1, oder FN-1 ..."));

    query.prepare("SELECT strValue FROM globals WHERE name='shopName'");
    query.exec();
    if (query.next())
        m_shopName->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('shopName', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='shopOwner'");
    query.exec();
    if (query.next())
        m_shopOwner->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('shopOwner', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='shopAddress'");
    query.exec();
    if (query.next())
        m_shopAddress->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('shopAddress', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='shopUid'");
    query.exec();
    if (query.next())
        m_shopUid->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('shopUid', '')");

    m_currency->addItem("EUR");
    m_currency->addItem("CHF");

    m_taxlocation->addItem("AT");
    m_taxlocation->addItem("DE");
    m_taxlocation->addItem("CH");

    query.prepare("SELECT strValue FROM globals WHERE name='shopCashRegisterId'");
    query.exec();
    if (query.next())
        m_shopCashRegisterId->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('shopCashRegisterId', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='currency'");
    query.exec();
    if (query.next())
        m_currency->setCurrentText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('currency', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='taxlocation'");
    query.exec();
    if (query.next())
        m_taxlocation->setCurrentText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('taxlocation', '')");

    QGroupBox *shopGroup = new QGroupBox(tr("Stammdaten"));
    QFormLayout *shopLayout = new QFormLayout;
    shopLayout->setAlignment(Qt::AlignLeft);
    shopLayout->addWidget(new QLabel());
    shopLayout->addRow(tr("Firmenname:"),m_shopName);
    shopLayout->addRow(tr("Eigentümer:"),m_shopOwner);
    shopLayout->addRow(tr("Adresse:"),m_shopAddress);
    shopLayout->addRow(tr("UID:"),m_shopUid);
    shopLayout->addRow(tr("Kassenidentifikationsnummer:"),m_shopCashRegisterId);
    shopGroup->setLayout(shopLayout);

    QGroupBox *currencyGroup = new QGroupBox(tr("Währung / Steuersatz"));
    QFormLayout *currencyLayout = new QFormLayout;
    m_currency->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);;
    m_taxlocation->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);;
    currencyLayout->addRow(tr("Währung:"), m_currency);
    currencyLayout->addRow(tr("Steuersatz:"), m_taxlocation);
    currencyGroup->setLayout(currencyLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(shopGroup);
    mainLayout->addWidget(currencyGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    if (RKSignatureModule::isDEPactive()) {
        m_shopCashRegisterId->setEnabled(false);
        currencyGroup->setEnabled(false);
    }

    connect(m_taxlocation, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::currentIndexChanged), this, &MasterDataTab::taxChanged);

    if (!RBAC::Instance()->hasPermission("settings_edit_masterdata"))
        disableWidgets();
}

void MasterDataTab::cashRegisterIdChanged()
{
    QrkSettings settings;
    settings.save2Database("shopCashRegisterId", getShopCashRegisterId());
}

QString MasterDataTab::getShopName()
{
    return m_shopName->text();
}

QString MasterDataTab::getShopOwner()
{
    return m_shopOwner->text();
}

QString MasterDataTab::getShopAddress()
{
    return m_shopAddress->toPlainText();
}

QString MasterDataTab::getShopUid()
{
    return m_shopUid->text();
}

QString MasterDataTab::getShopTaxes()
{
    return m_taxlocation->currentText();
}

QString MasterDataTab::getShopCurrency()
{
    return m_currency->currentText();
}

QString MasterDataTab::getShopCashRegisterId()
{
    return m_shopCashRegisterId->text();

}

PrinterTab::PrinterTab(QStringList availablePrinters, QWidget *parent)
    : Widget(parent)
{
    m_reportPrinterCheck = new QCheckBox();
    m_reportPrinterCombo = new QComboBox();
    m_paperFormatCombo = new QComboBox();

    m_invoiceCompanyPrinterCombo = new QComboBox();
    m_invoiceCompanyPaperFormatCombo = new QComboBox();

    m_invoiceCompanyMarginLeftSpin = new QSpinBox();
    m_invoiceCompanyMarginLeftSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_invoiceCompanyMarginRightSpin = new QSpinBox();
    m_invoiceCompanyMarginRightSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_invoiceCompanyMarginTopSpin = new QSpinBox();
    m_invoiceCompanyMarginTopSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_invoiceCompanyMarginBottomSpin = new QSpinBox();
    m_invoiceCompanyMarginBottomSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    QGroupBox *reportPrinterGroup = new QGroupBox(tr("Drucker"));
    QFormLayout *reportPrinterLayout = new QFormLayout();
    reportPrinterLayout->addRow(tr("PDF erstellen:"), m_reportPrinterCheck);
    reportPrinterLayout->addRow(tr("Bericht Drucker:"), m_reportPrinterCombo);
    reportPrinterLayout->addRow(tr("Papierformat:"), m_paperFormatCombo);
    reportPrinterGroup->setLayout(reportPrinterLayout);

    QGroupBox *invoiceCompanyPrinterGroup = new QGroupBox();
    QFormLayout *invoiceCompanyPrinterLayout = new QFormLayout;
    invoiceCompanyPrinterLayout->setAlignment(Qt::AlignLeft);
    invoiceCompanyPrinterLayout->addRow(tr("Firmenrechnung Drucker:"), m_invoiceCompanyPrinterCombo);
    invoiceCompanyPrinterLayout->addRow(tr("Firmenrechnung Papierformat:"), m_invoiceCompanyPaperFormatCombo);

    invoiceCompanyPrinterLayout->addRow(tr("Rand Links [mm]"), m_invoiceCompanyMarginLeftSpin);
    invoiceCompanyPrinterLayout->addRow(tr("Rand Rechts [mm]"), m_invoiceCompanyMarginRightSpin);
    invoiceCompanyPrinterLayout->addRow(tr("Rand Oben [mm]"), m_invoiceCompanyMarginTopSpin);
    invoiceCompanyPrinterLayout->addRow(tr("Rand Unten [mm]"), m_invoiceCompanyMarginBottomSpin);
    invoiceCompanyPrinterGroup->setLayout(invoiceCompanyPrinterLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(reportPrinterGroup);
    mainLayout->addWidget(invoiceCompanyPrinterGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    QrkSettings settings;
    QString reportPrinter = settings.value("reportPrinter").toString();
    QString invoiceCompanyPrinter = settings.value("invoiceCompanyPrinter").toString();

    for (int i = 0; i < availablePrinters.count(); i++)
    {
        m_reportPrinterCombo->addItem(availablePrinters[i]);
        m_invoiceCompanyPrinterCombo->addItem(availablePrinters[i]);

        if ( reportPrinter == availablePrinters[i] )
            m_reportPrinterCombo->setCurrentIndex(i);
        if ( invoiceCompanyPrinter == availablePrinters[i] )
            m_invoiceCompanyPrinterCombo->setCurrentIndex(i);

    }
    m_reportPrinterCheck->setChecked(settings.value("reportPrinterPDF", false).toBool());
    connect(m_reportPrinterCheck, &QCheckBox::toggled,this, &PrinterTab::reportPrinterCheck_toggled);

    m_reportPrinterCombo->setEnabled(!m_reportPrinterCheck->isChecked());

    m_paperFormatCombo->addItem("A4");
    m_paperFormatCombo->addItem("A5");
    m_paperFormatCombo->addItem("POS");
    m_paperFormatCombo->setCurrentText(settings.value("paperFormat", "A4").toString());

    m_invoiceCompanyPaperFormatCombo->addItem("A4");
    m_invoiceCompanyPaperFormatCombo->addItem("A5");
    m_invoiceCompanyPaperFormatCombo->setCurrentText(settings.value("invoiceCompanyPaperFormat", "A4").toString());

    m_invoiceCompanyMarginLeftSpin->setValue(settings.value("invoiceCompanyMarginLeft", 90).toInt());
    m_invoiceCompanyMarginTopSpin->setValue(settings.value("invoiceCompanyMarginTop", 50).toInt());
    m_invoiceCompanyMarginRightSpin->setValue(settings.value("invoiceCompanyMarginRight", 5).toInt());
    m_invoiceCompanyMarginBottomSpin->setValue(settings.value("invoiceCompanyMarginBottom", 0).toInt());

    if (!RBAC::Instance()->hasPermission("settings_edit_printer"))
        disableWidgets();

}

void PrinterTab::reportPrinterCheck_toggled(bool checkState)
{
    m_reportPrinterCombo->setEnabled(!checkState);
}

bool PrinterTab::getReportPrinterPDF()
{
    return m_reportPrinterCheck->isChecked();
}

QString PrinterTab::getReportPrinter()
{
    return m_reportPrinterCombo->currentText();
}

QString PrinterTab::getPaperFormat()
{
    return m_paperFormatCombo->currentText();
}

QString PrinterTab::getInvoiceCompanyPrinter()
{
    return m_invoiceCompanyPrinterCombo->currentText();
}

QString PrinterTab::getInvoiceCompanyPaperFormat()
{
    return m_invoiceCompanyPaperFormatCombo->currentText();
}

int PrinterTab::getInvoiceCompanyMarginLeft()
{
    return m_invoiceCompanyMarginLeftSpin->value();
}

int PrinterTab::getInvoiceCompanyMarginRight()
{
    return m_invoiceCompanyMarginRightSpin->value();
}

int PrinterTab::getInvoiceCompanyMarginTop()
{
    return m_invoiceCompanyMarginTopSpin->value();
}

int PrinterTab::getInvoiceCompanyMarginBottom()
{
    return m_invoiceCompanyMarginBottomSpin->value();
}

ReceiptPrinterTab::ReceiptPrinterTab(QStringList availablePrinters, QWidget *parent)
    : Widget(parent)
{
    m_receiptPrinterCombo = new QComboBox();
    m_useReportPrinterCheck = new QCheckBox();

    m_numberCopiesSpin = new QSpinBox();
    m_numberCopiesSpin->setMinimum(1);
    m_numberCopiesSpin->setMaximum(2);
    m_numberCopiesSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_paperWidthSpin = new QSpinBox();
    m_paperWidthSpin->setMaximum(210);
    m_paperWidthSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_paperHeightSpin = new QSpinBox();
    m_paperHeightSpin->setMaximum(4000);
    m_paperHeightSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_marginLeftSpin = new QSpinBox();
    m_marginLeftSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_marginRightSpin = new QSpinBox();
    m_marginRightSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_marginTopSpin = new QSpinBox();
    m_marginTopSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_marginBottomSpin = new QSpinBox();
    m_marginBottomSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedProduktSpin = new QSpinBox();
    m_feedProduktSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedCompanyHeaderSpin = new QSpinBox();
    m_feedCompanyHeaderSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedCompanyAddressSpin = new QSpinBox();
    m_feedCompanyAddressSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedCashRegisteridSpin = new QSpinBox();
    m_feedCashRegisteridSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedTimestampSpin = new QSpinBox();
    m_feedTimestampSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedTaxSpin = new QSpinBox();
    m_feedTaxSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedPrintHeaderSpin = new QSpinBox();
    m_feedPrintHeaderSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedHeaderTextSpin = new QSpinBox();
    m_feedHeaderTextSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    m_feedQRCodeSpin = new QSpinBox();
    m_feedQRCodeSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);

    QGroupBox *receiptPrinterGroup = new QGroupBox(tr("BON Drucker"));
    QGridLayout *receiptPrinterLayout = new QGridLayout;
    receiptPrinterLayout->setAlignment(Qt::AlignLeft);

    receiptPrinterLayout->addWidget( new QLabel(tr("Drucker:")), 1,1,1,2);
    receiptPrinterLayout->addWidget( m_receiptPrinterCombo, 1,3,1,2);

    receiptPrinterLayout->addWidget( new QLabel(tr("Berichtdrucker für den zweiten Ausdruck verwenden:")), 2,1,1,2);
    receiptPrinterLayout->addWidget( m_useReportPrinterCheck, 2,3,1,2);

    QGroupBox *receiptPrinterGroup2 = new QGroupBox();
    QGridLayout *receiptPrinterLayout2 = new QGridLayout;

    receiptPrinterLayout2->addWidget( new QLabel(tr("Anzahl Kopien")), 2,1,1,1);
    receiptPrinterLayout2->addWidget( m_numberCopiesSpin, 2,2,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Papier Breite [mm]")), 3,1,1,1);
    receiptPrinterLayout2->addWidget( m_paperWidthSpin, 3,2,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Papier Höhe [mm]")), 4,1,1,1);
    receiptPrinterLayout2->addWidget( m_paperHeightSpin, 4,2,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Rand Links [mm]")), 5,1,1,1);
    receiptPrinterLayout2->addWidget( m_marginLeftSpin, 5,2,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Rand Rechts [mm]")), 6,1,1,1);
    receiptPrinterLayout2->addWidget( m_marginRightSpin, 6,2,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Rand Oben [mm]")), 7,1,1,1);
    receiptPrinterLayout2->addWidget( m_marginTopSpin, 7,2,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Rand Unten [mm]")), 8,1,1,1);
    receiptPrinterLayout2->addWidget( m_marginBottomSpin, 8,2,1,1);

    QLabel *infoTextLabel = new QLabel(tr("Zeilenabstand in Pixel nach ..."));
    infoTextLabel->setStyleSheet("font-weight: bold");
    receiptPrinterLayout2->addWidget( infoTextLabel, 1,3,1,2);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Firmenname [px]")), 2,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedCompanyHeaderSpin, 2,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Adresszeile [px]")), 3,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedCompanyAddressSpin, 3,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Kassenidzeile [px]")), 4,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedCashRegisteridSpin, 4,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Zeitzeile [px]")), 5,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedTimestampSpin, 5,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Artikelzeile [px]")), 6,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedProduktSpin, 6,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Mwst. Zeile [px]")), 7,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedTaxSpin, 7,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("BON Kopfzeile [px]")), 8,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedPrintHeaderSpin, 8,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("Kunden Zusatztext [px]")), 9,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedHeaderTextSpin, 9,4,1,1);

    receiptPrinterLayout2->addWidget( new QLabel(tr("QR Code [px]")), 10,3,1,1);
    receiptPrinterLayout2->addWidget( m_feedQRCodeSpin, 10,4,1,1);

    receiptPrinterGroup->setLayout(receiptPrinterLayout);
    receiptPrinterGroup2->setLayout(receiptPrinterLayout2);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(receiptPrinterGroup);
    mainLayout->addWidget(receiptPrinterGroup2);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    QrkSettings settings;
    QString receiptPrinter = settings.value("receiptPrinter").toString();
    for (int i = 0; i < availablePrinters.count(); i++)
    {
        m_receiptPrinterCombo->addItem(availablePrinters[i]);
        if ( receiptPrinter == availablePrinters[i] )
            m_receiptPrinterCombo->setCurrentIndex(i);
    }

    m_useReportPrinterCheck->setChecked(settings.value("useReportPrinter", true).toBool());
    m_numberCopiesSpin->setValue(settings.value("numberCopies", 1).toInt());
    m_paperWidthSpin->setValue(settings.value("paperWidth", 80).toInt());
    m_paperHeightSpin->setValue(settings.value("paperHeight", 3000).toInt());

    m_marginLeftSpin->setValue(settings.value("marginLeft", 0).toInt());
    m_marginTopSpin->setValue(settings.value("marginTop", 17).toInt());
    m_marginRightSpin->setValue(settings.value("marginRight", 5).toInt());
    m_marginBottomSpin->setValue(settings.value("marginBottom", 0).toInt());

    m_feedProduktSpin->setValue(settings.value("feedProdukt", 5).toInt());
    m_feedCompanyHeaderSpin->setValue(settings.value("feedCompanyHeader", 5).toInt());
    m_feedCompanyAddressSpin->setValue(settings.value("feedCompanyAddress", 5).toInt());
    m_feedCashRegisteridSpin->setValue(settings.value("feedCashRegisterid", 5).toInt());
    m_feedTimestampSpin->setValue(settings.value("feedTimestamp", 5).toInt());
    m_feedTaxSpin->setValue(settings.value("feedTaxSpin", 5).toInt());
    m_feedPrintHeaderSpin->setValue(settings.value("feedPrintHeader", 5).toInt());
    m_feedHeaderTextSpin->setValue(settings.value("feedHeaderText", 5).toInt());
    m_feedQRCodeSpin->setValue(settings.value("feedQRCode", 20).toInt());

    if (!RBAC::Instance()->hasPermission("settings_edit_receipt_printer"))
        disableWidgets();

}

QString ReceiptPrinterTab::getReceiptPrinter()
{
    return m_receiptPrinterCombo->currentText();
}

bool ReceiptPrinterTab::getUseReportPrinter()
{
    return m_useReportPrinterCheck->isChecked();
}

int ReceiptPrinterTab::getNumberCopies()
{
    return m_numberCopiesSpin->value();
}

int ReceiptPrinterTab::getpaperWidth()
{
    return m_paperWidthSpin->value();
}

int ReceiptPrinterTab::getpaperHeight()
{
    return m_paperHeightSpin->value();
}

int ReceiptPrinterTab::getmarginLeft()
{
    return m_marginLeftSpin->value();
}

int ReceiptPrinterTab::getmarginRight()
{
    return m_marginRightSpin->value();
}

int ReceiptPrinterTab::getmarginTop()
{
    return m_marginTopSpin->value();
}

int ReceiptPrinterTab::getmarginBottom()
{
    return m_marginBottomSpin->value();
}

int ReceiptPrinterTab::getfeedProdukt()
{
    return m_feedProduktSpin->value();
}

int ReceiptPrinterTab::getfeedCompanyHeader()
{
    return m_feedCompanyHeaderSpin->value();
}

int ReceiptPrinterTab::getfeedCompanyAddress()
{
    return m_feedCompanyAddressSpin->value();
}

int ReceiptPrinterTab::getfeedCashRegisterid()
{
    return m_feedCashRegisteridSpin->value();
}

int ReceiptPrinterTab::getfeedTimestamp()
{
    return m_feedTimestampSpin->value();
}

int ReceiptPrinterTab::getfeedTax()
{
    return m_feedTaxSpin->value();
}

int ReceiptPrinterTab::getfeedPrintHeader()
{
    return m_feedPrintHeaderSpin->value();
}

int ReceiptPrinterTab::getfeedHeaderText()
{
    return m_feedHeaderTextSpin->value();
}

int ReceiptPrinterTab::getfeedQRCode()
{
    return m_feedQRCodeSpin->value();
}

ReceiptTab::ReceiptTab(QStringList availablePrinters, QWidget *parent)
    : Widget(parent)
{

    m_receiptPrinterHeading = new QComboBox();
    m_printCollectionReceiptCheck = new QCheckBox();
    m_collectionReceiptTextEdit = new QLineEdit();
    m_collectionReceiptTextEdit->setPlaceholderText(tr("Abholbon für"));

    m_collectionReceiptCopiesSpin = new QSpinBox();
    m_collectionReceiptCopiesSpin->setMinimum(1);

    m_printCompanyNameBoldCheck = new QCheckBox();
    m_printQRCodeCheck = new QCheckBox();
    m_printQRCodeLeftCheck = new QCheckBox();

    m_useLogo = new QCheckBox();
    m_useLogoRightCheck = new QCheckBox();
    m_logoEdit = new QLineEdit();
    m_logoEdit->setReadOnly(true);
    m_logoButton = new QPushButton;

    QIcon icon = QIcon(":src/icons/save.png");
    QSize size = QSize(24,24);
    m_logoButton->setIcon(icon);
    m_logoButton->setIconSize(size);
//    m_logoButton->setText(tr("Auswahl"));

    QHBoxLayout *logoLayout = new QHBoxLayout;
    logoLayout->addWidget(m_useLogo);
    logoLayout->addWidget(m_logoEdit);
    logoLayout->addWidget(m_logoButton);

    QHBoxLayout *collectionBonLayout = new QHBoxLayout;
    collectionBonLayout->addWidget(m_printCollectionReceiptCheck);
    collectionBonLayout->addWidget(new QLabel(tr("Anzahl Kopien:")));
    collectionBonLayout->addWidget(m_collectionReceiptCopiesSpin);

    m_collectionPrinterCombo = new QComboBox;
    QrkSettings settings;
    QString collectionPrinter = settings.value("collectionPrinter").toString();
    for (int i = 0; i < availablePrinters.count(); i++)
    {
        m_collectionPrinterCombo->addItem(availablePrinters[i]);
        if ( collectionPrinter == availablePrinters[i] )
            m_collectionPrinterCombo->setCurrentIndex(i);
    }

    m_collectionPrinterPaperFormatCombo = new QComboBox;
    m_collectionPrinterPaperFormatCombo->addItem("A4");
    m_collectionPrinterPaperFormatCombo->addItem("A5");
    m_collectionPrinterPaperFormatCombo->addItem("POS");
    m_collectionPrinterPaperFormatCombo->setCurrentText(settings.value("collectionPrinterPaperFormat", "POS").toString());

    QGroupBox *receiptGroup = new QGroupBox(tr("Kassa BON"));
    QFormLayout *receiptLayout = new QFormLayout;
    receiptLayout->setAlignment(Qt::AlignLeft);
    receiptLayout->addRow(tr("Überschrift:"), m_receiptPrinterHeading);
    receiptLayout->addRow(tr("Firmenname Fett drucken:"), m_printCompanyNameBoldCheck);
    receiptLayout->addRow(tr("QRCode drucken:"), m_printQRCodeCheck);
    receiptLayout->addRow(tr("QRCode auf der linken Seite drucken:"), m_printQRCodeLeftCheck);

    receiptLayout->addRow(tr("Logo verwenden:"), logoLayout);
    receiptLayout->addRow(tr("Logo auf der rechten Seite drucken:"), m_useLogoRightCheck);
    receiptGroup->setLayout(receiptLayout);

    QGroupBox *collectionGroup = new QGroupBox(tr("Abhol BON"));
    QFormLayout *collectionLayout = new QFormLayout;
    collectionLayout->setAlignment(Qt::AlignLeft);
    collectionLayout->addRow(tr("Extrabon pro Artikel drucken:"), collectionBonLayout);
    collectionLayout->addRow(tr("Extrabon Text:"), m_collectionReceiptTextEdit);
    collectionLayout->addRow(tr("Extrabon Drucker:"), m_collectionPrinterCombo);
    collectionLayout->addRow(tr("Papierformat:"), m_collectionPrinterPaperFormatCombo);

    collectionGroup->setLayout(collectionLayout);


    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(receiptGroup);
    mainLayout->addWidget(collectionGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    m_receiptPrinterHeading->addItem("KASSABON");
    m_receiptPrinterHeading->addItem("KASSENBON");
    m_receiptPrinterHeading->addItem("Zahlungsbestätigung");

    m_receiptPrinterHeading->setCurrentText(settings.value("receiptPrinterHeading", "KASSABON").toString());

    m_printCompanyNameBoldCheck->setChecked(settings.value("printCompanyNameBold", true).toBool());

    m_useLogo->setChecked(settings.value("useLogo", false).toBool());
    m_logoEdit->setText(settings.value("logo", "./logo.png").toString());
    m_useLogoRightCheck->setChecked(settings.value("logoRight", false).toBool());

    m_printCollectionReceiptCheck->setChecked(settings.value("printCollectionReceipt", false).toBool());
    QString collectionReceiptText = settings.value("collectionReceiptText", tr("Abholbon für")).toString();
    m_collectionReceiptTextEdit->setText(collectionReceiptText);

    m_collectionReceiptCopiesSpin->setValue(settings.value("collectionReceiptCopies", 1).toInt());

    m_printQRCodeCheck->setChecked(settings.value("qrcode", true).toBool());
    m_printQRCodeLeftCheck->setChecked(settings.value("qrcodeleft", false).toBool());

    connect(m_logoButton, &QPushButton::clicked, this, &ReceiptTab::logoButton_clicked);
    connect(m_useLogo, &QPushButton::clicked, this, &ReceiptTab::useLogoCheck_toggled);
    connect(m_printQRCodeCheck, &QPushButton::clicked, m_printQRCodeLeftCheck, &QCheckBox::setEnabled);

    if (!RBAC::Instance()->hasPermission("settings_edit_receipt"))
        disableWidgets();

}

QString ReceiptTab::getLogo()
{
    return m_logoEdit->text();
}

void ReceiptTab::useLogoCheck_toggled(bool toggled)
{
    m_logoButton->setEnabled(toggled);
}

bool ReceiptTab::getUseLogo()
{
    return m_useLogo->isChecked();
}

void ReceiptTab::logoButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Logo Auswahl"), getLogo(), tr("Image Files (*.png *.jpg *.bmp)"));

    if (!fileName.isEmpty())
        m_logoEdit->setText(fileName);

}

bool ReceiptTab::getPrintCompanyNameBold()
{
    return m_printCompanyNameBoldCheck->isChecked();
}

QString ReceiptTab::getReceiptPrinterHeading()
{
    return m_receiptPrinterHeading->currentText();
}

QString ReceiptTab::getCollectionPrinter()
{
    return m_collectionPrinterCombo->currentText();
}

QString ReceiptTab::getCollectionPrinterPaperFormat()
{
    return m_collectionPrinterPaperFormatCombo->currentText();
}

bool ReceiptTab::getPrintCollectionReceipt()
{
    return m_printCollectionReceiptCheck->isChecked();
}

int ReceiptTab::getCollectionReceiptCopies()
{
    return m_collectionReceiptCopiesSpin->value();
}


QString ReceiptTab::getCollectionReceiptText()
{
    return m_collectionReceiptTextEdit->text();
}

bool ReceiptTab::getIsLogoRight()
{
    return m_useLogoRightCheck->isChecked();
}

bool ReceiptTab::getPrintQRCode()
{
    return m_printQRCodeCheck->isChecked();
}

bool ReceiptTab::getPrintQRCodeLeft()
{
    return m_printQRCodeLeftCheck->isChecked();
}

ReceiptEnhancedTab::ReceiptEnhancedTab(QWidget *parent)
    : Widget(parent)
{
    m_printAdvertisingEdit = new TextEdit();
    m_printHeaderEdit = new TextEdit();
    m_printFooterEdit = new TextEdit();

    completer = new QCompleter(this);
    completer->setModel(modelFromFile(":src/txt/templatecompleter.txt"));
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    m_printAdvertisingEdit->setCompleter(completer);
    m_printHeaderEdit->setCompleter(completer);
    m_printFooterEdit->setCompleter(completer);

    m_useAdvertising = new QCheckBox();
    m_advertisingEdit = new QLineEdit();
    m_advertisingEdit->setReadOnly(true);
    m_advertisingButton = new QPushButton;

    QIcon icon = QIcon(":src/icons/save.png");
    QSize size = QSize(24,24);
    m_advertisingButton->setIcon(icon);
    m_advertisingButton->setIconSize(size);
//    m_advertisingButton->setText(tr("Auswahl"));

    QHBoxLayout *advertisingLayout = new QHBoxLayout;
    advertisingLayout->addWidget(m_useAdvertising);
    advertisingLayout->addWidget(m_advertisingEdit);
    advertisingLayout->addWidget(m_advertisingButton);

    QGroupBox *receiptGroup = new QGroupBox(tr("Kassa BON Text"));
    QFormLayout *receiptLayout = new QFormLayout;
    receiptLayout->setAlignment(Qt::AlignLeft);

    receiptLayout->addRow(tr("KassaBon Werbung Grafik:"), advertisingLayout);

    receiptLayout->addRow(tr("KassaBon Werbung Text:"), m_printAdvertisingEdit);
    receiptLayout->addRow(tr("BON Kopfzeile:"), m_printHeaderEdit);
    receiptLayout->addRow(tr("BON Fußzeile:"), m_printFooterEdit);

    receiptGroup->setLayout(receiptLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(receiptGroup);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare("SELECT strValue FROM globals WHERE name='printAdvertisingText'");
    query.exec();
    if (query.next())
        m_printAdvertisingEdit->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('printAdvertisingText', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='printHeader'");
    query.exec();
    if (query.next())
        m_printHeaderEdit->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('printHeader', '')");

    query.prepare("SELECT strValue FROM globals WHERE name='printFooter'");
    query.exec();
    if ( query.next() )
        m_printFooterEdit->setText(query.value(0).toString());
    else
        query.exec("INSERT INTO globals (name, strValue) VALUES('printFooter', '')");

    QrkSettings settings;
    m_useAdvertising->setChecked(settings.value("useAdvertising", false).toBool());
    m_advertisingEdit->setText(settings.value("advertising", "./advertising.png").toString());

    connect(m_advertisingButton, &QPushButton::clicked, this, &ReceiptEnhancedTab::advertisingButton_clicked);
    connect(m_useAdvertising, &QCheckBox::toggled, this, &ReceiptEnhancedTab::useAdvertisingCheck_toggled);

    if (!RBAC::Instance()->hasPermission("settings_edit_receipt_text"))
        disableWidgets();

}

QAbstractItemModel *ReceiptEnhancedTab::modelFromFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return new QStringListModel(completer);

    QStringList words;

    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        if (!line.isEmpty())
            words << line.trimmed();
    }

    std::sort(words.begin(), words.end(), std::greater<QString>());

    return new QStringListModel(words, completer);
}

QString ReceiptEnhancedTab::getAdvertisingText()
{
    return m_printAdvertisingEdit->toPlainText();
}

QString ReceiptEnhancedTab::getHeader()
{
    return m_printHeaderEdit->toPlainText();
}

QString ReceiptEnhancedTab::getFooter()
{
    return m_printFooterEdit->toPlainText();
}

QString ReceiptEnhancedTab::getAdvertising()
{
    return m_advertisingEdit->text();
}

void ReceiptEnhancedTab::useAdvertisingCheck_toggled(bool toggled)
{
    m_advertisingButton->setEnabled(toggled);
}

bool ReceiptEnhancedTab::getUseAdvertising()
{
    return m_useAdvertising->isChecked();
}

void ReceiptEnhancedTab::advertisingButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Werbung Auswahl"), getAdvertising(), tr("Image Files (*.png *.jpg *.bmp)"));

    if (!fileName.isEmpty())
        m_advertisingEdit->setText(fileName);

}

SCardReaderTab::SCardReaderTab(QWidget *parent)
    : Widget(parent)
{

    QrkSettings settings;
    m_scardReaderComboBox = new QComboBox();
    m_providerComboBox = new QComboBox();
    m_providerLoginEdit = new QLineEdit();
    m_providerPasswordEdit = new QLineEdit();

    QStringList scardReaders;
    RKSmartCardInfo::getReaders(&scardReaders);
    int readerCount = scardReaders.count();

    QString currentCardReader = settings.value("currentCardReader").toString();
    for (int i = 0; i < readerCount; i++) {
        m_scardReaderComboBox->addItem(scardReaders.at(i));
        if (currentCardReader == scardReaders.at(i))
            m_scardReaderComboBox->setCurrentText(currentCardReader);
    }

    m_providerComboBox->addItem("A-Trust");
    QString currentOnlineReader = settings.value("atrust_connection").toString();
    bool onlineReader = false;
    if (currentOnlineReader.split("@").size() == 3) {
        m_providerLoginEdit->setText(currentOnlineReader.split("@")[0]);
        m_providerPasswordEdit->setText(currentOnlineReader.split("@")[1]);
        onlineReader = currentCardReader.isEmpty();
    }

    /*----------------------------------------------------------------------*/
    m_scardGroup = new QGroupBox(tr("Kartenleser"));
    m_scardGroup->setCheckable(true);

    QFormLayout *scardLayout = new QFormLayout;
    scardLayout->setAlignment(Qt::AlignLeft);
    scardLayout->addRow(tr("Kartenleser:"), m_scardReaderComboBox);
    m_scardGroup->setLayout(scardLayout);

    /*----------------------------------------------------------------------*/

    m_onlineGroup = new QGroupBox(tr("Onlinedienst"));
    m_onlineGroup->setCheckable(true);
    m_onlineGroup->setChecked(onlineReader && currentCardReader.isEmpty());
    m_scardGroup->setChecked(!currentCardReader.isEmpty());

    QFormLayout *onlineLayout = new QFormLayout;
    QHBoxLayout *loginLayout = new QHBoxLayout;
    loginLayout->addWidget(m_providerLoginEdit);
    loginLayout->addWidget(new QLabel((tr("Kennwort:"))));
    loginLayout->addWidget(m_providerPasswordEdit);

    onlineLayout->setAlignment(Qt::AlignLeft);
    onlineLayout->addRow(tr("Anbieter:"), m_providerComboBox);
    onlineLayout->addRow(tr("Login:"), loginLayout);
    m_onlineGroup->setLayout(onlineLayout);

    /*----------------------------------------------------------------------*/

    QLabel *infoLabel = new QLabel();
    infoLabel->setWordWrap(true);

    QrkPushButton *scardTestButton = new QrkPushButton(tr("Test"));
    m_scardActivateButton = new QrkPushButton(tr("DEP-7 Aktivieren"));
    if (RKSignatureModule::isDEPactive()) {
        m_scardActivateButton->setText(tr("Kasse außer\nBetrieb nehmen"));
        //        m_scardActivateButton->setVisible(false);
        infoLabel->setText(tr("DEP-7 (RKSV Daten Erfassungs Protokoll) ist aktiv. Um den Kartenleser, die Karte oder die Online Daten zu ändern führen Sie den Test durch. Es wird auf eine gültige Signaturkarte bzw. auf die richtigen Onlinedaten überprüft.\nACHTUNG beim verlassen werden die neuen Daten gespeichert. Führen Sie hier nur Änderungen durch wenn Sie genau wissen was Sie tun. Bei Problemen hilft Ihnen die Community im Forum weiter."));
    } else {
        m_scardActivateButton->setEnabled(false);
        infoLabel->setText(tr("Um das DEP-7 (RKSV Daten Erfassungs Protokoll) zu aktivieren führen Sie zuerst den Test durch. Es wird auf eine gültige Signaturkarte bzw. auf die richtigen Onlinedaten überprüft."));
    }

    scardTestButton->setFixedWidth(150);
    scardTestButton->setFixedHeight(60);

    m_scardActivateButton->setFixedWidth(150);
    m_scardActivateButton->setFixedHeight(60);

    m_infoWidget = new QListWidget();

    QHBoxLayout *testHLayout = new QHBoxLayout();
    QVBoxLayout *testLayout = new QVBoxLayout();

    testHLayout->setAlignment(Qt::AlignTop);
    testHLayout->addWidget(scardTestButton);
    testHLayout->addWidget(m_scardActivateButton);

    testLayout->setAlignment(Qt::AlignTop);
    testLayout->addWidget(infoLabel);
    testLayout->addLayout(testHLayout);
    testLayout->addWidget(m_infoWidget);

//    testGroup->setLayout(testLayout);

    /*----------------------------------------------------------------------*/

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_scardGroup);
    mainLayout->addWidget(m_onlineGroup);
    mainLayout->addLayout(testLayout);

    mainLayout->addStretch(1);
    setLayout(mainLayout);

    /*----------------------------------------------------------------------*/
    connect(m_scardGroup, &QGroupBox::clicked, this, &SCardReaderTab::scardGroup_clicked);
    connect(m_onlineGroup, &QGroupBox::clicked, this, &SCardReaderTab::onlineGroup_clicked);
    connect(scardTestButton, &QPushButton::clicked, this, &SCardReaderTab::testButton_clicked);
    connect(m_scardActivateButton, &QPushButton::clicked, this, &SCardReaderTab::activateButton_clicked);
    if (readerCount == 0) {
        m_scardReaderComboBox->addItem(tr("Kein Kartenleser vorhanden"));
        m_scardReaderComboBox->setEnabled(false);
        emit scardGroup_clicked(false);
    }

    if (!RBAC::Instance()->hasPermission("settings_edit_see"))
        disableWidgets();

}

bool SCardReaderTab::saveSettings()
{
    QrkSettings settings;
    bool ret = false;
    QString currentCardReader = getCurrentCardReader();
    if (currentCardReader.isEmpty()) {
        settings.removeSettings("currentCardReader");
    } else {
        if (!m_onlineGroup->isChecked()) {
            settings.save2Settings("currentCardReader", currentCardReader);
            ret = true;
        }
    }

    QString connectionString = getCurrentOnlineConnetionString();
    if (connectionString.split("@").size() == 3) {
        settings.save2Settings("atrust_connection", connectionString);
        ret = true;
    } else {
        settings.removeSettings("atrust_connection");
    }

    return ret;
}

QString SCardReaderTab::getCurrentCardReader()
{
    if (m_scardReaderComboBox->isEnabled())
        return m_scardReaderComboBox->currentText();

    return QString();
}

QString SCardReaderTab::getCurrentOnlineConnetionString()
{
    QString connectionstring;
    if (!m_providerLoginEdit->text().isEmpty()) {
        connectionstring.append(m_providerLoginEdit->text());
        connectionstring.append("@");
    }
    if (!m_providerPasswordEdit->text().isEmpty()) {
        connectionstring.append(m_providerPasswordEdit->text());
        connectionstring.append("@");
    }
    connectionstring.append("https://www.a-trust.at/asignrkonline/v2");
    if (connectionstring.split("@").size() == 3)
        return connectionstring;

    return "";
}

void SCardReaderTab::scardGroup_clicked(bool b)
{
    m_scardGroup->setChecked(b);
    m_onlineGroup->setChecked(!b);
}

void SCardReaderTab::onlineGroup_clicked(bool b)
{
    m_onlineGroup->setChecked(b);
    m_scardGroup->setChecked(!b);
}

void SCardReaderTab::testButton_clicked(bool)
{

    QRKProgress waitbar;
    waitbar.setText(tr("Signaturerstellungseinheit wird geprüft."));
    waitbar.setWaitMode(true);
    waitbar.show();
    m_infoWidget->clear();
    if (m_onlineGroup->isChecked())
        m_rkSignature = RKSignatureModuleFactory::createInstance(getCurrentOnlineConnetionString(), DemoMode::isDemoMode());
    else
        m_rkSignature = RKSignatureModuleFactory::createInstance(m_scardReaderComboBox->currentText(), DemoMode::isDemoMode());

    if (m_rkSignature->selectApplication()) {
        m_scardActivateButton->setEnabled(true);
        m_infoWidget->addItem(tr("Signatureinheit: %1").arg(m_rkSignature->getCardType()));
        m_infoWidget->addItem(tr("Zertifikat Seriennummer: hex %1, dec %2").arg(m_rkSignature->getCertificateSerial(true)).arg(m_rkSignature->getCertificateSerial(false)));
    } else {
        m_scardActivateButton->setEnabled(false);
        m_infoWidget->addItem(tr("Kein Signatur Module gefunden."));
    }

    delete m_rkSignature;
}

void SCardReaderTab::activateButton_clicked(bool)
{
    if (!saveSettings() && !DemoMode::isDemoMode() && !RKSignatureModule::isDEPactive()) {
        QMessageBox messageBox(QMessageBox::Information,
                               QObject::tr("DEP-7 aktivieren"),
                               QObject::tr("Keine gültige Signaturerstellungseinheit gefunden."),
                               QMessageBox::Yes | QMessageBox::No,
                               Q_NULLPTR);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Ja"));
        messageBox.setButtonText(QMessageBox::No, QObject::tr("Nein"));
        messageBox.exec();
    } else {
        if (!RKSignatureModule::isDEPactive()) {
            QString cashRegisterId = Database::getCashRegisterId();
            if (cashRegisterId.isEmpty()) {
                QMessageBox messageBox(QMessageBox::Information,
                                       QObject::tr("Kassenidentifikationsnummer"),
                                       QObject::tr("Keine gültige Kassenidentifikationsnummer."),
                                       QMessageBox::Yes,
                                       Q_NULLPTR);
                messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Ok"));
                messageBox.exec();
                return;
            }
            QMessageBox messageBox(QMessageBox::Question,
                                   QObject::tr("DEP-7 aktivieren"),
                                   QObject::tr("Achtung!\nWenn das DEP-7 (RKSV Daten Erfassungs Prodokoll) aktiviert wird beachten sie Bitte folgende Punkte.\n1. Es werden alle Rechnungen elektronisch signiert und im DEP gespeichert.\n2. Eine De-aktivierung ist nur möglich wenn die Kasse außer Betrieb genommen wird.\n3. Sie müssen den erstellten Startbeleg sicher aufbewahren.\n4. Die Inbetriebnahme muß dem zuständigen Finanzamt gemeldet werden.\n5. Die Kassenidentifikationsnummer kann nicht mehr geändert werden"),
                                   QMessageBox::Yes | QMessageBox::No,
                                   Q_NULLPTR);
            messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Ja"));
            messageBox.setButtonText(QMessageBox::No, QObject::tr("Nein"));
            if (messageBox.exec() == QMessageBox::Yes ) {
                QRKProgress waitbar;
                waitbar.setText(tr("DEP-7 wird aktiviert."));
                waitbar.setWaitMode(true);
                waitbar.show();

                Reports rep(Q_NULLPTR, true);
                bool check = rep.checkEOAnyServerMode();
                if (check) {
                    ReceiptItemModel rec;
                    if (rec.createStartReceipt()) {
                        m_scardActivateButton->setVisible(false);
                        waitbar.close();
                        QMessageBox::information(this,tr("DEP-7 wurde aktiviert!"), tr("DEP-7 wurde aktiviert. Die Kasse ist RKSV-konform.\n Der STARTBELEG wurde erstellt."), "Ok");
                    } else {
                        waitbar.close();
                        QMessageBox::information(this,tr("Fehler!"), tr("DEP-7 konnte nicht aktiviert werden. Siehe Log."), "Ok");
                    }
                } else {
                    waitbar.close();
                    QMessageBox::information(this,tr("Fehler!"), tr("DEP-7 konnte nicht aktiviert werden. Tages/Monatsabschluss wurde schon erstellt."), "Ok");
                }
            }
        } else {
            QMessageBox messageBox(QMessageBox::Question,
                                   QObject::tr("Kasse außer Betrieb nehmen"),
                                   QObject::tr("Achtung!\nDie Kasse wird außer Betrieb genommen. Das DEP-7 (RKSV Daten Erfassungs Prodokoll) wird de-aktiviert. Melden Sie die außer Betriebnahme dem zuständigen Finanzamt."),
                                   QMessageBox::Yes | QMessageBox::No,
                                   Q_NULLPTR);
            messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Ja"));
            messageBox.setButtonText(QMessageBox::No, QObject::tr("Nein"));
            if (messageBox.exec() == QMessageBox::Yes ) {
                QRKProgress waitbar;
                waitbar.setText(tr("Signaturerstellungseinheit wird geprüft."));
                waitbar.setWaitMode(true);
                waitbar.show();

                Reports rep(Q_NULLPTR, true);
                bool check = rep.checkEOAnyServerMode();
                if (check) {
                    RKSignatureModule::setDEPactive(false);
                    m_scardActivateButton->setText(tr("Kasse außer Betrieb."));
                    m_scardActivateButton->setEnabled(false);
                    ReceiptItemModel rec;
                    if (rec.createNullReceipt(CONCLUSION_RECEIPT)) {
                        m_scardActivateButton->setVisible(false);
                        Database::setCashRegisterInAktive();
                        waitbar.close();
                        QMessageBox::information(this,tr("Außer Betrieb!"), tr("Die Kasse wurde außer Betrieb genommen.\n Der SCHLUSSBELEG wurde erstellt."), "Ok");
                    } else {
                        waitbar.close();
                        QMessageBox::information(this,tr("Fehler!"), tr("DEP-7 konnte nicht aktiviert werden. Siehe Log."), "Ok");
                    }
                } else {
                    waitbar.close();
                    QMessageBox::information(this,tr("Fehler!"), tr("DEP-7 konnte nicht aktiviert werden. Tages/Monatsabschluss wurde schon erstellt."), "Ok");
                }
            }
        }
    }
}
