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

#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include "RK/rk_signaturemodule.h"

class QDialogButtonBox;
class QTabWidget;
class TextEdit;
class QCompleter;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLineEdit;
class QRadioButton;
class QLabel;
class QrkPushButton;
class QGroupBox;
class QListWidget;
class ASignSmartCard;
class Journal;
class QStackedWidget;

class Widget: public QWidget
{
        Q_OBJECT
    public:
        explicit Widget(QWidget *parent = 0):QWidget(parent){}

        void disableWidgets()
        {
            QList<QWidget *> widgets = this->findChildren<QWidget *>();

            foreach(QWidget* widget, widgets)
            {
                widget->setEnabled(false);
            }
        }
};

class ReceiptMainTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptMainTab(QWidget *parent = 0);
    bool getInputNetPrice();
    bool getDiscount();
    bool getMaximumItemSold();
    int getDecimalDigits();
    bool getDecimalQuantity();
    QSize getQuickButtonSize();
    QSize getButtonSize();
    QSize getNumpadButtonSize();
    bool hideDebitcardButton();
    bool hideCreditcardButton();
    QString getDefaultTax();
    bool useVirtualNumPad();
    bool useInputProductNumber();
    bool getRegisterHeaderMovable();

signals:
    void maximumSoldItemChanged(bool enabled);

private:
    QCheckBox *m_useInputNetPriceCheck;
    QCheckBox *m_useDiscountCheck;
    QCheckBox *m_useMaximumItemSoldCheck;
    QCheckBox *m_useDecimalQuantityCheck;
    QSpinBox *m_decimalRoundSpin;
    QCheckBox *m_hideDebitcardCheck;
    QCheckBox *m_hideCreditcardCheck;
    QCheckBox *m_useVirtualNumPadCheck;
    QCheckBox *m_useInputProductNumberCheck;
    QCheckBox *m_registerHeaderMoveableCheck;

    QComboBox *m_defaultTaxComboBox;

    QSpinBox *m_quickButtonWidth;
    QSpinBox *m_quickButtonHeight;
    QSpinBox *m_fixedButtonHeight;
    QSpinBox *m_minimumButtonWidth;
    QSpinBox *m_fixedNumpadButtonHeight;
    QSpinBox *m_fixedNumpadButtonWidth;

};

class BarcodeTab : public Widget
{
    Q_OBJECT

public:
    explicit BarcodeTab(QWidget *parent = 0);
    int getBarcodePrefix();
    QString getBarcodeSuccessSound();
    QString getBarcodeFailureSound();
    bool getBarcodeSuccessSoundEnabled();
    bool getBarcodeFailureSoundEnabled();
    bool getBarcodeAsDefault();
    void play();
    QString getMultimediaPath();
    void setMultimediaPath();

private:
    QLineEdit *m_barcodeMultimediaPath;
    QComboBox *m_barcodePrefixesComboBox;
    QCheckBox *m_barcodeSoundSuccess;
    QCheckBox *m_barcodeSoundFailure;
    QCheckBox *m_barcodeInputLineEditDefault;
    QComboBox *m_barcodeSuccessCombo;
    QComboBox *m_barcodeFailureCombo;
};

class ExtraTab : public Widget
{
    Q_OBJECT

public:
    explicit ExtraTab(QWidget *parent = 0);
    bool getGivenDialog();
    bool getStockDialog();
    bool getSalesWidget();
    bool getReceiptPrintedDialog();
    bool getProductGroup();
    bool isFontsGroup();
    QString getSystemFont();
    QString getPrinterFont();
    QString getReceiptPrinterFont();

signals:
public slots:
    void maximumSoldItemChanged(bool enabled);

private slots:
    void systemFontButton_clicked(bool);
    void printerFontButton_clicked(bool);
    void receiptPrinterFontButton_clicked(bool);
    void printerTestButton_clicked(bool);
    void receiptPrinterTestButton_clicked(bool);
    void fontsGroup_toggled(bool toggled);

private:
    QCheckBox *m_useProductGroupCheck;
    QCheckBox *m_useGivenDialogCheck;
    QCheckBox *m_salesWidgetCheck;
    QCheckBox *m_useReceiptPrintedDialogCheck;
    QCheckBox *m_useStockDialogCheck;

    QGroupBox *m_fontsGroup;

    QrkPushButton *m_systemFontButton;
    QrkPushButton *m_printerFontButton;
    QrkPushButton *m_receiptPrinterFontButton;

    QLabel *m_systemFontSizeLabel;
    QLabel *m_printerFontSizeLabel;
    QLabel *m_receiptPrinterFontSizeLabel;

