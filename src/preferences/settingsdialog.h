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

#include "3rdparty/ckvsoft/widget.h"
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
class QTimeEdit;
class RelationalTableModel;
class QTableView;

class ReceiptLayoutTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptLayoutTab(QWidget *parent = Q_NULLPTR);
    bool useProductCategories();
    bool getHideProductCategories();
    bool getHideProductGroup();
    QSize getStretchFactor();
    int getLayoutDirection();
    QSize getQuickButtonSize();
    QSize getButtonSize();
    QSize getNumpadButtonSize();

private:
    void useProductCategoriesChanged(bool checked);

    QCheckBox *m_useProductCategoriesCheck;
    QCheckBox *m_showProductCategoriesCheck;
    QCheckBox *m_showProductGroupCheck;

    QSpinBox *m_stretchProductGroup_0_SpinBox;
    QSpinBox *m_stretchProductGroup_1_SpinBox;
    QComboBox *m_stretchProductGroupComboBox;

    QSpinBox *m_quickButtonWidth;
    QSpinBox *m_quickButtonHeight;
    QSpinBox *m_fixedButtonHeight;
    QSpinBox *m_minimumButtonWidth;
    QSpinBox *m_fixedNumpadButtonHeight;
    QSpinBox *m_fixedNumpadButtonWidth;

};

class ReceiptMainTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptMainTab(QWidget *parent = Q_NULLPTR);
    bool getInputNetPrice();
    bool getDiscount();
    bool getMaximumItemSold();
    int getDecimalDigits();
    bool getDecimalQuantity();
    bool hideDebitcardButton();
    bool hideCreditcardButton();
    bool hideR2BButton();
    QString getDefaultTax();
    bool useVirtualNumPad();
    bool useInputProductNumber();
    bool getRegisterHeaderMovable();
    bool getOptionalDescriptionButton();

signals:
    void maximumSoldItemChanged(bool enabled);

private:
    QCheckBox *m_useInputNetPriceCheck;
    QCheckBox *m_useDiscountCheck;
    QCheckBox *m_useMaximumItemSoldCheck;
    QCheckBox *m_useDecimalQuantityCheck;
    QSpinBox *m_decimalRoundSpin;
    QCheckBox *m_optionalDescriptionButtonCheck;
    QCheckBox *m_hideDebitcardCheck;
    QCheckBox *m_hideCreditcardCheck;
    QCheckBox *m_hideR2BButtonCheck;
    QCheckBox *m_useVirtualNumPadCheck;
    QCheckBox *m_useInputProductNumberCheck;
    QCheckBox *m_registerHeaderMoveableCheck;

    QComboBox *m_defaultTaxComboBox;

};

class ReportTab : public Widget
{
    Q_OBJECT

public:
    explicit ReportTab(QWidget *parent = Q_NULLPTR);
    QTime getCurfewTime();
    bool getProductGroup();

private:
    void curfewChanged(QTime time);
    QTimeEdit *m_curfewTimeEdit;
    QCheckBox *m_useProductGroupCheck;

};

class BarcodeTab : public Widget
{
    Q_OBJECT

public:
    explicit BarcodeTab(QWidget *parent = Q_NULLPTR);
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
    explicit ExtraTab(QWidget *parent = Q_NULLPTR);
    bool getGivenDialog();
    bool getPriceChangedDialog();
    bool getStockDialog();
    bool getSalesWidget();
    bool getReceiptPrintedDialog();
    bool getGroupSeparator();
    int getFirstProductnumber();
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
    void groupSeparatorChanged(bool checked);

private:
    QCheckBox *m_usePriceChangedCheck;
    QCheckBox *m_useGivenDialogCheck;
    QCheckBox *m_salesWidgetCheck;
    QCheckBox *m_useReceiptPrintedDialogCheck;
    QCheckBox *m_useStockDialogCheck;
    QCheckBox *m_groupSeparatorCheck;

    QSpinBox *m_firstProductnumber;

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
    QLabel *m_groupSeparatorLabel;

    QFont m_systemFont;
    QFont m_printerFont;
    QFont m_receiptPrinterFont;

};

class ServerTab : public Widget
{
    Q_OBJECT

public:
    explicit ServerTab(QWidget *parent = Q_NULLPTR);
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
    explicit GeneralTab(QWidget *parent = Q_NULLPTR);
    QString getBackupDirectory();
    QString getPdfDirectory();
    QString getDataDirectory();
    QString getExternalDepDirectory();
    int getKeepMaxBackups();

public slots:
    void masterTaxChanged(const QString &tax);

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

class PrinterDefinitionTab : public Widget
{
    Q_OBJECT

public:
    explicit PrinterDefinitionTab(QWidget *parent = Q_NULLPTR);

private:
    RelationalTableModel *m_model = Q_NULLPTR;
    QTableView *m_tableView = Q_NULLPTR;

