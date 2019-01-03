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

#include "exportproducts.h"
#include "database.h"
#include "singleton/spreadsignal.h"
#include "preferences/qrksettings.h"

#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>

#include <QDebug>

ExportProducts::ExportProducts(QWidget *parent)
    : QDialog(parent)
{
}

ExportProducts::~ExportProducts()
{
   Spread::Instance()->setProgressBarValue(-1);
}

void ExportProducts::Export()
{
    QrkSettings settings;
    QString lastUsedDirectory = settings.value("lastUsedDirectory", QDir::currentPath()).toString();

    QString filename = QFileDialog::getSaveFileName(0, tr("Datei speichern"), lastUsedDirectory, "Artikel (*.csv)", 0, QFileDialog::DontUseNativeDialog);

    settings.save2Settings("lastUsedDirectory", filename);

    if (!filename.isNull() && productsExport(filename)) {
        QMessageBox::information(0, tr("Export"), tr("Produkte wurden nach %1 exportiert.").arg(filename));
    } else {
        QMessageBox::warning(0, tr("Export"), tr("Produkte konnten nicht nach %1 exportiert werden.\nÜberprüfen Sie bitte Ihre Schreibberechtigung.").arg(filename));
    }
}

bool ExportProducts::productsExport(QString outputFilename)
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
    bool firstLine = true;

    query.prepare(QString("SELECT * FROM products WHERE \"group\" > 1"));
    if(query.exec()){
        while (query.next()) {
            const QSqlRecord record= query.record();
            int count = record.count();
            if(firstLine){
                for(int i=0;i<count;++i) {
                    outStream << record.fieldName(i); //Headers
                    if (i < count -1) outStream << ';';
                }
            }
            firstLine=false;
            outStream << endl;
            for(int i=0;i<count;++i) {
                if (static_cast<QMetaType::Type>(record.field(i).type()) == QMetaType::QString)
                    outStream << "\"" << record.value(i).toString() << "\"";
                else
                    outStream << record.value(i).toString();
                if (i < count -1) outStream << ';';
            }
        }
    }
    outputFile.close();
    Spread::Instance()->setProgressBarValue(-1);
    return true;
}
