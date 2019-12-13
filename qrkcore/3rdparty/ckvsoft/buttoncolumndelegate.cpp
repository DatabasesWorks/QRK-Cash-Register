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

#include "buttoncolumndelegate.h"
#include <QPainter>
#include <QStylePainter>
#include <QApplication>

ButtonColumnDelegate::ButtonColumnDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

ButtonColumnDelegate::ButtonColumnDelegate(const QString &icon, QObject *parent) : QStyledItemDelegate(parent), m_icon(icon)
{
}

ButtonColumnDelegate::~ButtonColumnDelegate()
{
}

void ButtonColumnDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionButton button;
    button.rect = option.rect;
    if (m_icon.isEmpty())
        button.text = index.data().toString();
    else
        button.icon = QIcon(m_icon);

    button.iconSize = QSize(24,24);
    button.state = QStyle::State_Enabled;
    QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter);
}

QWidget *ButtonColumnDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    QPushButton *btn = new QPushButton(parent);
    if (m_icon.isEmpty())
        btn->setText(index.data().toString());
    else
        btn->setIcon(QIcon(m_icon));

    btn->setIconSize(QSize(24,24));
    //        btn->setText("...");
    return btn;
}

void ButtonColumnDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QPushButton *btn = qobject_cast<QPushButton*>(editor);
    btn->setProperty("data_value", index.data());
    //        btn->setProperty("data_value", "...");
    if (m_icon.isEmpty())
        btn->setText(index.data().toString());
    else
        btn->setIcon(QIcon(m_icon));

    btn->setIconSize(QSize(24,24));
}

void ButtonColumnDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QPushButton *btn = qobject_cast<QPushButton*>(editor);
    model->setData(index, btn->property("data_value"));
}

void ButtonColumnDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /* index */ ) const
{
    editor->setGeometry(option.rect);
}
