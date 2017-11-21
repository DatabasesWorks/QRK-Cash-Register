#include "barcodessettings.h"
#include "../qrkcore/preferences/qrksettings.h"

#include <QGroupBox>
#include <QFormLayout>

BarcodesSettings::BarcodesSettings(QWidget *parent)
    : QWidget(parent)
{

    m_barcode_finishReceipt = new QLineEdit(this);
    m_barcode_removeLastPosition = new QLineEdit(this);
    m_barcode_endOfDay = new QLineEdit(this);
    m_barcode_discount = new QLineEdit(this);
    m_barcode_editPrice = new QLineEdit(this);

    m_barcode_printLastReceiptAgain = new QLineEdit(this);
    m_barcode_cancelLastReceipt = new QLineEdit(this);

    m_barcode_amount_0 = new QLineEdit(this);
    m_barcode_amount_1 = new QLineEdit(this);
    m_barcode_amount_2 = new QLineEdit(this);
    m_barcode_amount_3 = new QLineEdit(this);
    m_barcode_amount_4 = new QLineEdit(this);
    m_barcode_amount_5 = new QLineEdit(this);
    m_barcode_amount_6 = new QLineEdit(this);
    m_barcode_amount_7 = new QLineEdit(this);
    m_barcode_amount_8 = new QLineEdit(this);
    m_barcode_amount_9 = new QLineEdit(this);
    m_barcode_amount_00 = new QLineEdit(this);
    m_barcode_amount_000 = new QLineEdit(this);
    m_barcode_amount_250 = new QLineEdit(this);
    m_barcode_amount_500 = new QLineEdit(this);

    QGroupBox *actionGroup = new QGroupBox();
    QFormLayout *actionLayout = new QFormLayout();
    actionLayout->addRow(tr("BON Abschluß:"), m_barcode_finishReceipt);
    actionLayout->addRow(tr("Letzten Artikel entfernen:"), m_barcode_removeLastPosition);
    actionLayout->addRow(tr("Tagesabschluß:"), m_barcode_endOfDay);
    actionLayout->addRow(tr("Rabatt ändern:"), m_barcode_discount);
    actionLayout->addRow(tr("Preis ändern:"), m_barcode_editPrice);
    actionLayout->addRow(tr("Kopie des letzten BON Drucken:"), m_barcode_printLastReceiptAgain);
    actionLayout->addRow(tr("Storno des letzten BONs:"), m_barcode_cancelLastReceipt);
    actionGroup->setLayout(actionLayout);

    QGroupBox *amountGroup = new QGroupBox();
    QFormLayout *amountLayout = new QFormLayout;
    amountLayout->setAlignment(Qt::AlignLeft);
    amountLayout->addRow(tr("Wert   0:"), m_barcode_amount_0);
    amountLayout->addRow(tr("Wert   1:"), m_barcode_amount_1);
    amountLayout->addRow(tr("Wert   2:"), m_barcode_amount_2);
    amountLayout->addRow(tr("Wert   3:"), m_barcode_amount_3);
    amountLayout->addRow(tr("Wert   4:"), m_barcode_amount_4);
    amountLayout->addRow(tr("Wert   5:"), m_barcode_amount_5);
    amountLayout->addRow(tr("Wert   6:"), m_barcode_amount_6);
    amountLayout->addRow(tr("Wert   7:"), m_barcode_amount_7);
    amountLayout->addRow(tr("Wert   8:"), m_barcode_amount_8);
    amountLayout->addRow(tr("Wert   8:"), m_barcode_amount_9);
    amountLayout->addRow(tr("Wert  00:"), m_barcode_amount_00);
    amountLayout->addRow(tr("Wert 000:"), m_barcode_amount_000);
    amountLayout->addRow(tr("Wert 250:"), m_barcode_amount_250);
    amountLayout->addRow(tr("Wert 500:"), m_barcode_amount_500);
    amountGroup->setLayout(amountLayout);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(actionGroup, 0,0);
    mainLayout->addWidget(amountGroup, 0,1);

    setLayout(mainLayout);

    QrkSettings settings;
    settings.beginGroup("BarCodesPlugin");

    m_barcode_finishReceipt->setText(settings.value("barcodeFinishReceipt", "100009000001").toString());
    m_barcode_removeLastPosition->setText(settings.value("barcodeRemoveLastPosition", "100009000002").toString());
    m_barcode_endOfDay->setText(settings.value("barcodeEndOfDay", "100009000003").toString());
    m_barcode_discount->setText(settings.value("barcodeDiscount", "100009000007").toString());
    m_barcode_editPrice->setText(settings.value("barcodeEditPrice", "100009000010").toString());
    m_barcode_printLastReceiptAgain->setText(settings.value("barcodePrintLastReceiptAgain", "100009000005").toString());
    m_barcode_cancelLastReceipt->setText(settings.value("barcodeCancelReceipt", "100009000006").toString());
    m_barcode_amount_0->setText(settings.value("barcodeAmount_0", "100008000000").toString());
    m_barcode_amount_1->setText(settings.value("barcodeAmount_1", "100008000001").toString());
    m_barcode_amount_2->setText(settings.value("barcodeAmount_2", "100008000002").toString());
    m_barcode_amount_3->setText(settings.value("barcodeAmount_3", "100008000003").toString());
    m_barcode_amount_4->setText(settings.value("barcodeAmount_4", "100008000004").toString());
    m_barcode_amount_5->setText(settings.value("barcodeAmount_5", "100008000005").toString());
    m_barcode_amount_6->setText(settings.value("barcodeAmount_6", "100008000006").toString());
    m_barcode_amount_7->setText(settings.value("barcodeAmount_7", "100008000007").toString());
    m_barcode_amount_8->setText(settings.value("barcodeAmount_8", "100008000008").toString());
    m_barcode_amount_9->setText(settings.value("barcodeAmount_9", "100008000009").toString());
    m_barcode_amount_00->setText(settings.value("barcodeAmount_00", "100008000020").toString());
    m_barcode_amount_000->setText(settings.value("barcodeAmount_000", "100008000030").toString());
    m_barcode_amount_250->setText(settings.value("barcodeAmount_250", "100008000250").toString());
    m_barcode_amount_500->setText(settings.value("barcodeAmount_500", "100008000500").toString());

    settings.endGroup();

}
