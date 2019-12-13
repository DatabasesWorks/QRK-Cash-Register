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

#ifndef QSQLRTMODEL_H
#define QSQLRTMODEL_H

#include "globals_ckvsoft.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlRelationalTableModel>

class CKVSOFT_EXPORT QSqlRTModel: public QSqlRelationalTableModel
{
    Q_OBJECT

public:
    explicit QSqlRTModel(QObject *_parent = Q_NULLPTR, QSqlDatabase _db = QSqlDatabase()): QSqlRelationalTableModel(_parent, _db) {}
    void setFilter(const QString & filter)
    {
        QSqlRelationalTableModel::setFilter(QString(filter).replace("WHERE", "", Qt::CaseInsensitive));
    }
    void setJoin(const QString & __join)
    {
        p_join = __join;
    }
    void setFrom(const QString & __from)
    {
        p_from = __from;
    }

    QString join() const
    {
        QStringList list = p_join.split("JOIN ", QString::KeepEmptyParts, Qt::CaseInsensitive);
        return list.at(1);
    }

    QString from() const
    {
        QStringList list = p_join.split("FROM ", QString::KeepEmptyParts, Qt::CaseInsensitive);
        return list.at(1);
    }

protected:
    QString selectStatement() const
    {
        const QString statement = QSqlRelationalTableModel::selectStatement().replace("SELECT", "SELECT DISTINCT", Qt::CaseInsensitive).replace("FROM " + tableName(), p_from, Qt::CaseInsensitive);
        QStringList list = statement.split("WHERE ", QString::KeepEmptyParts, Qt::CaseInsensitive);
        list.insert(1, p_join);
        list.insert(2, "WHERE");
        return list.join(" ");
    }
private:
    QString p_join;
    QString p_from;
};
#endif // QSQLRTMODEL_H
