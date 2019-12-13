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

#include "barcodessettings.h"
#include "qrkpushbutton.h"
#include "../qrkcore/preferences/qrksettings.h"

#include <QGroupBox>
#include <QFormLayout>
#include <QLabel>
#include <QFontDatabase>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>

BarcodesSettings::BarcodesSettings(QWidget *parent)
    : QWidget(parent)
{

    int id = QFontDatabase::addApplicationFont(":font/free3of9.ttf");
    QFontDatabase::applicationFontFamilies(id).at(0);

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

    m_actionGroup = new QGroupBox();
    m_actionGroup->setCheckable(true);
    m_actionGroup->setChecked(false);
    m_actionGroup->setTitle(tr("Barcode Plugin verwenden"));
    QFormLayout *actionLayout = new QFormLayout();
    actionLayout->addRow(tr("BON Abschluss:"), m_barcode_finishReceipt);
    actionLayout->addRow(tr("Letzten Artikel entfernen:"), m_barcode_removeLastPosition);
    actionLayout->addRow(tr("Tagesabschluss:"), m_barcode_endOfDay);
    actionLayout->addRow(tr("Rabatt ändern:"), m_barcode_discount);
    actionLayout->addRow(tr("Preis ändern:"), m_barcode_editPrice);
    actionLayout->addRow(tr("Kopie des letzten BON Drucken:"), m_barcode_printLastReceiptAgain);
    actionLayout->addRow(tr("Storno des letzten BONs:"), m_barcode_cancelLastReceipt);
    actionLayout->addRow(tr("Wert  00:"), m_barcode_amount_00);
    actionLayout->addRow(tr("Wert 000:"), m_barcode_amount_000);
    actionLayout->addRow(tr("Wert 250:"), m_barcode_amount_250);
    actionLayout->addRow(tr("Wert 500:"), m_barcode_amount_500);
    m_actionGroup->setLayout(actionLayout);

    m_amountGroup = new QGroupBox();
    m_amountGroup->setEnabled(m_actionGroup->isChecked());
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
    amountLayout->addRow(tr("Wert   9:"), m_barcode_amount_9);

    m_amountGroup->setLayout(amountLayout);

    QrkPushButton *cancelButton = new QrkPushButton;
    cancelButton->setMinimumHeight(60);
    cancelButton->setMinimumWidth(0);
    QIcon cancelicon = QIcon(":src/icons/cancel.png");
    QSize size = QSize(32,32);
    cancelButton->setIcon(cancelicon);
    cancelButton->setIconSize(size);
    cancelButton->setText(tr("Beenden"));

    QrkPushButton *saveButton = new QrkPushButton;
    saveButton->setMinimumHeight(60);
    saveButton->setMinimumWidth(0);

    QIcon saveicon = QIcon(":src/icons/save.png");
    saveButton->setIcon(saveicon);
    saveButton->setIconSize(size);
    saveButton->setText(tr("Speichern"));

    QrkPushButton *printBarcodesButton = new QrkPushButton;
    printBarcodesButton->setMinimumHeight(60);
    printBarcodesButton->setMinimumWidth(0);

    QIcon printicon = QIcon(":src/icons/ok-print.png");
    printBarcodesButton->setIcon(printicon);
    printBarcodesButton->setIconSize(size);
    printBarcodesButton->setText(tr("Speichern und Drucken"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QSpacerItem* spacer = new QSpacerItem( 0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding );
    buttonLayout->addItem(spacer);
    buttonLayout->addWidget(printBarcodesButton);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(m_actionGroup, 0,0);
    mainLayout->addWidget(m_amountGroup, 0,1);
    mainLayout->addLayout(buttonLayout, 1, 0, 1, 2, Qt::AlignRight);

    connect(printBarcodesButton, &QPushButton::clicked, this, &BarcodesSettings::printBarcodes);
    connect(saveButton, &QPushButton::clicked, this, &BarcodesSettings::save);
    connect(cancelButton, &QPushButton::clicked, this, &BarcodesSettings::cancelClicked);
    connect(m_actionGroup, &QGroupBox::toggled, this, &BarcodesSettings::actionGroupChanged);

    setLayout(mainLayout);

    init();
}

void BarcodesSettings::init()
{
    QrkSettings settings;
    settings.beginGroup("BarCodesPlugin");

    m_actionGroup->setChecked(settings.value("barcode_enabled", false).toBool());

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

void BarcodesSettings::save()
{
    QrkSettings settings;
    settings.beginGroup("BarCodesPlugin");

    settings.save2Settings("barcode_enabled", m_actionGroup->isChecked());
    settings.save2Settings("barcodeFinishReceipt", m_barcode_finishReceipt->text());
    settings.save2Settings("barcodeRemoveLastPosition", m_barcode_removeLastPosition->text());
    settings.save2Settings("barcodeEndOfDay", m_barcode_endOfDay->text());
    settings.save2Settings("barcodeDiscount", m_barcode_discount->text());
    settings.save2Settings("barcodeEditPrice", m_barcode_editPrice->text());
    settings.save2Settings("barcodePrintLastReceiptAgain", m_barcode_printLastReceiptAgain->text());
    settings.save2Settings("barcodeCancelReceipt", m_barcode_cancelLastReceipt->text());
    settings.save2Settings("barcodeAmount_0", m_barcode_amount_0->text());
    settings.save2Settings("barcodeAmount_1", m_barcode_amount_1->text());
    settings.save2Settings("barcodeAmount_2", m_barcode_amount_2->text());
    settings.save2Settings("barcodeAmount_3", m_barcode_amount_3->text());
    settings.save2Settings("barcodeAmount_4", m_barcode_amount_4->text());
    settings.save2Settings("barcodeAmount_5", m_barcode_amount_5->text());
    settings.save2Settings("barcodeAmount_6", m_barcode_amount_6->text());
    settings.save2Settings("barcodeAmount_7", m_barcode_amount_7->text());
    settings.save2Settings("barcodeAmount_8", m_barcode_amount_8->text());
    settings.save2Settings("barcodeAmount_9", m_barcode_amount_9->text());
    settings.save2Settings("barcodeAmount_00", m_barcode_amount_00->text());
    settings.save2Settings("barcodeAmount_000", m_barcode_amount_000->text());
    settings.save2Settings("barcodeAmount_250", m_barcode_amount_250->text());
    settings.save2Settings("barcodeAmount_500", m_barcode_amount_500->text());
    settings.endGroup();
}

void BarcodesSettings::printBarcodes()
{
    QPrinter printer;

    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle("Print Document");
    if (dialog->exec() != QDialog::Accepted)
        return;

    QTextDocument doc;

    QString text = "<table style=\"width: 100%;\" cellspacing=\"5\" cellpadding=\"5\"><tbody><tr>"
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_finishReceipt->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_removeLastPosition->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("BON Abschluss") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Letzten Artikel entfernen") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_endOfDay->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_discount->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Tagesabschluss") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Rabatt ändern") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_editPrice->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_printLastReceiptAgain->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Preis ändern") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Kopie des letzten BON Drucken") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_cancelLastReceipt->text() + "*</td><td>&nbsp;</td><td>&nbsp;</td>" +
            "</tr><tr>" +
              "<td>" + tr("Storno des letzten BONs:") + "</td><td>&nbsp;</td><td>&nbsp;</td>" +
            "</tr></tbody></table>";

            doc.setHtml(text);
            doc.adjustSize();
            doc.print(&printer);

     text = "<table style=\"width: 100%;\" cellspacing=\"5\" cellpadding=\"5\"><tbody><tr>"
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_0->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_1->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 0") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 1") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_2->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_3->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 2") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 3") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_4->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_5->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 4") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 5") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_6->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_7->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 6") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 7") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_8->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_9->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 8") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 9") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_00->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_000->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 00") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 000") + "</td>" +
            "</tr><tr>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_250->text() + "*</td><td>&nbsp;</td>" +
              "<td style=\"font-family:free 3 of 9;font-size:50px\">*" + m_barcode_amount_500->text() + "*</td>" +
            "</tr><tr>" +
              "<td>" + tr("Wert 250") + "</td><td>&nbsp;</td>" +
              "<td>" + tr("Wert 500") + "</td>" +
            "</tr></tbody></table>";

    doc.setHtml(text);
    doc.adjustSize();
    doc.print(&printer);

    emit save();
}

void BarcodesSettings::actionGroupChanged(bool checked)
{
    m_amountGroup->setEnabled(checked);
}
