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

#include "printerdelegate.h"
#include "database.h"

#include <QStyleOptionViewItem>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QComboBox>
#include <QTimer>
#include <QApplication>

PrinterDelegate::PrinterDelegate(const QStringList &printers, QObject *parent, bool combo)
    : QStyledItemDelegate(parent), m_printers(printers), m_combo(combo)
{
}

QWidget *PrinterDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem & /* option */,
                                       const QModelIndex &index) const
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->setEditable(false);
    if (index.column() == 1 && m_combo) {
        comboBox->addItems(m_printers);
    } else if (index.column() == 2 && m_combo) {
        QJsonArray jArray = Database::getDefinitions();
        foreach (const QJsonValue &value, jArray) {
            QJsonObject obj = value.toObject();
            comboBox->addItem(obj["name"].toString(), obj["id"].toInt());
        }
    }
    QTimer::singleShot(0, comboBox, &QComboBox::showPopup);

    connect(comboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &PrinterDelegate::emitCommitData);

    return comboBox;
}

void PrinterDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
    if (!comboBox)
        return;

    int pos = comboBox->findText(index.model()->data(index).toString(),
                                 Qt::MatchExactly);
    comboBox->setCurrentIndex(pos);
}

void PrinterDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
    if (!comboBox)
        return;

    model->setData(index, comboBox->currentText());
}

void PrinterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (index.column() != 0 && m_combo) {
        QStyleOptionComboBox box;
        box.state = option.state;

        box.rect = option.rect;
        box.currentText = index.data().toString();

        QApplication::style()->drawComplexControl(QStyle::CC_ComboBox, &box, painter, Q_NULLPTR);
        QApplication::style()->drawControl(QStyle::CE_ComboBoxLabel, &box, painter, Q_NULLPTR);
        return;
    }
    QStyledItemDelegate::paint(painter,option, index);
}

QString PrinterDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    QByteArray ba = QByteArray::fromBase64(value.toByteArray());
    QJsonDocument doc = QJsonDocument::fromBinaryData(ba);
    QJsonArray jArray = QJsonDocument().fromBinaryData(ba).array();
    if (jArray.isEmpty())
        return QStyledItemDelegate::displayText(value, locale);

    QStringList list;
    int i = 0;
    foreach (const QJsonValue &value, jArray) {
        QJsonObject obj = value.toObject();
        list.append(QString("%1: %2 (%3)").arg(++i).arg(obj["name"].toString()).arg(Database::getDefinitionName(obj["definitionid"].toInt())));
    }
    return QStyledItemDelegate::displayText(list.join(",\n"), locale);
}

void PrinterDelegate::emitCommitData(int id)
{
    Q_UNUSED(id);
    emit commitData(qobject_cast<QWidget *>(sender()));
}
