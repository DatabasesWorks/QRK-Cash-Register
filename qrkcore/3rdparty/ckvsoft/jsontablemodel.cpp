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

#include "jsontablemodel.h"

#include <QJsonObject>

QJsonTableModel::QJsonTableModel(const QJsonTableModel::Header &header, QObject *parent )
    : QAbstractTableModel(parent), m_header(header)
{
}

bool QJsonTableModel::setJson(const QJsonDocument &json)
{
    return setJson( json.array() );
}

bool QJsonTableModel::setJson( const QJsonArray &array )
{
    beginResetModel();
    m_json = array;
    endResetModel();
    return true;
}

QVariant QJsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if( role != Qt::DisplayRole ) {
        return QVariant();
    }

    switch( orientation ) {
    case Qt::Horizontal:
        return m_header[section]["title"];
    case Qt::Vertical:
        //return section + 1;
        return QVariant();
    }
    return QVariant();
}

int QJsonTableModel::rowCount(const QModelIndex & /* parent */ ) const
{
    return m_json.size();
}

int QJsonTableModel::columnCount(const QModelIndex & /* parent */ ) const
{
    return m_header.size();
}


QJsonObject QJsonTableModel::getJsonObject( const QModelIndex &index ) const
{
    const QJsonValue& value = m_json[index.row() ];
    return value.toObject();
}

QVariant QJsonTableModel::data( const QModelIndex &index, int role ) const
{
    switch(role) {
    case Qt::DisplayRole: {
        QJsonObject obj = getJsonObject(index);
        const QString& key = m_header[index.column()]["index"];
        if( obj.contains(key)) {
            QJsonValue v = obj[ key ];
            if( v.isString() ) {
                return v.toString();
            } else if( v.isDouble() ) {
                return QString::number(v.toDouble());
            } else {
                return QVariant();
            }
        } else {
            return QVariant();
        }
    }
    case Qt::ToolTipRole:
        return QVariant();
    default:
        return QVariant();
    }
}

bool QJsonTableModel::setData(const QModelIndex &index, const QVariant &value, int /* role*/ )
{
    const QString& key = m_header[index.column()]["index"];
    QJsonObject obj = m_json[index.row()].toObject();
    obj[key] = value.toString();
    m_json.replace(index.row(),obj);
    return true;
}

Qt::ItemFlags QJsonTableModel::flags (const QModelIndex &index) const
{
    if (m_columsEditable.contains(index.column()))
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
    else
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void QJsonTableModel::enableColumnEdit(int col, bool enable)
{
    if (m_columsEditable.contains(col) && !enable) {
        m_columsEditable.removeAll(col);
    } else if (enable) {
       m_columsEditable.append(col);
    }
}

QJsonObject QJsonTableModel::findValueFromJsonArray(const QString &key, int val)
{
    for (const auto obj : m_json) {
        if (obj.toObject().value(key) == val)
            return obj.toObject();
    }
    return QJsonObject();
}

QJsonObject QJsonTableModel::findValueFromJsonArray(const QString &key, const QString &val)
{
    for (const auto obj : m_json) {
        if (obj.toObject().value(key).toString().contains(val, Qt::CaseSensitivity::CaseInsensitive))
            return obj.toObject();
    }
    return QJsonObject();
}
