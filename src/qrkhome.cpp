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

#include "defines.h"
#include "qrkhome.h"
#include "import/filewatcher.h"
#include "singleton/spreadsignal.h"
#include "qrkprogress.h"
#include "preferences/settingsdialog.h"
#include "preferences/qrksettings.h"
#include "database.h"
#include "utils/utils.h"
#include "salesinfo.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "qrkpushbutton.h"
#include "ui_qrkhome.h"

#include <QMessageBox>
#include <QDesktopWidget>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>

#include <QDebug>

QRKHome::QRKHome(bool servermode, QWidget *parent)
    : QWidget(parent),ui(new Ui::QRKHome), m_menu(Q_NULLPTR)
{
    connect(Spread::Instance(), &SpreadSignal::updateSafetyDevice, this, &QRKHome::safetyDevice);
    connect(RBAC::Instance(), static_cast<void(Acl::*)()>(&Acl::userChanged), this, &QRKHome::init);

    ui->setupUi(this);

    ui->signatureDamagedLabel->setVisible(false);

    if ( QApplication::desktop()->width() < 1200 )
    {
        ui->documentButton->setMinimumWidth(0);
        ui->managerButton->setMinimumWidth(0);
        ui->registerButton->setMinimumWidth(0);
        ui->taskButton->setMinimumWidth(0);
    }

    // create the menu popup
    {
        m_menu = new QFrame(this, Qt::Popup /* Qt::FramelessWindowHint | Qt::Tool */);
        m_menu->setFrameStyle(QFrame::StyledPanel);
        m_menu->hide();
        QVBoxLayout *vbox = new QVBoxLayout(m_menu);

        QrkPushButton *b;
        b = new QrkPushButton(QIcon(":src/icons/exit.png"), tr("Beenden"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::exitButton_clicked);
        vbox->addWidget(b);

        b = new QrkPushButton(QIcon(":src/icons/view-fullscreen.png"), tr("Vollbild"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::fullScreenButton_clicked);
        vbox->addWidget(b);

        b = new QrkPushButton(QIcon(":src/icons/settings.png"), tr("Einstellungen"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::settingsSlot);
        vbox->addWidget(b);

        connect(ui->menuButton, &QrkPushButton::clicked, this, &QRKHome::menuSlot);

    }

    // create the manager popup
    {
        m_manager = new QFrame(this, Qt::Popup);
        m_manager->setFrameStyle(QFrame::StyledPanel);
        m_manager->hide();
        QVBoxLayout *vbox = new QVBoxLayout(m_manager);

        QrkPushButton *b;
        b = new QrkPushButton(QIcon(":src/icons/products.png"), tr("Artikel Manager"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::productmanagerButton_clicked);
        vbox->addWidget(b);

        b = new QrkPushButton(QIcon(":src/icons/useradmin.png"), tr("Benutzer Manager"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::usermanagerButton_clicked);
        vbox->addWidget(b);

        connect(ui->managerButton, &QrkPushButton::clicked, this, &QRKHome::managerSlot);
    }

    // create the task popup
    {
        m_task = new QFrame(this, Qt::Popup/* Qt::FramelessWindowHint | Qt::Tool */ );
        m_task->setFrameStyle(QFrame::StyledPanel);
        m_task->hide();
        QVBoxLayout *vbox = new QVBoxLayout(m_task);

        QrkPushButton *b;
        b = new QrkPushButton(QIcon(":src/icons/day.png"), tr("Tagesabschluss"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::endOfDay);
        vbox->addWidget(b);

        b = new QrkPushButton(QIcon(":src/icons/month.png"), tr("Monatsabschluss"));
        b->setStyleSheet("Text-align:left");
        connect(b, &QrkPushButton::clicked, this, &QRKHome::endOfMonth);
        vbox->addWidget(b);

        connect(ui->taskButton, &QrkPushButton::clicked, this, &QRKHome::taskSlot);
    }

    connect(ui->registerButton, &QPushButton::clicked, this, &QRKHome::registerButton_clicked);
    connect(ui->documentButton, &QPushButton::clicked, this, &QRKHome::documentButton_clicked);

    connect(ui->serverModeCheckBox, &QCheckBox::clicked, this, &QRKHome::serverModeCheckBox_clicked);
    connect(Spread::Instance(), &SpreadSignal::updateImportInfo, this, &QRKHome::importInfo);

    connect(ui->dayPushButton, &QPushButton::clicked, this, &QRKHome::dayPushButton_clicked);
    connect(ui->monthPushButton, &QPushButton::clicked, this, &QRKHome::monthPushButton_clicked);
    connect(ui->yearPushButton, &QPushButton::clicked, this, &QRKHome::yearPushButton_clicked);
    connect(ui->logOnOffButton, &QPushButton::clicked, this, &QRKHome::logOnOff);

    ui->importWidget->setVisible(true);
    setMinimumHeight(400);

    m_fw = new FileWatcher;
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, m_fw, &FileWatcher::directoryChanged);
    connect(m_fw, &FileWatcher::apport, this, &QRKHome::serverModeCheckBox_clicked);

    ui->serverModeCheckBox->setChecked(servermode);

    ui->dataBaseLabel->setText(Database::getDatabaseVersion());

}

QRKHome::~QRKHome()
{
    delete ui;
    delete m_fw;
}

void QRKHome::dayPushButton_clicked(bool)
{
    // ---------- DAY -----------------------------
    SalesInfo info(SALESINFO_DAY);
    info.exec();
}

void QRKHome::monthPushButton_clicked(bool)
{
    // ---------- MONTH -----------------------------
    SalesInfo info(SALESINFO_MONTH);
    info.exec();
}

void QRKHome::yearPushButton_clicked(bool)
{
    // ----------- YEAR ---------------------------
    SalesInfo info(SALESINFO_YEAR);
    info.exec();
}

void QRKHome::safetyDevice(bool active)
{
    if (active != m_previousSafetyDeviceState) {
        m_previousSafetyDeviceState = active;
        if (!active) {
            ui->signatureDamagedLabel->setVisible(true);
            importInfo(tr("Signaturerstellungseinrichtung ausgefallen!"), true);
            if (isServerMode()) {
                serverModeCheckBox_clicked(false);
                serverCriticalMessageBox(QObject::tr("Signaturerstellungseinrichtung"), QObject::tr("ACHTUNG! Signaturerstellungseinrichtung ausgefallen!\nKontaktieren Sie Ihren Administrator wenn dieses Problem weiterhin auftritt."));
            }
        } else {
            ui->signatureDamagedLabel->setVisible(false);
        }
    }
}

void QRKHome::setExternalDepLabels(bool visible)
{
    ui->externalDepDescriptionLabel->setVisible(visible);
    ui->externalDepDirIconLabel->setVisible(visible);
    ui->externalDepDirlabel->setVisible(visible);
}

void QRKHome::init()
{
    QrkSettings settings;
    ui->userInfoFrame->setVisible(RBAC::Instance()->Login());

    QPixmap pixmapOK;
    QPixmap pixmapNotOK;
    pixmapOK.load(":src/icons/ok.png");
    pixmapNotOK.load(":src/icons/cancel.png");

    qint64 size = 0;
    qint64 bytesAvailable = 0;
    double percent = 1.0;

    if (settings.value("backupDirectory").toString().isEmpty()){
        ui->backupDirIconLabel->setPixmap(pixmapNotOK);
        ui->backupDirLabel->setText(tr("n/a"));
        ui->backupDirLabel->setStyleSheet("background: rgba(22, 86, 232, 0.5)");
    } else {
        QString backupDir = settings.value("backupDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString();
        if (!Utils::isDirectoryWritable(backupDir))
            ui->backupDirIconLabel->setPixmap(pixmapNotOK);
        else
            ui->backupDirIconLabel->setPixmap(pixmapOK);
        ui->backupDirLabel->setToolTip(backupDir);
        ui->backupDirLabel->setText(backupDir);
        Utils::diskSpace(backupDir, size, bytesAvailable, percent);

        ui->backupDirLabel->setStyleSheet(QString("background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(22, 86, 232, 1), stop:%1 rgba(22, 86, 232, 0.5), stop:%2 rgba(111, 111, 111, 0.5));").arg(percent).arg(percent + 0.1));
        ui->backupDirLabel->setToolTip(QString("Freier Speicher: %1 GB / Gesamtspeicher: %2 GB").arg(bytesAvailable / 1024).arg(size / 1024));
    }

    QString dataDir = settings.value("sqliteDataDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/data").toString();
    if (!Utils::isDirectoryWritable(dataDir)) {
        ui->dataDirIconLabel->setPixmap(pixmapNotOK);
        ui->dataDirlabel->setStyleSheet("background: rgba(22, 86, 232, 0.5)");
    } else {
        ui->dataDirIconLabel->setPixmap(pixmapOK);
        Utils::diskSpace(dataDir, size, bytesAvailable, percent);
        ui->dataDirlabel->setStyleSheet(QString("background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(22, 86, 232, 1), stop:%1 rgba(22, 86, 232, 0.5), stop:%2 rgba(111, 111, 111, 0.5));").arg(percent).arg(percent + 0.1));
    }
    ui->dataDirlabel->setToolTip(dataDir);
    ui->dataDirlabel->setText(dataDir);
    ui->dataDirlabel->setToolTip(QString("Freier Speicher: %1 GB / Gesamtspeicher: %2 GB").arg(bytesAvailable / 1024).arg(size / 1024));

    if (settings.value("pdfDirectory").toString().isEmpty()){
        ui->pdfDirIconLabel->setPixmap(pixmapNotOK);
        ui->pdfDirLabel->setText(tr("n/a"));
        ui->pdfDirLabel->setStyleSheet("background: rgba(22, 86, 232, 0.5)");
    } else {
        QString pdfDir = settings.value("pdfDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString();
        if (!Utils::isDirectoryWritable(pdfDir))
            ui->pdfDirIconLabel->setPixmap(pixmapNotOK);
        else
            ui->pdfDirIconLabel->setPixmap(pixmapOK);
        ui->pdfDirLabel->setToolTip(pdfDir);
        ui->pdfDirLabel->setText(pdfDir);
        Utils::diskSpace(pdfDir, size, bytesAvailable, percent);
        ui->pdfDirLabel->setStyleSheet(QString("background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(22, 86, 232, 1), stop:%1 rgba(22, 86, 232, 0.5), stop:%2 rgba(111, 111, 111, 0.5));").arg(percent).arg(percent + 0.1));
        ui->pdfDirLabel->setToolTip(QString("Freier Speicher: %1 GB / Gesamtspeicher: %2 GB").arg(bytesAvailable / 1024).arg(size / 1024));
    }

    if (settings.value("externalDepDirectory").toString().isEmpty()){
        ui->externalDepDirIconLabel->setPixmap(pixmapNotOK);
        ui->externalDepDirlabel->setText(tr("n/a"));
        ui->externalDepDirlabel->setStyleSheet("background: rgba(22, 86, 232, 0.5)");
    } else {
        QString externalDepDir = settings.value("externalDepDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString();
        if (!Utils::isDirectoryWritable(externalDepDir)) {
            ui->externalDepDirIconLabel->setPixmap(pixmapNotOK);
            ui->externalDepDirlabel->setStyleSheet("background: rgba(22, 86, 232, 0.5)");
            Utils::diskSpace(externalDepDir, size, bytesAvailable, percent);
            ui->externalDepDirlabel->setToolTip(QString("Freier Speicher: %1 GB / Gesamtspeicher: %2 GB").arg(bytesAvailable / 1024).arg(size / 1024));
        } else {
            ui->externalDepDirIconLabel->setPixmap(pixmapOK);
            Utils::diskSpace(externalDepDir, size, bytesAvailable, percent);
            ui->externalDepDirlabel->setStyleSheet(QString("background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(22, 86, 232, 1), stop:%1 rgba(22, 86, 232, 0.5), stop:%2 rgba(111, 111, 111, 0.5));").arg(percent).arg(percent + 0.1));
            ui->externalDepDirlabel->setToolTip(QString("Freier Speicher: %1 GB / Gesamtspeicher: %2 GB").arg(bytesAvailable / 1024).arg(size / 1024));
        }
        ui->externalDepDirlabel->setText(externalDepDir);
    }

    if (settings.value("showSalesWidget", true).toBool() && RBAC::Instance()->hasPermission("salesinfo_view")) {
        ui->salesFrame->setHidden(false);
        ui->lcdNumberDay->display(Database::getDayCounter());
        ui->lcdNumberMonth->display(Database::getMonthCounter());
        ui->lcdNumberYear->display(Database::getYearCounter());
    } else {
        ui->salesFrame->setHidden(true);
    }

    ui->serverModeCheckBox->setText(tr("Server Modus (Importverzeichnis: %1, Zeichensatz: %2)")
                                    .arg(settings.value("importDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString())
                                    .arg(settings.value("importCodePage", "UTF-8").toString()));

    m_watcherpath = settings.value("importDirectory", QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).toString();

    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
        qDebug() << "Function Name: " << Q_FUNC_INFO << " remove directories from WatchList";
    }

    if (isServerMode()) {
        m_watcher.addPath(m_watcherpath);
        ui->importWidget->setVisible(true);
        emit serverModeCheckBox_clicked(true);
    } else {
        ui->importWidget->setVisible(false);
        // emit serverModeCheckBox_clicked(false);
    }

    if (Database::isCashRegisterInAktive()) {
        ui->registerButton->setEnabled(false);
        ui->taskButton->setEnabled(false);
        ui->serverModeCheckBox->setEnabled(false);
    }

    ui->displayNameLabel->setText(RBAC::Instance()->getDisplayname());
    int gender = RBAC::Instance()->getGender();
    QPixmap pixmap;
    QString avatar = RBAC::Instance()->getAvatar();
    if (avatar.isEmpty())
        pixmap = (gender == 0)?QPixmap(":src/icons/user_male_480.png"):QPixmap(":src/icons/user_female_480.png");
    else
        pixmap = QPixmap(avatar);

    ui->userInfoIconLabel->setPixmap(pixmap.scaled(120, 120, Qt::KeepAspectRatio));
    ui->userInfoFrame->setVisible((RBAC::Instance()->getUserId() > 0));

    // Remove old Importfiles
    QDir dir(m_watcherpath);
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
}

void QRKHome::setServerMode(bool mode)
{
    serverModeCheckBox_clicked(mode);
}

bool QRKHome::isServerMode()
{
    return ui->serverModeCheckBox->isChecked();
}

void QRKHome::menuSlot()
{
    QPoint p(ui->menuButton->x() + ui->menuButton->width() - m_menu->sizeHint().width(),
             ui->menuButton->y() - m_menu->sizeHint().height());

    m_menu->move(mapToGlobal(p));
    m_menu->show();
}

//--------------------------------------------------------------------------------

void QRKHome::settingsSlot()
{
    if(!RBAC::Instance()->hasPermission("settings_access")) {
        QMessageBox::warning(Q_NULLPTR, tr("Information!"), tr("Leider haben Sie keine Berechtigung.\nFehlende Berechtigung '%1'").arg(RBAC::Instance()->getPermNameFromID(RBAC::Instance()->getPermIDfromKey("settings_access"))));
        return;
    }

    SettingsDialog tab;
    if (tab.exec() == QDialog::Accepted )
    {
        init();
        emit refreshMain();
    }
}

//--------------------------------------------------------------------------------

void QRKHome::taskSlot()
{
    if(!RBAC::Instance()->hasPermission("tasks_access", true)) {
        QMessageBox::warning(Q_NULLPTR, tr("Information!"), tr("Leider haben Sie keine Berechtigung.\nFehlende Berechtigung '%1'").arg(RBAC::Instance()->getPermNameFromID(RBAC::Instance()->getPermIDfromKey("task_access"))));
        return;
    }

    QPoint p(ui->taskButton->x() + ui->taskButton->width() - m_task->sizeHint().width(),
             ui->taskButton->y() - m_task->sizeHint().height());

    m_task->move(mapToGlobal(p));
    m_task->show();
}

void QRKHome::managerSlot()
{
    if(!RBAC::Instance()->hasPermission("manager_access", true)) return;

    QPoint p(ui->managerButton->x() + ui->managerButton->width() - m_manager->sizeHint().width(),
             ui->managerButton->y() - m_manager->sizeHint().height());

    m_manager->move(mapToGlobal(p));
    m_manager->show();
}

void QRKHome::serverModeCheckBox_clicked(bool checked)
{
    ui->importWidget->setVisible(checked);
    ui->registerButton->setEnabled(!checked);
    ui->taskButton->setEnabled(!checked);

    QrkSettings settings;
    if (settings.value("importServerFullscreen", false).toBool()) {
        if (RBAC::Instance()->hasPermission("salesinfo_view"))
            ui->salesFrame->setHidden(checked?checked: !settings.value("showSalesWidget", true).toBool());

        ui->userInfoFrame->setHidden(checked?checked: (RBAC::Instance()->getUserId() < 1));
        ui->pathFrame->setHidden(checked);
    } else {
        ui->salesFrame->setHidden(!settings.value("showSalesWidget", true).toBool());
        ui->pathFrame->setHidden(false);
    }

    if (checked) {
        if (Utils::isDirectoryWritable(m_watcherpath)) {
            m_watcher.addPath(m_watcherpath);
            /* create and remove file
           * to scan at servermode startup
           */
            QFile f(m_watcherpath + "/scan");
            f.open(QFile::WriteOnly);
            f.putChar('s');
            f.close();
            f.remove();
        } else {
            QMessageBox::warning(this,tr("Fehler"),tr("Import Verzeichnis %1 ist nicht beschreibbar.").arg(m_watcherpath),QMessageBox::Ok);
            ui->serverModeCheckBox->setChecked(false);
            emit serverModeCheckBox_clicked(false);
        }

    } else {

        m_watcher.removePath(m_watcherpath);
        connect(this, &QRKHome::stopWatcher, m_fw, &FileWatcher::stopWorker);
        emit stopWatcher();

        QRKProgress *p = new QRKProgress();
        connect(m_fw, &FileWatcher::workerStopped, p, &QRKProgress::deleteLater);
        connect(p, &QRKProgress::destroyed, this, &QRKHome::servermodefinished);

        p->setText(tr("Warte auf die Fertigstellung der aktuellen Importdatei."));
        p->setWaitMode();
        p->show();
        qApp->processEvents();

        m_fw->removeDirectories();
        ui->serverModeCheckBox->setChecked(false);
    }
}

void QRKHome::importInfo(QString str, bool isError)
{
    int count = ui->importWidget->count();
    if (count > 99) {
        delete ui->importWidget->takeItem(count -1);
    }
    ui->importWidget->addItem(QDateTime::currentDateTime().toString() + ": " + str);
    if (isError) {
        int count = ui->importWidget->count();
        if (str.startsWith(tr("INFO"))) {
            ui->importWidget->item(count -1)->setTextColor(Qt::darkYellow);
            qWarning() << "Function Name: " << Q_FUNC_INFO << str;
        } else {
            ui->importWidget->item(count -1)->setTextColor(Qt::red);
            qWarning() << "Function Name: " << Q_FUNC_INFO << str;
            serverCriticalMessageBox(QObject::tr("Import"), str);
        }
    } else {
        int count = ui->importWidget->count();
        ui->importWidget->item(count -1)->setTextColor(Qt::darkGreen);
        qDebug() << "Function Name: " << Q_FUNC_INFO << str;
    }

    ui->importWidget->sortItems(Qt::DescendingOrder);
    ui->importWidget->setWordWrap( true);
//    ui->importWidget->repaint();

    ui->lcdNumberDay->display(Database::getDayCounter());
    ui->lcdNumberMonth->display(Database::getMonthCounter());
    ui->lcdNumberYear->display(Database::getYearCounter());

}

void QRKHome::serverCriticalMessageBox(QString title, QString message)
{
    QrkSettings settings;
    if (settings.value("serverCriticalMessageBox", true).toBool()) {
        serverModeCheckBox_clicked(false);
        QMessageBox messageBox(QMessageBox::Critical,
                               title,
                               message,
                               QMessageBox::Yes,
                               Q_NULLPTR);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Weiter"));
        messageBox.setWindowFlags(messageBox.windowFlags() | Qt::WindowStaysOnTopHint);
        messageBox.exec();
    }
}
