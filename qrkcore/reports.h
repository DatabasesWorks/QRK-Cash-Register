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

#ifndef REPORTS_H
#define REPORTS_H

#include "journal.h"
#include "receiptitemmodel.h"
#include "qrkcore_global.h"

class QRK_EXPORT Reports : public ReceiptItemModel
{
    Q_OBJECT
  public:
    Reports(QObject *parent = Q_NULLPTR, bool servermode = false);
    ~Reports();

    bool mustDoEOAny(QDateTime check);
    bool checkEOAny(QDateTime checkDate = QDateTime::currentDateTime(), bool checkDay = true);
//    bool checkEOAnyServerMode();

    bool endOfDay();
    bool endOfDay(bool ask);
    bool endOfMonth();

    static QString getReport(int id, bool report_by_productgroup = false, bool test = false);

  private:
    bool checkEOAnyMessageBoxYesNo(int type, QDateTime datetime, QString text = "");
    void checkEOAnyMessageBoxInfo(int type, QDateTime datetime, QString text);
    bool canCreateEOD(QDateTime);
    bool canCreateEOM(QDateTime);
    int getReportType();
    bool nullReceipt(QDate date);
    QDateTime getLastEODateTime();
    QMap<int, QDateTime> getEOFMap(QDateTime checkDate = QDateTime::currentDateTime());

    qint64 getDiffTime(QDateTime dateTime, bool old = false);
    qint64 getDiffTime(QDateTime dateTime, QTime curfew);

    bool createEOD(int, QDateTime);
    bool createEOM(int, QDateTime);
    bool insert(QStringList, int, QDateTime, QDateTime);

    QStringList createStat(int, QString, QDateTime, QDateTime);
    QStringList createYearStat(int, QDate);
    void printDocument(int id, QString title);

    bool doEndOfDay(QDateTime datetime);
    bool doEndOfMonth(QDateTime datetime);

    QString m_yearsales;
    int m_currentReceipt = 0;
    bool m_servermode;

};

#endif // REPORTS_H