    QLabel *m_systemFontStretchLabel;
    QLabel *m_printerFontStretchLabel;
    QLabel *m_receiptPrinterFontStretchLabel;

    QFont m_systemFont;
    QFont m_printerFont;
    QFont m_receiptPrinterFont;

};

class ServerTab : public Widget
{
    Q_OBJECT

public:
    explicit ServerTab(QWidget *parent = 0);
    QString getImportDirectory();
    QString getImportCodePage();
    bool getServerFullscreen();
    bool getServerCriticalMessageBox();

private slots:
    void importDirectoryButton_clicked();

private:
    QLineEdit *m_importDirectoryEdit;
    QComboBox *m_codePageCombo;
    QCheckBox *m_serverFullscreen;
    QCheckBox *m_serverCriticalMessageBox;

};

class GeneralTab : public Widget
{
    Q_OBJECT

public:
    explicit GeneralTab(QWidget *parent = 0);
    QString getBackupDirectory();
    QString getPdfDirectory();
    QString getDataDirectory();
    QString getExternalDepDirectory();
    int getKeepMaxBackups();

public slots:
    void masterTaxChanged(QString tax);

private slots:
    void backupDirectoryButton_clicked();
    void pdfDirectoryButton_clicked();
    void dataDirectoryButton_clicked();
    void externalDepDirectoryButton_clicked();

private:
    bool moveDataFiles( QString fromDir, QString toDir);

    QLineEdit *m_backupDirectoryEdit;
    QSpinBox  *m_keepMaxBackupSpinBox;
    QLineEdit *m_pdfDirectoryEdit;
    QLineEdit *m_dataDirectoryEdit;
    QLineEdit *m_externalDepDirectoryEdit;
    QGroupBox *m_externalDepGroup;

};

class PrinterTab : public Widget
{
    Q_OBJECT

public:
    explicit PrinterTab(QStringList availablePrinters, QWidget *parent = 0);

    bool getReportPrinterPDF();
    QString getReportPrinter();
    QString getPaperFormat();
    QString getInvoiceCompanyPrinter();
    QString getInvoiceCompanyPaperFormat();
    int getInvoiceCompanyMarginLeft();
    int getInvoiceCompanyMarginRight();
    int getInvoiceCompanyMarginTop();
    int getInvoiceCompanyMarginBottom();

private slots:
    void reportPrinterCheck_toggled(bool);

private:

    QCheckBox *m_reportPrinterCheck;
    QComboBox *m_reportPrinterCombo;
    QComboBox *m_paperFormatCombo;
    QComboBox *m_invoiceCompanyPrinterCombo;
    QComboBox *m_invoiceCompanyPaperFormatCombo;
    QSpinBox *m_invoiceCompanyMarginLeftSpin;
    QSpinBox *m_invoiceCompanyMarginRightSpin;
    QSpinBox *m_invoiceCompanyMarginTopSpin;
    QSpinBox *m_invoiceCompanyMarginBottomSpin;
};

class ReceiptPrinterTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptPrinterTab(QStringList availablePrinters, QWidget *parent = 0);

    QString getReceiptPrinter();
    bool getUseReportPrinter();
    int getNumberCopies();
    int getpaperWidth();
    int getpaperHeight();
    int getmarginLeft();
    int getmarginRight();
    int getmarginTop();
    int getmarginBottom();

    int getfeedProdukt();
    int getfeedCompanyHeader();
    int getfeedCompanyAddress();
    int getfeedCashRegisterid();
    int getfeedTimestamp();
    int getfeedTax();
    int getfeedPrintHeader();
    int getfeedHeaderText();
    int getfeedQRCode();

private:
//    QComboBox *m_paperFormatCombo;
    QComboBox *m_receiptPrinterCombo;
    QCheckBox *m_useReportPrinterCheck;
    QSpinBox *m_fontSizeSpin;
    QSpinBox *m_grossFontSpin;
    QSpinBox *m_numberCopiesSpin;
    QSpinBox *m_paperWidthSpin;
    QSpinBox *m_paperHeightSpin;
    QSpinBox *m_marginLeftSpin;
    QSpinBox *m_marginRightSpin;
    QSpinBox *m_marginTopSpin;
    QSpinBox *m_marginBottomSpin;

    QSpinBox *m_feedProduktSpin;
    QSpinBox *m_feedCompanyHeaderSpin;
    QSpinBox *m_feedCompanyAddressSpin;
    QSpinBox *m_feedCashRegisteridSpin;
    QSpinBox *m_feedTimestampSpin;
    QSpinBox *m_feedTaxSpin;
    QSpinBox *m_feedPrintHeaderSpin;
    QSpinBox *m_feedHeaderTextSpin;
    QSpinBox *m_feedQRCodeSpin;

};

class ReceiptTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptTab(QStringList availablePrinters, QWidget *parent = 0);

    QString getReceiptPrinterHeading();
    bool getPrintCompanyNameBold();
    QString getCollectionPrinter();
    QString getCollectionPrinterPaperFormat();
    bool getPrintCollectionReceipt();
    QString getCollectionReceiptText();
    int getCollectionReceiptCopies();
    bool getIsLogoRight();
    bool getPrintQRCode();
    bool getPrintQRCodeLeft();
    QString getLogo();
    bool getUseLogo();

private slots:
    void logoButton_clicked();
    void useLogoCheck_toggled(bool);

private:
    QLineEdit *m_logoEdit;
    QCheckBox *m_useLogo;
    QPushButton *m_logoButton;

    QComboBox *m_collectionPrinterCombo;
    QComboBox *m_collectionPrinterPaperFormatCombo;
    QComboBox *m_receiptPrinterHeading;
    QCheckBox *m_printCompanyNameBoldCheck;
    QCheckBox *m_printCollectionReceiptCheck;
    QLineEdit *m_collectionReceiptTextEdit;
    QSpinBox  *m_collectionReceiptCopiesSpin;
    QCheckBox *m_useLogoRightCheck;
    QCheckBox *m_printQRCodeCheck;
    QCheckBox *m_printQRCodeLeftCheck;
};

class ReceiptEnhancedTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptEnhancedTab(QWidget *parent = 0);

    QString getAdvertising();
    bool getUseAdvertising();

    QString getAdvertisingText();
    QString getHeader();
    QString getFooter();

private slots:
    void advertisingButton_clicked();
    void useAdvertisingCheck_toggled(bool);

private:
    QAbstractItemModel *modelFromFile(const QString& fileName);
    QCompleter *completer;

    TextEdit *m_printAdvertisingEdit;
    TextEdit *m_printHeaderEdit;
    TextEdit *m_printFooterEdit;

    QLineEdit *m_advertisingEdit;
    QCheckBox *m_useAdvertising;
    QPushButton *m_advertisingButton;
};

class MasterDataTab : public Widget
{
    Q_OBJECT

public:
    explicit MasterDataTab(QWidget *parent = 0);

    QString getShopName();
    QString getShopOwner();
    QString getShopAddress();
    QString getShopUid();
    QString getShopCashRegisterId();
    QString getShopTaxes();
    QString getShopCurrency();

signals:
    void taxChanged(QString);

private slots:
    void cashRegisterIdChanged();

private:
    QLineEdit *m_shopName;
    QLineEdit *m_shopOwner;
     TextEdit *m_shopAddress;
    QLineEdit *m_shopUid;
    QLineEdit *m_shopCashRegisterId;
    QComboBox *m_currency;
    QComboBox *m_taxlocation;

};

class SCardReaderTab : public Widget
{
    Q_OBJECT

public:
    explicit SCardReaderTab(QWidget *parent = 0);
    QString getCurrentCardReader();
    QString getCurrentOnlineConnetionString();
    bool saveSettings();

private slots:
    void scardGroup_clicked(bool);
    void onlineGroup_clicked(bool);
    void testButton_clicked(bool);
    void activateButton_clicked(bool);

private:

    QComboBox *m_scardReaderComboBox;
    QComboBox *m_providerComboBox;
    QLineEdit *m_providerLoginEdit;
    QLineEdit *m_providerPasswordEdit;

    QGroupBox *m_scardGroup;
    QGroupBox *m_onlineGroup;
    QListWidget *m_infoWidget;

    QrkPushButton *m_scardActivateButton;

    RKSignatureModule *m_rkSignature;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);

private slots:
    void masterTaxChanged(QString);
    void accept();

private:
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;

    GeneralTab *m_general;
    MasterDataTab *m_master;
    PrinterTab *m_printer;
    ReceiptPrinterTab *m_receiptprinter;
    ReceiptMainTab *m_receiptmain;
    ReceiptTab *m_receipt;
    BarcodeTab *m_barcode;
    ReceiptEnhancedTab *m_receiptenhanced;
    ExtraTab *m_extra;
    ServerTab *m_server;
    SCardReaderTab *m_scardreader;
    Journal *m_journal;
};

#endif
