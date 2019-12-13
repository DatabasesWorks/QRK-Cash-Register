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

#include "givendialog.h"
#include "defines.h"
#include "database.h"
#include "3rdparty/ckvsoft/numerickeypad.h"
#include "preferences/qrksettings.h"
#include "ui_givendialog.h"

#include <QDoubleValidator>
#include <QFontDatabase>

GivenDialog::GivenDialog(double &sum, QWidget *parent) :
    QDialog(parent), ui(new Ui::GivenDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    /*
    int id = QFontDatabase::addApplicationFont(":/src/font/digital.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    QFont digital(family);
    digital.setBold(false);
    digital.setPointSize(40);

    ui->lcdNumber->setFont(digital);
    */

    m_numericKeyPad = new NumericKeypad(false, this);
    ui->numericKeyPadLayout->addWidget(m_numericKeyPad);
    ui->mixedFrame->setHidden(true);
    ui->lcdNumber->setText(QLocale().toString(0.0,'f',2) + " " + Database::getCurrency());

    ui->buttonGroup->setId(ui->creditCard, PAYED_BY_CREDITCARD);
    ui->buttonGroup->setId(ui->debitCard, PAYED_BY_DEBITCARD);
    m_numericKeyPad->setHidden(true);

    QDoubleValidator *doubleVal = new QDoubleValidator(0.0, 9999999.99, 2, this);
    doubleVal->setNotation(QDoubleValidator::StandardNotation);
    ui->givenEdit->setValidator(doubleVal);
    m_sum = sum;
    ui->toPayLabel->setText(tr("Zu bezahlen: %1 %2").arg(QLocale().toString(sum,'f',2)).arg(Database::getCurrency()));

    QPalette palette = ui->lcdNumber->palette();

    palette.setColor(ui->lcdNumber->backgroundRole(), Qt::darkGreen);
    palette.setColor(ui->lcdNumber->foregroundRole(), Qt::darkGreen);
    ui->lcdNumber->setPalette(palette);

    QrkSettings settings;
    m_numericKeyPad->setVisible(settings.value("virtualNumPad", false).toBool());

    connect (ui->givenEdit, &QLineEdit::textChanged, this, &GivenDialog::textChanged);
    connect (ui->finishButton, &QPushButton::clicked, this, &GivenDialog::accept);
    connect (ui->cancelButton, &QPushButton::clicked, this, &GivenDialog::close);
    connect (ui->mixedButton, &QPushButton::clicked, this, &GivenDialog::mixedButton);
    connect(ui->buttonGroup, static_cast<void(QButtonGroup::*)(int, bool)>(&QButtonGroup::buttonToggled), this, &GivenDialog::mixedPay);
    connect(m_numericKeyPad, &NumericKeypad::textChanged, [=]() {
        ui->givenEdit->setText(m_numericKeyPad->text());
    });
    connect(ui->numericKeyPadPushButton, &QrkPushButton::clicked, this, &GivenDialog::numPadToogle);

    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

GivenDialog::~GivenDialog()
{
    delete ui;
}

void GivenDialog::mixedButton()
{
    ui->mixedFrame->setHidden(false);
    ui->mixedButton->setEnabled(false);
    mixedPay(PAYED_BY_DEBITCARD, true);
}

void GivenDialog::mixedPay(int id, bool checked)
{
    m_mixed = true;
    ui->mixedButton->setEnabled(false);
    ui->finishButton->setEnabled(true);

    if (checked && id == PAYED_BY_DEBITCARD) {
        ui->mixTypeLabel->setText(tr("Mit Bankomatkarte:"));
        ui->mixedCashLabel->setText(QLocale().toString(m_sum - QLocale().toDouble(ui->givenEdit->text())));
        setLCDPalette(Qt::darkGreen);

    } else if (checked && id == PAYED_BY_CREDITCARD) {
        ui->mixTypeLabel->setText(tr("Mit Kreditkarte:"));
        ui->mixedCashLabel->setText(QLocale().toString(m_sum - QLocale().toDouble(ui->givenEdit->text())));
        setLCDPalette(Qt::darkGreen);

    } else {
        ui->debitCard->setChecked(false);
        ui->creditCard->setChecked(false);
        ui->mixTypeLabel->setText("");
        ui->mixedCashLabel->setText("");
        m_mixed = false;
    }

    double retourMoney = m_sum - QLocale().toDouble(ui->givenEdit->text()) - QLocale().toDouble(ui->mixedCashLabel->text());
    ui->lcdNumber->setText(QLocale().toString(retourMoney,'f',2) + " " + Database::getCurrency());

}

void GivenDialog::accept()
{
    if (m_mixed) {
        if (ui->debitCard->isChecked()) {
            mixedMap.insert(PAYED_BY_DEBITCARD, QLocale().toDouble(ui->mixedCashLabel->text()));
        } else {
            mixedMap.insert(PAYED_BY_CREDITCARD, QLocale().toDouble(ui->mixedCashLabel->text()));
        }
    }
    mixedMap.insert(PAYED_BY_CASH, getGivenValue());

    QDialog::accept();
}

void GivenDialog::textChanged(QString given)
{

    if (given.isEmpty()) {
        resetGiven();
        return;
    }

    m_mixed = false;

    double retourMoney = QLocale().toDouble(given) - m_sum;

    if (retourMoney >= 0.0) {
        setLCDPalette(Qt::darkGreen);
        ui->mixedButton->setEnabled(false);
        ui->mixedFrame->setHidden(true);
        ui->finishButton->setEnabled(true);
        emit mixedPay(0, false);
    } else {
        setLCDPalette(Qt::red);
        ui->mixedButton->setEnabled(true);
        ui->finishButton->setEnabled(false);
        ui->mixedFrame->setHidden(true);
    }

    ui->lcdNumber->setText(QLocale().toString(retourMoney,'f',2) + " " + Database::getCurrency());

}

void GivenDialog::resetGiven()
{
    m_mixed = false;

    setLCDPalette(Qt::darkGreen);
    ui->mixedButton->setEnabled(false);
    ui->mixedFrame->setHidden(true);
    ui->finishButton->setEnabled(true);
    ui->debitCard->setChecked(false);
    ui->creditCard->setChecked(false);
    ui->mixTypeLabel->setText("");
    ui->mixedCashLabel->setText("");

    double retourMoney = 0.0;
    ui->lcdNumber->setText(QLocale().toString(retourMoney,'f',2) + " " + Database::getCurrency());
}

void GivenDialog::setLCDPalette(QColor color)
{
    QPalette palette = ui->lcdNumber->palette();
    palette.setColor(ui->lcdNumber->backgroundRole(), color);
    palette.setColor(ui->lcdNumber->foregroundRole(), color);
    ui->lcdNumber->setPalette(palette);
}

double GivenDialog::getGivenValue()
{
    bool ok = false;
    double value = QLocale().toDouble(ui->givenEdit->text(), &ok);
    if (ok)
        return value;

    return 0.0;
}

QMap<int, double> GivenDialog::getGiven()
{
    return mixedMap;
}

void GivenDialog::numPadToogle(bool)
{
    bool hidden = m_numericKeyPad->isHidden();
    if (hidden) {
        m_numericKeyPad->setVisible(hidden);
    } else {
        m_numericKeyPad->setVisible(hidden);
        emit m_numericKeyPad->clear();
    }
}
