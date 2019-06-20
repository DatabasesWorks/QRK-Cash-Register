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
#include "3rdparty/qbcmath/bcmath.h"
#include "singleton/spreadsignal.h"
#include "preferences/qrksettings.h"
#include "checkablelist/checkablelistdialog.h"

#include <QTextStream>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QFile>
#include <QMessageBox>
#include <QLocale>
#include <QTextCodec>
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
    settings.beginGroup("productsExport");

    QString lastUsedDirectory = settings.value("lastUsedDirectory", QDir::currentPath()).toString();
    if (settings.value("useDecimalQuantity", false).toBool())
        m_decimals = settings.value("decimalDigits", 2).toInt();

    QString selected = settings.value("lastUsedExport", "products.itemnum,products.name,products.gross,products.groupid,products.tax" ).toString();

    CheckableListDialog cd;
    cd.setModelData(Database::getDatabaseTableHeaderNames("products"), selected.split(','));
    if (cd.exec() != QDialog::Accepted)
        return;

    QString filename = QFileDialog::getSaveFileName(Q_NULLPTR, tr("Datei speichern"), lastUsedDirectory, "Artikel (*.csv)", Q_NULLPTR, QFileDialog::DontUseNativeDialog);

    settings.save2Settings("lastUsedDirectory", filename);

    if (!filename.isNull() && productsExport(filename, cd.getRequiredList())) {
        QMessageBox::information(Q_NULLPTR, tr("Export"), tr("Produkte wurden nach %1 exportiert.").arg(filename));
    } else {
        QMessageBox::warning(Q_NULLPTR, tr("Export"), tr("Produkte konnten nicht nach %1 exportiert werden.\nÜberprüfen Sie bitte Ihre Schreibberechtigung.").arg(filename));
    }
    settings.save2Settings("lastUsedDirectory", QFileInfo(filename).absolutePath());
    settings.endGroup();
}

bool ExportProducts::productsExport(QString outputFilename, QStringList requiredlist)
{

    QTextCodec *codec = QTextCodec::codecForName ("UTF-8");

    QrkSettings settings;
    settings.beginGroup("productsExport");

    QString selectlist = requiredlist.join(',');
    settings.save2Settings("lastUsedExport", selectlist);
    settings.endGroup();

    selectlist.replace("products.groupid", "groups.name as \"groupname\"");

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

    query.prepare("SELECT " + selectlist + " FROM ((select max(version) as version, origin from products group by origin) p1 inner join (select * from products) as products on p1.version=products.version and p1.origin=products.origin) INNER JOIN groups where groupid = groups.id AND groupid > 1");

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
                if (static_cast<QMetaType::Type>(record.field(i).type()) == QMetaType::QString) {
                    outStream << "\"" << record.value(i).toString().replace("\"","\"\"") << "\"";
                } else if (static_cast<QMetaType::Type>(record.field(i).type()) == QMetaType::Double) {
                    QBCMath value(record.value(i).toString());
                    if ((record.fieldName(i) == "sold") || (record.fieldName(i) == "stock") || (record.fieldName(i) == "minstock")) {
                        value.round(m_decimals);
                        if (m_decimals == 0) {
                            outStream << value.toInt();
                        } else {
                            outStream << value.toLocale();
                        }
                    } else {
                        value.round(2);
                        outStream << value.toLocale();
                    }
                } else {
                    outStream << codec->toUnicode(record.value(i).toString().toUtf8());
                }
                if (i < count -1) outStream << ';';
            }
        }
    } else {
        qWarning() << "Function Name: " << Q_FUNC_INFO << query.lastError().text();
        qDebug() << "Function Name: " << Q_FUNC_INFO << Database::getLastExecutedQuery(query);
    }
    outputFile.close();
    Spread::Instance()->setProgressBarValue(-1);
    return true;
}
