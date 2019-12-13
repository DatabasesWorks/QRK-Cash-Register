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

#include "qrkpushbutton.h"
#include "numerickeypad.h"
#include "preferences/qrksettings.h"
#include "3rdparty/qbcmath/bcmath.h"

#include <QGridLayout>
#include <QLineEdit>
#include <QApplication>
#include <QDoubleValidator>
#include <QDesktopWidget>

NumericKeypad::NumericKeypad( bool full, QWidget *parent )
    :QWidget(parent)
{

    QrkSettings settings;
    setHidden(true);
    QGridLayout *layout = new QGridLayout( this );
    layout->setMargin(0);

    m_lineEdit = new QLineEdit();
    m_lineEdit->setAlignment( Qt::AlignRight );

    m_digits = settings.value("decimalDigits", 2).toInt();
    if (!settings.value("useDecimalQuantity", false).toBool())
        m_digits = 0;

    QSize buttonSize = settings.value("numpadButtonSize", QSize(30,30)).toSize();
    if ( QApplication::desktop()->height() < 768 ) {
        buttonSize.setHeight(30);
        buttonSize.setWidth(30);
    }

    QString style = "QPushButton {"
                    "color: white;"
                    "border: 1px solid #199909;"
                    "border-radius: 6px;"
                    "background-color: rgba(0, 0, 232, 0.5);"
                    "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                    "stop: 0 rgba(22, 86, 232, 0.5), stop: 1 rgba(22, 86, 232, 1));"
                    "}"
                    "QPushButton:pressed {"
                    "color: white;"
                    "border: 1px solid #333333;"
                    "background-color: #222222;"
                    "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                    "stop: 0 #444444, stop: 1 #222222);"
                    "}";

    /*
    style = "background-color: qradialgradient(spread:pad, cx:0.499807, cy:0.489, radius:0.5, fx:0.499, fy:0.488909, stop:0.0795455 rgba(0, 147, 185, 255), stop:1 rgba(30, 30, 30, 255));"
            "border: 5px solid black;"
            "border-radius: 6px;";
    */

    QString styleRed = "QPushButton {"
                       "color: white;"
                       "border: 1px solid #199909;"
                       "border-radius: 6px;"
                       "background-color: rgba(222, 0, 0, 1);"
                       "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                       "stop: 0 rgba(111, 0, 0, 0.5), stop: 1 rgba(255, 0, 0, 1));"
                       "}"
                       "QPushButton:pressed {"
                       "color: white;"
                       "border: 1px solid #333333;"
                       "background-color: #222222;"
                       "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 0.67, "
                       "stop: 0 #444444, stop: 1 #222222);"
                       "}";

    for(int i = 1; i <= 9; ++i){
        QrkPushButton *singleButton = new QrkPushButton();
        singleButton->setStyleSheet(style);
        singleButton->setFixedSize(buttonSize);
        singleButton->setText(QString::number(i));
        layout->addWidget(singleButton, (12 - i) / 3, (i-1) % 3);
        QString text = QString().setNum(i);
        connect(singleButton, &QPushButton::clicked, this, [this, text](){ buttonClicked(text); });
    }

    QrkPushButton *buttonZero = new QrkPushButton( "0" );
    buttonZero->setStyleSheet(style);
    buttonZero->setFixedSize(buttonSize);

    QrkPushButton *buttonDot = new QrkPushButton( QLocale().decimalPoint() );
    buttonDot->setStyleSheet(style);
    buttonDot->setFixedSize(buttonSize);

    QrkPushButton *buttonClear = new QrkPushButton( tr("C") );
    buttonClear->setStyleSheet(styleRed);
    //    buttonClear->setFixedSize(buttonSize);
    buttonClear->setMaximumHeight(16777215);
    buttonClear->setFixedWidth(buttonSize.width());
    buttonClear->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QrkPushButton *buttonPlusMinus = new QrkPushButton( tr("+/-") );
    buttonPlusMinus->setStyleSheet(style);
    buttonPlusMinus->setFixedSize(buttonSize);

    if (full) {
        QrkPushButton *buttonCount = new QrkPushButton( tr("Anzahl") );
        buttonCount->setStyleSheet("Text-align:left");
        buttonCount->setIcon(QIcon(":src/icons/count.png"));
        buttonCount->setIconSize(QSize(32, 32));
        buttonCount->setFixedHeight(buttonSize.height());

        QrkPushButton *buttonSinglePrice = new QrkPushButton( tr("E-Preis") );
        buttonSinglePrice->setStyleSheet("Text-align:left");
        buttonSinglePrice->setIcon(QIcon(":src/icons/singleprice.png"));
        buttonSinglePrice->setIconSize(QSize(32, 32));
        buttonSinglePrice->setFixedHeight(buttonSize.height());

        m_buttonDiscount = new QrkPushButton( tr("Rabatt") );
        m_buttonDiscount->setStyleSheet("Text-align:left");
        m_buttonDiscount->setIcon(QIcon(":src/icons/discount.png"));
        m_buttonDiscount->setIconSize(QSize(32, 32));
        m_buttonDiscount->setFixedHeight(buttonSize.height());

        QrkPushButton *buttonPrice = new QrkPushButton( tr("Preis") );
        buttonPrice->setStyleSheet("Text-align:left");
        buttonPrice->setIcon(QIcon(":src/icons/sum.png"));
        buttonPrice->setIconSize(QSize(32, 32));
        buttonPrice->setFixedHeight(buttonSize.height());
        layout->addWidget( buttonCount, 1, 4, 1, 2);
        layout->addWidget( buttonSinglePrice, 2, 4, 1, 2);
        layout->addWidget( buttonPrice, 4, 4, 1, 2);
        layout->addWidget( m_buttonDiscount, 3, 4, 1, 2);

        connect( buttonCount, &QPushButton::clicked, this, &NumericKeypad::setCount);
        connect( buttonSinglePrice, &QPushButton::clicked, this, &NumericKeypad::setSinglePrice);
        connect( m_buttonDiscount, &QPushButton::clicked, this, &NumericKeypad::setDiscount);
        connect( buttonPrice, &QPushButton::clicked, this, &NumericKeypad::setPrice);
    } else {
        QrkPushButton *buttonBackspace = new QrkPushButton();
        buttonBackspace->setIcon(QIcon(":src/icons/backspace.png"));
        buttonBackspace->setIconSize(QSize(32, 32));
        buttonBackspace->setFixedSize(buttonSize);
        buttonBackspace->setStyleSheet(style);

 //       layout->addWidget( m_lineEdit, 0, 0, 1, 4 );
 //       layout->addWidget( buttonBackspace, 1, 3, 1, 1 );
        connect(buttonBackspace, &QPushButton::clicked, this, &NumericKeypad::backspace);
    }

    layout->addWidget( buttonPlusMinus, 4, 0);
    layout->addWidget( buttonZero, 4, 1 );
    layout->addWidget( buttonDot, 4, 2 );
    layout->addWidget( buttonClear, 3, 3, 2, 1 );

    connect(buttonPlusMinus, &QPushButton::clicked, this, [this](){ buttonClicked("-"); });
    connect(buttonZero, &QPushButton::clicked, this, [this](){ buttonClicked("0"); });
    connect(buttonDot, &QPushButton::clicked, this, [this](){ buttonClicked(QLocale().decimalPoint()); });
    connect(buttonClear, &QPushButton::clicked, m_lineEdit, &QLineEdit::clear);
    connect( m_lineEdit, &QLineEdit::textChanged, this, &NumericKeypad::setText);

}

