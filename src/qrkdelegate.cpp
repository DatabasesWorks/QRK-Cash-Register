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

#include "qrkdelegate.h"
#include "preferences/qrksettings.h"
#include "database.h"

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QCompleter>
#include <QPainter>
#include <QDateTime>
#include <QDebug>

#include "iostream"

QrkDelegate::QrkDelegate(int type, QObject *parent)
    :QStyledItemDelegate(parent), m_type(type)
{
    m_shortcurrency = Database::getShortCurrency();
    m_taxlocation = Database::getTaxLocation();
}

QWidget* QrkDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &item, const QModelIndex &index) const
{
    if (m_type == SPINBOX) {
        QSpinBox *spinbox = new QSpinBox(parent);
        spinbox->setMinimum(-99999);
        spinbox->setMaximum(99999);
        spinbox->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        connect( spinbox , static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &QrkDelegate::commitAndCloseEditor) ;

        return spinbox;

    } else if (m_type == DOUBLE_SPINBOX) {
        QrkSettings settings;
        int digits = settings.value("decimalDigits", 2).toInt();
        QDoubleSpinBox *doublespinbox = new QDoubleSpinBox(parent);
        if (digits > 2) {
            doublespinbox->setMinimum(-99999.999);
            doublespinbox->setMaximum(99999.999);
            doublespinbox->setSingleStep(0.001);
        } else {
            doublespinbox->setMinimum(-99999.99);
            doublespinbox->setMaximum(99999.99);
            doublespinbox->setSingleStep(0.01);
        }
        doublespinbox->setDecimals(digits);
        doublespinbox->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        // connect( doublespinbox , static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &QrkDelegate::commitAndCloseEditor) ;

        return doublespinbox;

    } else if (m_type == COMBO_TAX) {
        QComboBox *combo = new QComboBox(parent);
        combo->setEditable(false);
        combo->setInsertPolicy(QComboBox::InsertAfterCurrent);
        combo->setDuplicatesEnabled(false);
        QSqlDatabase dbc= Database::database();
        QSqlQuery query(dbc);
        query.prepare(QString("SELECT tax FROM taxTypes WHERE taxlocation=:taxlocation ORDER BY id"));
        query.bindValue(":taxlocation", m_taxlocation);
        if(!query.exec()){
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Can't get taxType list! Taxlocation = " << m_taxlocation;
            qWarning() << "Function Name: " << Q_FUNC_INFO << " Query:" << Database::getLastExecutedQuery(query);
        }
        while(query.next()){
            combo->addItem(query.value(0).toString());
        }
        combo->setCurrentIndex(combo->findText(index.data().value<QString>()));
        return combo;

    } else if (m_type == PRODUCTS) {
        QLineEdit *editor = new QLineEdit( parent );
        editor->setPlaceholderText(tr("Artikelname"));
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);
//        query.prepare("SELECT name FROM products WHERE visible = 1");
        query.prepare("select DISTINCT p2.name from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where visible = 1");
        query.exec();

        QStringList list;

        while(query.next()){
            QString value = query.value("name").toString();
            list << value;
        }

        QCompleter *editorCompleter = new QCompleter( list ) ;
        editorCompleter->setCaseSensitivity( Qt::CaseInsensitive ) ;
        editorCompleter->setFilterMode( Qt::MatchContains );
        editor->setCompleter( editorCompleter );

        connect( editor , &QLineEdit::textChanged, this, &QrkDelegate::commitAndCloseEditor) ;

        return editor ;
    } else if (m_type == PRODUCTSNUMBER) {
        QLineEdit *editor = new QLineEdit( parent );
        QSqlDatabase dbc = Database::database();
        QSqlQuery query(dbc);
//        query.prepare("SELECT itemnum FROM products WHERE visible = 1 AND itemnum IS NOT ''");
        query.prepare("select DISTINCT p2.itemnum from (select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as  p2 on p1.version=p2.version and p1.origin=p2.origin where visible = 1 AND itemnum > ''");
        query.exec();

        QStringList list;

        while(query.next()){
            QString value = query.value("itemnum").toString();
            list << value;
        }

        QCompleter *editorCompleter = new QCompleter( list ) ;
        editorCompleter->setCaseSensitivity( Qt::CaseInsensitive ) ;
        editorCompleter->setFilterMode( Qt::MatchContains );
        editor->setCompleter( editorCompleter );

        connect( editor , &QLineEdit::textChanged, this, &QrkDelegate::commitAndCloseEditor) ;
//        QRegExp rx("[1-9]\\d{1,16}"); // without leading 0
//        QRegExp rx("\\d{0,16}");
//        QValidator *intVal = new QRegExpValidator(rx);
//        editor->setValidator(intVal);

        return editor ;
    } else if (m_type == NUMBERFORMAT_DOUBLE || m_type == DISCOUNT) {
        QLineEdit* editor = new QLineEdit(parent);
        if (m_type == DISCOUNT) {
            QRegExpValidator* rxv = new QRegExpValidator(QRegExp("^\\d{0,2}([\\.\\,]\\d{1,2})?$"));
            editor->setValidator(rxv);
        } else {
            QRegExpValidator* rxv = new QRegExpValidator(QRegExp("[-+]?[0-9]*[\\.,]?[0-9]+([eE][-+]?[0-9]+)?"));
            editor->setValidator(rxv);
        }
        editor->setAlignment(Qt::AlignRight|Qt::AlignVCenter);

        return editor;
    }

    return QStyledItemDelegate::createEditor(parent, item, index);

}

