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

#ifndef BARCODESSETTINGS_H
#define BARCODESSETTINGS_H

#include <QWidget>
#include <QLineEdit>

class BarcodesSettings : public QWidget
{
    Q_OBJECT

public:
    explicit BarcodesSettings(QWidget *parent = 0);

signals:
    void cancelClicked();

private:
    void init();
    void printBarcodes();
    void save();

    QLineEdit *m_barcode_finishReceipt;
    QLineEdit *m_barcode_removeLastPosition;
    QLineEdit *m_barcode_endOfDay;
    QLineEdit *m_barcode_discount;
    QLineEdit *m_barcode_editPrice;

    QLineEdit *m_barcode_printLastReceiptAgain;
    QLineEdit *m_barcode_cancelLastReceipt;

    QLineEdit *m_barcode_amount_0;
    QLineEdit *m_barcode_amount_1;
    QLineEdit *m_barcode_amount_2;
    QLineEdit *m_barcode_amount_3;
    QLineEdit *m_barcode_amount_4;
    QLineEdit *m_barcode_amount_5;
    QLineEdit *m_barcode_amount_6;
    QLineEdit *m_barcode_amount_7;
    QLineEdit *m_barcode_amount_8;
    QLineEdit *m_barcode_amount_9;
    QLineEdit *m_barcode_amount_00;
    QLineEdit *m_barcode_amount_000;
    QLineEdit *m_barcode_amount_250;
    QLineEdit *m_barcode_amount_500;

};


#endif // BARCODESSETTINGS_H
