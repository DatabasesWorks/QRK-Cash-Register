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
*/

#include "backup.h"
#include "JlCompress.h"
#include "preferences/qrksettings.h"

#include <QStandardPaths>
#include <QApplication>
#include <QProcess>
#include <QDebug>

void Backup::create()
{
    QrkSettings settings;
    QString dataDir = settings.value("sqliteDataDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data").toString();
    create(dataDir);
}

void Backup::create(QString dataDir)
{

    QrkSettings settings;
    QString backupDir = settings.value("backupDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backup").toString();
    QString confname = qApp->property("configuration").toString();
    if (!confname.isEmpty())
        confname = "_" + confname;

    QString infile = QString("%1/%2-QRK%3.db").arg(dataDir).arg(QDate::currentDate().year()).arg(confname);
    QString outfile = QString("%1/data_%2.zip").arg(backupDir).arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));

    bool ok = JlCompress::compressFile( outfile, infile );
    if (!ok)
        qWarning() << "Function Name: " << Q_FUNC_INFO << " JlCompress::compressFile:" << ok;

    removeOldestFiles();

}

void Backup::restore(QString filename, bool restart)
{
    QrkSettings settings;
    QString dataDir = settings.value("sqliteDataDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data").toString();
    QString backupDir = settings.value("backupDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/backup").toString();

    QString zipfile = QString("%1/%2").arg(backupDir).arg(filename);
    QStringList files = JlCompress::getFileList(zipfile);
    files = JlCompress::extractFiles(zipfile, files, dataDir);

    if (files.isEmpty()) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " JlCompress::extractFiles: none, filename: " << filename << " zipfile: " << zipfile;
    } else if (restart) {
        // Spawn a new instance of myApplication:
        QString app = QApplication::applicationFilePath();
        QStringList arguments = QApplication::arguments();
        arguments << "-r";
        QString wd = QDir::currentPath();
        QProcess::startDetached(app, arguments, wd);
        QApplication::exit();
    }
}

void Backup::pakLogFile()
{

    QrkSettings settings;
    QString confname = qApp->property("configuration").toString();
    if (!confname.isEmpty())
        confname = "_" + confname;

    QString backupDir = QDir(settings.value("backupDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString()).absolutePath();
    QString infile = QString("%1").arg(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QString("/qrk.log").arg(confname));
    QString outfile = QString("%1/qrk%2_log_%3.zip").arg(backupDir).arg(confname).arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));

    bool ok = JlCompress::compressFile( outfile, infile );
    if (!ok) {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " JlCompress::compressFile:" << ok;
        return;
    }

    QFile(infile).remove();
}

void Backup::removeOldestFiles()
{
    QrkSettings settings;
    int keep = settings.value("keepMaxBackups", -1).toInt();
    if (keep == -1)
        return;

    QString backupDir = QDir(settings.value("backupDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString()).absolutePath();

    QDir dir(backupDir);
    dir.setNameFilters(QStringList() << "*.zip");
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Time | QDir::Reversed);
    QStringList list = dir.entryList();
    while(list.size() > keep){
        QString f = list.takeFirst();
        QFile::remove(dir.absoluteFilePath(f));
        qInfo() << "Function Name: " << Q_FUNC_INFO << " Remove old backup FileName: " << f;
    }
}

bool Backup::removeDir(const QString & dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}
