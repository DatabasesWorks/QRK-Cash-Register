/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2018 Christian Kvasny <chris@ckvsoft.at>
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
 *  * ckvtemplate use part of code from NLTemplate (c) 2013 tom@catnapgames.com
 * wich is MIT Licensed https://github.com/catnapgames/NLTemplate
 * and some code from Arithmetic Expression Example
 * Solution: Kristijan Burnik, Udruga informatičara Božo Težak
 *             GMail: kristijanburnik
 *
 * both are modified for QT5 use by chris@ckvsoft.at
 *
*/

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "globals_ckvsoft.h"

#include <QObject>
#include <QPair>
#include <QVector>
#include <QTextStream>

class CKVSOFT_EXPORT Dictionary
{

    public:
        const QString find( const QString & name ) const;
        void set( const QString & name, const QString & value );

    protected:
        QVector<QPair<QString, QString> > properties;
};

class CKVSOFT_EXPORT ckvTemplate : public Dictionary
{

    public:
        ckvTemplate();
        QString process(QString tpl);

    private:
        enum Types {
            NUMBER = 0,
            OPERATION,
            SPACE,
            OTHER
        };

        Types getType(QChar c);
        bool greaterPriority(QPair<int, QString> a, QPair<int, QString> b);
        QVector<QPair <int, QString> > tokenize(QString s);
        QList<QPair <int, QString> > derive(QVector <QPair<int, QString> > tokens);
        QString calculate(QString s);
        QString checkForDate(QString name);
        void replace(QString &name);
};

#endif // TEMPLATE_H