    void addNew();
    void remove();
    void edit();
};

class PrinterConfigTab : public Widget
{
    Q_OBJECT

public:
    explicit PrinterConfigTab(const QStringList &availablePrinters, QWidget *parent = Q_NULLPTR);

private:
    RelationalTableModel *m_model = Q_NULLPTR;
    QTableView *m_tableView = Q_NULLPTR;

    void addNew();
    void remove();
    void edit();

    const QStringList m_availablePrinters;
};

class PrinterTab : public Widget
{
    Q_OBJECT

public:
    explicit PrinterTab(QWidget *parent = Q_NULLPTR);

    int getReportPrinter();
    int getInvoiceCompanyPrinter();
    int getReceiptPrinter();
    int getCollectionPrinter();

private:

    QComboBox *m_reportPrinterCombo;
    QComboBox *m_invoiceCompanyPrinterCombo;
    QComboBox *m_receiptPrinterCombo;
    QComboBox *m_collectionPrinterCombo;

};

class CollectionReceiptTab : public Widget
{
    Q_OBJECT

public:
    explicit CollectionReceiptTab(QWidget *parent = Q_NULLPTR);

    bool getPrintCollectionReceipt();
    QString getCollectionReceiptText();

private:

    QCheckBox *m_printCollectionReceiptCheck;
    QLineEdit *m_collectionReceiptTextEdit;

};

class ReceiptTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptTab(QWidget *parent = Q_NULLPTR);

    QString getReceiptPrinterHeading();
    bool getPrintCompanyNameBold();
    bool getIsLogoRight();
    bool getPrintQRCode();
    bool getPrintQRCodeLeft();
    QString getLogo();
    bool getUseLogo();

    int getfeedProdukt();
    int getfeedCompanyHeader();
    int getfeedCompanyAddress();
    int getfeedCashRegisterid();
    int getfeedTimestamp();
    int getfeedTax();
    int getfeedPrintHeader();
    int getfeedHeaderText();
    int getfeedQRCode();

private slots:
    void logoButton_clicked();
    void useLogoCheck_toggled(bool);

private:
    QLineEdit *m_logoEdit;
    QCheckBox *m_useLogo;
    QPushButton *m_logoButton;

    QComboBox *m_receiptPrinterHeading;
    QCheckBox *m_printCompanyNameBoldCheck;
    QCheckBox *m_useLogoRightCheck;
    QCheckBox *m_printQRCodeCheck;
    QCheckBox *m_printQRCodeLeftCheck;

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

class ReceiptEnhancedTab : public Widget
{
    Q_OBJECT

public:
    explicit ReceiptEnhancedTab(QWidget *parent = Q_NULLPTR);

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
    explicit MasterDataTab(QWidget *parent = Q_NULLPTR);

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
    explicit SCardReaderTab(QWidget *parent = Q_NULLPTR);
    QString getCurrentCardReader();
    QString getCurrentOnlineConnetionString();
    bool saveSettings();

private slots:
    void scardGroup_clicked(bool);
    void onlineGroup_clicked(bool);
    void testButton_clicked(bool);
    void activateButton_clicked(bool);

private:
    bool sharedMode();

    QComboBox *m_scardReaderComboBox;
    QComboBox *m_providerComboBox;
    QLineEdit *m_providerLoginEdit;
    QLineEdit *m_providerPasswordEdit;

    QGroupBox *m_scardGroup;
    QGroupBox *m_onlineGroup;
    QListWidget *m_infoWidget;

    QCheckBox *m_sharedconnection;
    QrkPushButton *m_scardActivateButton;

    RKSignatureModule *m_rkSignature;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = Q_NULLPTR);

private slots:
    void masterTaxChanged(const QString &tax);
    void accept();

private:
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;

    GeneralTab *m_general;
    MasterDataTab *m_master;
    PrinterDefinitionTab *m_printerDefinition;
    PrinterConfigTab *m_printerConfig;
    CollectionReceiptTab *m_collectiopnReceiptTab;

    PrinterTab *m_printer;
    ReceiptMainTab *m_receiptmain;
    ReportTab *m_report;
    ReceiptLayoutTab *m_receiptlayout;
    ReceiptTab *m_receipt;
    BarcodeTab *m_barcode;
    ReceiptEnhancedTab *m_receiptenhanced;
    ExtraTab *m_extra;
    ServerTab *m_server;
    SCardReaderTab *m_scardreader;
    Journal *m_journal;
};

#endif
