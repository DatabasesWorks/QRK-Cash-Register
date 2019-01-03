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

#include "exportjournal.h"
#include "exportdialog.h"
#include "database.h"
#include "singleton/spreadsignal.h"
#include "3rdparty/ckvsoft/rbac/crypto.h"

#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QFile>
#include <QMessageBox>

#include <QDebug>

ExportJournal::ExportJournal(QWidget *parent)
    : QDialog(parent)
{
}

ExportJournal::~ExportJournal()
{
   Spread::Instance()->setProgressBarValue(-1);
}

void ExportJournal::Export()
{
    ExportDialog dlg(false);
    if (dlg.exec() == QDialog::Accepted ) {
        QString filename = dlg.getFilename();
        if (journalExport(filename, dlg.getFrom(), dlg.getTo())) {
            QMessageBox::information(0, tr("Export"), tr("Journal wurde nach %1 exportiert.").arg(filename));
        } else {
            QMessageBox::warning(0, tr("Export"), tr("Journal konnte nicht nach %1 exportiert werden.\nÜberprüfen Sie bitte Ihre Schreibberechtigung.").arg(filename));
        }
    }
}

bool ExportJournal::journalExport(QString outputFilename, QString from, QString to)
{

    QFile outputFile(outputFilename);
    // Try and open a file for output
    outputFile.open(QIODevice::WriteOnly | QIODevice::Text);

    // Check it opened OK
    if(!outputFile.isOpen()){
        qWarning() << "Function Name: " << Q_FUNC_INFO << " Error, unable to open" << outputFile.fileName() << "for output";
        return false;
    }

    // Point a QTextStream object at the file
    QTextStream outStream(&outputFile);

    QSqlDatabase dbc = Database::database();
    QSqlQuery query(dbc);

    query.prepare(QString("SELECT data FROM journal WHERE id < 5"));
    query.exec();
    while (query.next())
    {
        // Write the line to the file
        QString data = query.value("data").toString();
        data = Crypto::decrypt(data, SecureByteArray("Journal")).replace("\t", ";");
        outStream << data + '\n';
    }

    query.prepare(QString("SELECT version, cashregisterid, data FROM journal WHERE datetime BETWEEN :fromDate AND :toDate AND id > 4"));
    query.bindValue(":fromDate", from);
    query.bindValue(":toDate", to);

    query.exec();
    int i = 0;
    int numberOfRows = 0;
    if(query.last())
    {
        numberOfRows =  query.at() + 1;
        query.first();
        query.previous();
    }

    while (query.next())
    {
        i++;
        Spread::Instance()->setProgressBarValue(((float)i / (float)numberOfRows) * 100);
        QString data = query.value("data").toString();
        data = Crypto::decrypt(data, SecureByteArray("Journal"));
        QStringList datalist = data.split('\t');

        int j = 0;
        foreach (const QString &str, datalist) {
            datalist[j] = QString("\"%1\"").arg(str);
            j++;
        }
        data = datalist.join(';');

        QString s = QString("%1;%2;%3;%4\n").arg(i).arg(query.value("version").toString()).arg(query.value("cashregisterid").toString()).arg(data);
        outStream << s;
    }
    outStream.flush();
    // Close the file
    outputFile.close();
    Spread::Instance()->setProgressBarValue(-1);
    return true;
}
