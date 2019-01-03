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

#include "filewatcher.h"
#include "importworker.h"
#include "singleton/spreadsignal.h"
#include "utils/utils.h"

#include <QDir>
#include <QCollator>
#include <QQueue>
#include <QThread>
#include <QDebug>

FileWatcher::FileWatcher (QWidget* parent)
    : QWidget(parent)
{
    m_isBlocked = false;
    m_queue.clear();
}

FileWatcher::~FileWatcher ()
{
}

void FileWatcher::directoryChanged(const QString &path)
{

    if(m_isBlocked) {
        // Remove old Importfiles
        QDir dir(path);
        dir.setNameFilters(QStringList() << "*.false*" << "*.old*");
        dir.setFilter(QDir::Files);
        QStringList list = dir.entryList();
        while(list.size() > 0){
            QString f = list.takeFirst();
            if (QFileInfo(dir.absoluteFilePath(f)).lastModified() < QDateTime::currentDateTime().addDays(-7)) {
                QFile::remove(dir.absoluteFilePath(f));
                qInfo() << "Function Name: " << Q_FUNC_INFO << " Remove file older than 7 Days FileName: " << f;
            }
        }
        return;
    }

    m_isBlocked = true;

    if (!m_watchingPathList.contains(path))
        m_watchingPathList.append(path);

    QDir directory;
    directory.setFilter(QDir::Files | QDir::NoSymLinks);
    directory.setSorting(QDir::NoSort);  // will sort manually with std::sort

    directory.setPath(path);
    QStringList filter;
    filter.append("*.json");
    QStringList fileList;
    fileList.clear();
    fileList = directory.entryList(filter);

    std::sort(
        fileList.begin(),
        fileList.end(), Utils::compareNames);

    foreach (const QString str, fileList) {
        QFile f(path + "/" + str);
        if (!m_queue.contains(path + "/" + str) && f.exists()) {

            m_queue.append(path + "/" + str);
        }

        if (m_queue.size() > 50)
            break;
    }

    if (m_isBlocked && !m_queue.isEmpty())
        start();
    else
        m_isBlocked = false;
}

void FileWatcher::removeDirectories()
{
    m_watchingPathList.clear();
    if (m_queue.isEmpty())
        emit workerStopped();
}

void FileWatcher::finished()
{

    m_isBlocked = false;
    emit workerStopped();
    foreach (const QString str, m_watchingPathList) {
        emit directoryChanged(str);
    }
}

void FileWatcher::start()
{
    qDebug() << "Function Name: " << Q_FUNC_INFO << " From main thread: " << QThread::currentThread();

    QThread *thread = new QThread;
    thread->sleep(1); // Sometimes the File was not finished
    m_worker = new ImportWorker(m_queue);
    m_worker->clear();
    m_worker->moveToThread(thread);

    connect(thread, &QThread::started, m_worker, &ImportWorker::process);
    connect(this, &FileWatcher::stopWorker, m_worker, &ImportWorker::stopProcess, Qt::DirectConnection);
    connect(m_worker, &ImportWorker::finished, this, &FileWatcher::finished);
    connect(m_worker, &ImportWorker::finished, thread, &QThread::quit);
    connect(m_worker, &ImportWorker::finished, m_worker, &ImportWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
}