QString QrkDelegate::displayText(const QVariant &value, const QLocale &locale) const
{

    if (m_type == NUMBERFORMAT_DOUBLE) {
        int x = QString::number(value.toDouble()).length() - QString::number(value.toDouble()).indexOf(".");
        QString formattedNum;
        if (x > 3 && QString::number(value.toDouble()).indexOf(".") > 0) {
            formattedNum = QString("%1").arg(QLocale().toString(value.toDouble(), 'f', 3));
            formattedNum = QString("%1.. %2").arg(formattedNum.left(formattedNum.length() - 1)).arg(m_shortcurrency);
        } else {
            formattedNum = QString("%1 %2").arg(QLocale().toString(value.toDouble(), 'f', 2)).arg(m_shortcurrency);
        }
        return formattedNum;
    } else if (m_type == DISCOUNT) {
        QString formattedNum;
        double val = value.toString().replace(",",".").toDouble();
        if (val > 0)
            val *= -1;

        formattedNum = QString("%1 %").arg(QLocale().toString(val, 'f', 2));
        return formattedNum;

    } else if (m_type == COMBO_TAX) {
        QString formattedNum;
        if (m_taxlocation == "CH")
            formattedNum = QString("%1 %").arg(QLocale().toString(value.toDouble()));
        else
            formattedNum = QString("%1 %").arg(QLocale().toString(value.toDouble(), 'f', 0));

        return formattedNum;
    } else if (m_type == SPINBOX) {
        QString formattedNum;
        formattedNum = QLocale().toString(value.toDouble(), 'f', 0);

        return formattedNum;
    } else if (m_type == DOUBLE_SPINBOX) {
        QrkSettings settings;
        int digits = settings.value("decimalDigits", 2).toInt();
        QString formattedNum;
        formattedNum = QLocale().toString(value.toDouble(), 'f', digits);

        return formattedNum;

    }

    return QStyledItemDelegate::displayText(value, locale);
}

void QrkDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{

    if (m_type == SPINBOX) {
        // Get the value via index of the Model
        // use toDuble while toInt gives someone 0
        double value = index.model()->data(index, Qt::EditRole).toDouble();
        // Put the value into the SpinBox
        QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
        spinbox->setValue(value);
    } else if (m_type == DOUBLE_SPINBOX) {
        // Get the value via index of the Model
        double value = index.model()->data(index, Qt::EditRole).toDouble();
        // Put the value into the SpinBox
        QDoubleSpinBox *doublespinbox = static_cast<QDoubleSpinBox*>(editor);
        doublespinbox->setValue(value);
    } else if (m_type == COMBO_TAX) {
        if(index.data().canConvert<QString>()){
            QString taxTitle= index.data().value<QString>();
            QComboBox *combo= static_cast<QComboBox *>(editor);
            combo->setCurrentIndex(combo->findText(taxTitle));
        }
    } else if(m_type == PRODUCTS || m_type == PRODUCTSNUMBER){
        QLineEdit *edit = static_cast<QLineEdit*>( editor ) ;
        edit->setText( index.data(Qt::EditRole).toString());
    } else if (m_type == NUMBERFORMAT_DOUBLE || m_type == DISCOUNT) {
        QString v = index.model()->data(index,Qt::EditRole).toString();
        double value = v.replace(",",".").toDouble();

        if (m_type == DISCOUNT) {
            if (value < 0.00 || value > 99.99)
                value = 0.00;
        }
        QLineEdit* line = static_cast<QLineEdit*>(editor);
        line->setText(QString().setNum(value));
    }
}

void QrkDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{

    if (m_type == SPINBOX) {
        QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
        spinbox->interpretText();
        int value = spinbox->value();
        model->setData(index, value);
    } else if (m_type == DOUBLE_SPINBOX) {
        QDoubleSpinBox *doublespinbox = static_cast<QDoubleSpinBox*>(editor);
        doublespinbox->interpretText();
        double value = doublespinbox->value();
        model->setData(index, value);

    } else if (m_type == COMBO_TAX) {
        if (index.data().canConvert(QMetaType::QString)){
            QComboBox *combo = static_cast<QComboBox *>(editor);
            model->setData(index, QVariant::fromValue(combo->currentText().trimmed()));
        }

    } else if (m_type == PRODUCTS || m_type == PRODUCTSNUMBER) {
        QLineEdit *edit = static_cast<QLineEdit *>( editor ) ;
        model->setData( index, edit->text() ) ;

    } else if (m_type == NUMBERFORMAT_DOUBLE || m_type == DISCOUNT) {
        QLineEdit* line = static_cast<QLineEdit*>(editor);
        QString value = line->text();
        model->setData(index,value);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void QrkDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const
{

    editor->setGeometry(option.rect);

}

void QrkDelegate::paint(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{

    switch (m_type)
    {
    case NUMBERFORMAT_DOUBLE:
    case NUMBERFORMAT_INT:
    case COMBO_TAX:
    case DISCOUNT:
        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;

        QStyledItemDelegate::paint(painter, myOption, index);
        return;
    }

    QStyledItemDelegate::paint(painter, option, index);
}

void QrkDelegate::commitAndCloseEditor()
{
    if (m_type == COMBO_TAX) {
        QComboBox *combo= static_cast<QComboBox *>(sender());
        emit commitData(combo);
        emit closeEditor(combo);
    } else if (m_type == SPINBOX) {
        QSpinBox *spinbox= static_cast<QSpinBox *>(sender());
        emit commitData(spinbox);
    } else if (m_type == DOUBLE_SPINBOX) {
        QDoubleSpinBox *doublespinbox= static_cast<QDoubleSpinBox *>(sender());
        emit commitData(doublespinbox);
    } else if (m_type == PRODUCTS || m_type == PRODUCTSNUMBER) {
        QLineEdit *editor = static_cast<QLineEdit *>(sender());
        emit commitData( editor ) ;
    }
}
