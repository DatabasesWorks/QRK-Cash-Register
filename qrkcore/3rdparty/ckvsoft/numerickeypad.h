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

#ifndef NUMERICKEYPAD_H
#define NUMERICKEYPAD_H

#include "globals_ckvsoft.h"
#include "defines.h"

#include <QWidget>

class QLineEdit;
class QrkPushButton;

class CKVSOFT_EXPORT NumericKeypad : public QWidget
{
    Q_OBJECT

public:
    NumericKeypad(bool full = true, QWidget *parent = 0 );
    void clear();

    const QString text() const;
    void discountButtonSetEnabled(bool enabled);
    void setEditFocus();

public slots:
    void setText( const QString &text );
    void setCount(bool);
    void setDiscount(bool);
    void setSinglePrice(bool);
    void setPrice(bool);
    void backspace(bool);

signals:
    void textChanged( const QString &text );
    void valueButtonPressed( const QString &text, REGISTER_COL column );

private slots:
    void buttonClicked( const QString &text );

private:

    QrkPushButton *m_buttonDiscount = 0;
    QLineEdit *m_lineEdit;
    QString m_text;
    int m_digits = 2;
};

#endif // NUMERICKEYPAD_H
