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
 * ckvtemplate use part of code from NLTemplate (c) 2013 tom@catnapgames.com
 * wich is MIT Licensed https://github.com/catnapgames/NLTemplate
 * and some code from Arithmetic Expression Example
 * Solution: Kristijan Burnik, Udruga informatičara Božo Težak
 *             GMail: kristijanburnik
 *
 * both are modified for QT5 use by chris@ckvsoft.at
 */

#include "ckvtemplate.h"

#include <QDate>
#include <QTime>
#include <QDebug>

ckvTemplate::ckvTemplate()
{
}

void ckvTemplate::replace(QString &name)
{
    QString replacement = find(name);

    if (replacement.isEmpty())
        return;

    name = checkForDate(replacement);
}

QString ckvTemplate::checkForDate(QString name)
{
    QString check = name.mid(0, name.lastIndexOf(' '));

    if (QDate::fromString(check).isValid()) {
        bool month = false;
        if (name.toUpper().endsWith('M')) {
            month = true;
            name.chop(1);
        }

        int d = name.mid(name.lastIndexOf(' ')).toInt();
        if (month)
            name = QDate::fromString(check).addMonths(d).toString();
        else
            name = QDate::fromString(check).addDays(d).toString();
    } else if (QTime::fromString(check).isValid()) {
        name = check;
    }

    return name;
}

QString ckvTemplate::process(QString tpl)
{
    QStringMatcher in("{{");
    QStringMatcher out("}}");
    int position = 0;
    int next = 0;

    QString str;
    QString code;

    bool inside = false;

    while (position < tpl.size()) {
        if (!inside) {
            next = in.indexIn(tpl, position);
            if (next == -1) next = tpl.size();
            str += tpl.mid(position, next - position);
            position = next + 2;
            inside = true;
        } else {
            next = out.indexIn(tpl, position);
            Q_ASSERT(next != -1);
            code += tpl.mid(position, next - position);
            replace(code);
            str.append(calculate(code));
            code.clear();

            position = next + 2;

            inside = false;
        }
    }
    return str;
}

QString ckvTemplate::calculate(QString s)
{
    QList<QPair<int, QString> > calc = derive(tokenize(s));
    if (calc.size() == 0)
        return s;

    QPair<int, QString> a, b, op, result;
    int atback = 0;
    while (calc.size() > 1) {
        // obtain first three tokens ( hope for A B OP respectivley )
        a = calc[0];    // first operand
        b = calc[1];    // second operand
        op = calc[2];   // operation

        // Check if A, B and OP correspond to their expected token type:
        // Priority handling: keep waiting operands at end of dequeue if needed
        if (op.first != OPERATION) {
            // move the waiting operand to end of dequeue, and skip this round
            calc.push_back(a);
            calc.pop_front();

            // keep track of the waiting operands: will restore them after calc
            atback++;
        } else {
            // ready for calculating result, remove the front A, B and OP
            calc.pop_front(); calc.pop_front(); calc.pop_front();

            // the essence of the calculation is done here
            if (op.second == "*") {
                result.second = QString::number(a.second.toDouble() * b.second.toDouble(), 'f', 2);
            } else if (op.second == "+") {
                result.second = QString::number(a.second.toDouble() + b.second.toDouble(), 'f', 2);
            } else if (op.second == "-") {
                result.second = QString::number(a.second.toDouble() - b.second.toDouble(), 'f', 2);
            } else {
                if (b.second.toDouble() == 0.0) {
                    qWarning() << "Division by zero!";
                    break;
                }
                if (op.second == "/")
                    result.second = QString::number(a.second.toDouble() / b.second.toDouble(), 'f', 2);
                else if (op.second == "%")
                    result.second = QString::number(a.second.toInt() % b.second.toInt());
            }

            calc.push_front(result);

            // restore the waiting operands from back to front
            while (atback-- > 0) {
                calc.push_front(calc.back());
                calc.pop_back();
            }
            atback = 0;
        }
    }
    if (result.second.isEmpty())
        return s;

    return result.second;
}

