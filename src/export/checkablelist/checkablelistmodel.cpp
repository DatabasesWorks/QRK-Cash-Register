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

#include "checkablelistmodel.h"
#include "preferences/qrksettings.h"

CheckableListModel::CheckableListModel(QObject *parent):
    QStandardItemModel(parent)
{
}

QVariant CheckableListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role == Qt::BackgroundColorRole)
        return (itemFromIndex(index)->checkState() == Qt::Checked)? QColor("#ffffb2") : QColor("#ffffff");

    return QStandardItemModel::data(index, role);
}

QStringList CheckableListModel::getSelectedList(){
    QStringList list;

    int rows = rowCount();

    for (int i = 0; i < rows; i++) {
        QStandardItem *itm = item(i);
        if (itm->checkState() == Qt::Checked)
            list << itm->text();
    }

    return list;
}
