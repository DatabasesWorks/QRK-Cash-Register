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

#ifndef BUTTONCOLUMNDELEGATE_H
#define BUTTONCOLUMNDELEGATE_H

#include "globals_ckvsoft.h"

#include <QStyledItemDelegate>
#include <QPushButton>

class CKVSOFT_EXPORT ButtonColumnDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ButtonColumnDelegate(QObject *parent = Q_NULLPTR);
    explicit ButtonColumnDelegate(const QString &icon, QObject *parent = Q_NULLPTR);
    ~ButtonColumnDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    //    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void setModelData(QWidget *editor, QAbstractItemModel *modal, const QModelIndex &index) const Q_DECL_OVERRIDE;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    QString m_icon = "";

};

#endif // BUTTONCOLUMNDELEGATE_H
