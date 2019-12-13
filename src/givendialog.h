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

#ifndef GIVENDIALOG_H
#define GIVENDIALOG_H

#include <QDialog>
#include <QMap>

class NumericKeypad;

namespace Ui {
class GivenDialog;
}

class GivenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GivenDialog(double &sum, QWidget *parent = Q_NULLPTR);
    ~GivenDialog();
    QMap<int, double> getGiven();

private slots:
    void accept();
    void textChanged(QString);
    void mixedButton();
    void mixedPay(int id, bool checked);
    void numPadToogle(bool);

private:
    Ui::GivenDialog *ui;
    NumericKeypad *m_numericKeyPad = Q_NULLPTR;
    double getGivenValue();
    void resetGiven();
    void setLCDPalette(QColor color);

    double m_sum;
    QMap<int, double> mixedMap;
    bool m_mixed = false;
};

#endif // GIVENDIALOG_H
