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

#ifndef JSONTABLEMODEL_H
#define JSONTABLEMODEL_H

#include "globals_ckvsoft.h"

#include <QObject>
#include <QVector>
#include <QMap>
#include <QAbstractTableModel>
#include <QJsonDocument>
#include <QJsonArray>

class CKVSOFT_EXPORT QJsonTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    typedef QMap<QString,QString> Heading;
    typedef QVector<Heading> Header;
    QJsonTableModel(const Header& header, QObject * parent = Q_NULLPTR);

    bool setJson(const QJsonDocument &json);
    bool setJson(const QJsonArray &array);

    virtual QJsonObject getJsonObject(const QModelIndex &index) const;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    Qt::ItemFlags flags (const QModelIndex &index ) const;
    QJsonObject findValueFromJsonArray(const QString &key, const QString &val);
    QJsonObject findValueFromJsonArray(const QString &key, int val);
    void enableColumnEdit(int col, bool enable);

private:
    Header m_header;
    QJsonArray m_json;
    QList<int> m_columsEditable;
};

#endif // JSONTABLEMODEL_H