const QString NumericKeypad::text() const
{
    return m_text;
}

void NumericKeypad::discountButtonSetEnabled(bool enabled)
{
    m_buttonDiscount->setEnabled(enabled);

}

void NumericKeypad::buttonClicked( const QString &newText )
{
    if (newText == QLocale().decimalPoint() && m_text.indexOf(QLocale().decimalPoint()) > 0)
        return;
    else if (newText == QLocale().decimalPoint() && m_text.isEmpty()) {
        setText( "0" + newText);
        return;
    }

    if (newText == "-" && m_text.indexOf('-') >= 0) {
        QString text = m_text;
        setText( text.remove("-"));
        return;
    } else if (newText == "-") {
        setText( newText + m_text);
        return;
    }

    setText( m_text + newText );
}

void NumericKeypad::setEditFocus()
{
    m_lineEdit->setFocus();
    clear();
}

void NumericKeypad::setText( const QString &newText )
{
    if( newText == m_text )
        return;

    m_text = newText;
    m_lineEdit->setText( m_text );

    emit textChanged( m_text );
}

void NumericKeypad::clear()
{
    m_lineEdit->setText("");
}

void NumericKeypad::backspace(bool)
{
    QString text = m_text;
    text.chop(1);
    setText(text);
}

void NumericKeypad::setCount(bool)
{
    if (m_lineEdit->text().isEmpty())
        return;

    QBCMath text(QLocale().toDouble(m_lineEdit->text()));

    if (m_digits == 0) {
        emit valueButtonPressed( QString::number(text.toInt()), REGISTER_COL_COUNT );
    } else {
        text.round(m_digits);
        emit valueButtonPressed( text.toString(), REGISTER_COL_COUNT );
    }

    m_lineEdit->setText("");
}

void NumericKeypad::setDiscount(bool)
{
    if (m_lineEdit->text().isEmpty())
        return;

    QBCMath text(QLocale().toDouble(m_lineEdit->text()));
    text.round(2);

    emit valueButtonPressed( text.toString(), REGISTER_COL_DISCOUNT );
    m_lineEdit->setText("");
}

void NumericKeypad::setSinglePrice(bool)
{
    if (m_lineEdit->text().isEmpty())
        return;

    QBCMath text(QLocale().toDouble(m_lineEdit->text()));
    text.round(2);

    emit valueButtonPressed( text.toString(), REGISTER_COL_SINGLE );
    m_lineEdit->setText("");
}

void NumericKeypad::setPrice(bool)
{
    if (m_lineEdit->text().isEmpty())
        return;

    QBCMath text(QLocale().toDouble(m_lineEdit->text()));
    text.round(2);

    emit valueButtonPressed( text.toString(), REGISTER_COL_TOTAL );
    m_lineEdit->setText("");
}
