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
#include "database.h"
#include "ui_givendialog.h"

#include <QDoubleValidator>

GivenDialog::GivenDialog(double &sum, QWidget *parent) :
    QDialog(parent), ui(new Ui::GivenDialog)
{
    ui->setupUi(this);
    ui->lcdNumber->setText(QLocale().toString(0.0,'f',2) + " " + Database::getCurrency());

    QDoubleValidator *doubleVal = new QDoubleValidator(0.0, 9999999.99, 2, this);
    doubleVal->setNotation(QDoubleValidator::StandardNotation);
    ui->givenEdit->setValidator(doubleVal);
    m_sum = sum;
    ui->toPayLabel->setText(tr("Zu bezahlen: %1 %2").arg(QLocale().toString(sum,'f',2)).arg(Database::getCurrency()));

    QPalette palette = ui->lcdNumber->palette();

    palette.setColor(ui->lcdNumber->backgroundRole(), Qt::darkGreen);
    palette.setColor(ui->lcdNumber->foregroundRole(), Qt::darkGreen);
    ui->lcdNumber->setPalette(palette);

    connect (ui->givenEdit, &QLineEdit::textChanged, this, &GivenDialog::textChanged);
    connect (ui->pushButton, &QPushButton::clicked, this, &GivenDialog::accept);
    connect (ui->cancelButton, &QPushButton::clicked, this, &GivenDialog::close);

}

GivenDialog::~GivenDialog()
{
    delete ui;
}

void GivenDialog::accept()
{

  QDialog::accept();
}

void GivenDialog::textChanged(QString given)
{

    double retourMoney = QLocale().toDouble(given) - m_sum;
    QPalette palette = ui->lcdNumber->palette();

    if (retourMoney >= 0.0) {
        palette.setColor(ui->lcdNumber->backgroundRole(), Qt::darkGreen);
        palette.setColor(ui->lcdNumber->foregroundRole(), Qt::darkGreen);
    } else {
        palette.setColor(ui->lcdNumber->backgroundRole(), Qt::red);
        palette.setColor(ui->lcdNumber->foregroundRole(), Qt::red);
    }
    ui->lcdNumber->setPalette(palette);
//    ui->lcdNumber->display(QString::number(retourMoney,'f',2));
    ui->lcdNumber->setText(QLocale().toString(retourMoney,'f',2) + " " + Database::getCurrency());

}

double GivenDialog::getGiven()
{
    bool ok = false;
    double value = QLocale().toDouble(ui->givenEdit->text(), &ok);
    if (ok)
        return value;

    return 0.0;
}