QVector<QPair<int, QString> > ckvTemplate::tokenize(QString s)
{
    // add sentinel operation ~
    s.append("~");

    bool found_numbers = false;
    QVector<QPair<int, QString> > tokens;
    QString buffer;
    Types oldType = OTHER;
    for (int i = 0; i < s.size(); i++) {
        QChar c = s[i];
        Types type = getType(c);
        switch (type) {
        case NUMBER:
            buffer.append(c);
            found_numbers = true;
            break;
        case OTHER:
            if (c == '.' || c == ',')
                buffer.append('.');
            break;

        case OPERATION:
            if (!found_numbers) {
                qWarning() << "Empty expression!";
                break;
            }
            if (oldType == OTHER)
                continue;

            if (buffer.size() > 0) {
                tokens.push_back(qMakePair(NUMBER, buffer));
                buffer = "";
            }
            tokens.push_back(qMakePair(OPERATION, s.mid(i, 1)));

            break;
        case SPACE:
            break;
        }
        oldType = type;
        int ts = tokens.size();
        if (ts > 1 && tokens[ts - 1].first == tokens[ts - 2].first) {
            qWarning() << "Invalid expression!";
            break;
        }
    }

    // remove the sentinel operation ~
    if (tokens.size() > 0)
        tokens.pop_back();
    return tokens;
}

QList<QPair<int, QString> > ckvTemplate::derive(QVector<QPair<int, QString> > tokens)
{
    QList<QPair<int, QString> > calc, ops;
    QString last_op;
    for (int i = 0; i < tokens.size(); i++) {
        calc.push_back(tokens[i]);
        int cs = calc.size();

        switch (tokens[i].first) {
        case NUMBER:
            if (last_op != "") {
                calc.swap(cs - 1, cs - 2);
                last_op = "";
            }
            break;
        case OPERATION:
            last_op = tokens[i].second;
            ops.push_back(tokens[i]);
            break;
        }

        int os = ops.size();

        // handle priority if needed
        if (os > 1 && tokens[i].first == NUMBER) {
            if (greaterPriority(ops[os - 1], ops[os - 2])) {
                calc.swap(cs - 3, cs - 2);
                calc.swap(cs - 2, cs - 1);
                ops.swap(os - 1, os - 2);
            }
        }
    }
    return calc;
}

ckvTemplate::Types ckvTemplate::getType(QChar c)
{
    if (c.isDigit()) return NUMBER;
    if (c == ' ') return SPACE;
    if (c == '*' || c == '+' || c == '/' || c == '%' || c == '-') return OPERATION;
    if (c == '~') return OPERATION; // sentinel
    return OTHER;
}

bool ckvTemplate::greaterPriority(QPair<int, QString> a, QPair<int, QString> b)
{
    return (a.second == "*" || a.second == "/" || a.second == "%") && (b.second == "+" || b.second == "-");
}

const QString Dictionary::find(const QString & name) const
{
    QStringList list = name.split(" ");

    QVectorIterator<QPair<QString, QString> > i(properties);
    while (i.hasNext()) {
        QPair<QString, QString> prop = i.next();
        // foreach (QPair<QString, QString> const &prop, properties) {
        if (list.contains(prop.first)) {
            for (int i = 0; i < list.size(); i++) {
                if (prop.first == list[i]) {
                    list[i] = prop.second;
                    return list.join(" ");
                }
            }
        }
    }
    return "";
}

void Dictionary::set(const QString & name, const QString & value)
{
    QVectorIterator<QPair<QString, QString> > i(properties);
    while (i.hasNext()) {
        QPair<QString, QString> prop = i.next();
        //    foreach (QPair<QString, QString> &prop, properties) {
        if (prop.first == name) {
            prop.second = value;
            return;
        }
    }
    properties.push_back(QPair<QString, QString>(name, value));
}
