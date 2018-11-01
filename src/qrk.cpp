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

#include "defines.h"
#include "database.h"
#include "databasemanager.h"
#include "aboutdlg.h"
#include "qrk.h"
#include "reports.h"
#include "export/exportdep.h"
#include "export/exportjournal.h"
#include "qrkhome.h"
#include "qrkdocument.h"
#include "qrkregister.h"
#include "qrktimedmessagebox.h"
#include "manager/managerdialog.h"
#include "utils/demomode.h"
#include "import/csvimportwizard.h"
#include "singleton/spreadsignal.h"
#include "RK/rk_signaturemodulefactory.h"
#include "foninfo.h"
#include "backup.h"
#include "utils/utils.h"
#include "utils/versionchecker.h"
#include "preferences/qrksettings.h"
#include "pluginmanager/pluginmanager.h"
#include "pluginmanager/pluginview.h"
#include "pluginmanager/Interfaces/independentinterface.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "3rdparty/ckvsoft/rbac/aclmanager.h"
#include "3rdparty/ckvsoft/rbac/aclwizard.h"
#include "3rdparty/ckvsoft/rbac/userlogin.h"
#include <ui_qrk.h>

#include <QStackedWidget>
#include <QLabel>
#include <QLCDNumber>
#include <QTimer>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QProgressBar>
#include <QTreeView>
#include <QThread>

//-----------------------------------------------------------------------

QRK::QRK(bool servermode)
    : QMainWindow(), ui(new Ui::MainWindow), m_servermode(servermode)

{
    connect(Spread::Instance(), &SpreadSignal::updateProgressBar, this, &QRK::setStatusBarProgressBar);
    connect(Spread::Instance(), &SpreadSignal::updateProgressBarWait, this, &QRK::setStatusBarProgressBarWait);
    connect(Spread::Instance(), &SpreadSignal::updateSafetyDevice, this, &QRK::setSafetyDevice);

    connect(RBAC::Instance(), static_cast<void(Acl::*)()>(&Acl::userChanged), this, &QRK::init);

    ui->setupUi(this);

    m_dep = new QLabel(this);
    m_dep->setMinimumSize(m_dep->sizeHint());
    m_depPX = new QLabel(this);
    m_depPX->setMinimumSize(m_depPX->sizeHint());
    m_safetyDevicePX = new QLabel(this);
    m_safetyDevicePX->setMinimumSize(m_safetyDevicePX->sizeHint());

    m_currentRegisterYearLabel = new QLabel(this);
    m_currentRegisterYearLabel->setMinimumSize(m_currentRegisterYearLabel->sizeHint());
    m_currentRegisterYearLabel->setToolTip(tr("Kassenjahr"));
    m_cashRegisterIdLabel = new QLabel(this);
    m_cashRegisterIdLabel->setMinimumSize(m_cashRegisterIdLabel->sizeHint());
    m_cashRegisterIdLabel->setToolTip(tr("Kassenidentifikationsnummer"));

    m_progressBar = new QProgressBar(this);
    m_dateLcd = new QLCDNumber (this);
    m_dateLcd->setDigitCount(20);
    m_dateLcd->setMode (QLCDNumber::Dec);
    m_dateLcd->setSegmentStyle (QLCDNumber::Flat);
    m_dateLcd->setFrameStyle (QFrame::NoFrame);

    statusBar()->addPermanentWidget(m_dep,0);
    statusBar()->addPermanentWidget(m_depPX,0);
    statusBar()->addPermanentWidget(m_safetyDevicePX,0);
    statusBar()->addPermanentWidget(m_cashRegisterIdLabel,0);
    statusBar()->addPermanentWidget(m_currentRegisterYearLabel,0);
    statusBar()->addPermanentWidget(m_progressBar,0);
    statusBar()->addPermanentWidget(m_dateLcd,0);

    m_checkDateTime = QDateTime::currentDateTime();
    m_timer = new QTimer (this);
    connect (m_timer, &QTimer::timeout, this, &QRK::timerDone);

    m_currentRegisterYear = QDateTime::currentDateTime().toString("yyyy").toInt();
    QFont font = QApplication::font();
    font.setPointSize(11);
    QApplication::setFont(font);

    // setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    PluginManager::instance()->initialize();
    initPlugins();

    //Stacked Widget
    m_stackedWidget = new QStackedWidget(this);
    this->setCentralWidget(m_stackedWidget);

    m_timer->start(1000);

    iniStack();

    connect(ui->export_CSV, &QAction::triggered, this, &QRK::export_CSV);
    connect(ui->import_CSV, &QAction::triggered, this, &QRK::import_CSV);
    connect(ui->export_JSON, &QAction::triggered, this, &QRK::export_JSON);
    connect(ui->actionDEPexternalBackup, &QAction::triggered, this, &QRK::backupDEP);
    connect(ui->actionDatenbank_sichern, &QAction::triggered, this, &QRK::backup);
    connect(ui->actionAbout_QRK, &QAction::triggered, this, &QRK::actionAbout_QRK);
    connect(ui->actionAbout_QT, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->actionQRK_Forum, &QAction::triggered, this, &QRK::actionQRK_Forum);
    connect(ui->actionDEMO_Daten_zur_cksetzen, &QAction::triggered, this, &QRK::actionResetDemoData);
    connect(ui->actionDEMOMODUS_Verlassen, &QAction::triggered, this, &QRK::actionLeaveDemoMode);
    connect(ui->actionInfos_zur_Registrierung_bei_FON, &QAction::triggered, this, &QRK::infoFON);
    connect(ui->actionResuscitationCashRegister, &QAction::triggered, this, &QRK::actionResuscitationCashRegister);
    connect(ui->actionAclManager, &QAction::triggered, this, &QRK::actionAclManager);

    connect(this, &QRK::setServerMode, m_qrk_home, &QRKHome::setServerMode);
    connect(m_qrk_home, &QRKHome::endOfDay, this, &QRK::endOfDaySlot);
    connect(m_qrk_home, &QRKHome::endOfMonth, this, &QRK::endOfMonthSlot);
    connect(m_qrk_home, &QRKHome::fullScreenButton_clicked, this, &QRK::fullScreenSlot);
    connect(m_qrk_home, &QRKHome::exitButton_clicked, this, &QRK::exitSlot);
    connect(m_qrk_home, &QRKHome::logOnOff, this, &QRK::logOnOff);

    connect(m_qrk_home, &QRKHome::registerButton_clicked, this, &QRK::onRegisterButton_clicked);
    connect(m_qrk_home, &QRKHome::documentButton_clicked, this, &QRK::onDocumentButton_clicked);
    connect(m_qrk_home, &QRKHome::productmanagerButton_clicked, this, &QRK::onManagerButton_clicked);
    connect(m_qrk_home, &QRKHome::usermanagerButton_clicked, this, &QRK::actionAclManager);
    connect(m_qrk_home, &QRKHome::refreshMain, this, &QRK::init);

    connect(m_qrk_register, &QRKRegister::cancelRegisterButton_clicked, this, &QRK::onCancelRegisterButton_clicked);
    connect(m_qrk_register, &QRKRegister::finishedReceipt, this, &QRK::finishedReceipt);

    connect(m_qrk_document, &QRKDocument::cancelDocumentButton_clicked, this, &QRK::onCancelDocumentButton_clicked);
    connect(m_qrk_document, &QRKDocument::documentButton_clicked, this, &QRK::onDocumentButton_clicked);

    QString title = QString("QRK V%1.%2 - Qt Registrier Kasse - %3").arg(QRK_VERSION_MAJOR).arg(QRK_VERSION_MINOR).arg(Database::getShopName());
    setWindowTitle ( title );

    init();

    QrkSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());

    m_checkerThread = new QThread;
    m_versionChecker = new VersionChecker;
    m_versionChecker->moveToThread(m_checkerThread);
    connect(m_versionChecker, &VersionChecker::Version, this, &QRK::newApplicationVersionAvailable);
    connect(m_checkerThread, &QThread::started, m_versionChecker, &VersionChecker::run);
    m_checkerThread->start();
}

//--------------------------------------------------------------------------------

QRK::~QRK()
{
    delete ui;
    PluginManager::instance()->uninitialize();
    m_timer->stop();
}

//--------------------------------------------------------------------------------

void QRK::iniStack()
{
    m_qrk_home =  new QRKHome(m_servermode, m_stackedWidget);
    m_stackedWidget->addWidget(m_qrk_home);

    m_qrk_register =  new QRKRegister(m_stackedWidget);
    m_stackedWidget->addWidget(m_qrk_register);

    m_qrk_document =  new QRKDocument(m_stackedWidget);
    m_stackedWidget->addWidget(m_qrk_document);

    m_stackedWidget->setCurrentWidget(m_qrk_home);
}

//--------------------------------------------------------------------------------

void QRK::initPlugins()
{
    QStringList plugins = PluginManager::instance()->plugins();
    QListIterator<QString> i(plugins);
    bool hasPlugins = false;
    while(i.hasNext()) {
        QString path = i.next();
        QString name = PluginManager::instance()->getNameByPath(path);
        IndependentInterface *plugin = qobject_cast<IndependentInterface *>(PluginManager::instance()->getObjectByName(name));
        if (plugin) {
            QAction *action = ui->menuPlugins->addAction(plugin->getPluginName(), this, SLOT(runPlugin()));
            action->setObjectName(name);
            addAction(action);
            delete plugin;
            hasPlugins = true;
        }
    }
    if (hasPlugins)
        ui->menuPlugins->addSeparator();

      ui->menuPlugins->addAction(tr("Plugins ..."), this, SLOT(viewPlugins()));

}

void QRK::runPlugin()
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());
    QString name = action->objectName();
    IndependentInterface *plugin = qobject_cast<IndependentInterface *>(PluginManager::instance()->getObjectByName(name));
    if (plugin) {
        plugin->process();
        delete plugin;
    }
}

void QRK::viewPlugins()
{
    PluginView view(this);
    view.setCloseButtonVisible(true);
    view.exec();
}

void QRK::timerDone()
{
    QDateTime t = QDateTime::currentDateTime();
    m_dateLcd->display (t.toString("dd-MM-yyyy  hh:mm:ss"));
    if (m_checkDateTime.secsTo(t) < 0)
        DateTimeCheck();

    m_checkDateTime = t;
}

//--------------------------------------------------------------------------------

void QRK::DateTimeCheck()
{
    // DateTime check
    if (Database::getLastJournalEntryDate().secsTo(QDateTime::currentDateTime()) < 0) {
        QMessageBox messageBox(QMessageBox::Critical,
                               QObject::tr("Eventueller Datum/Uhrzeit Fehler"),
                               QObject::tr("ACHTUNG! Die Uhrzeit des Computers ist eventuell falsch.\n\nLetzter Datenbankeintrag: %1\nDatum/Uhrzeit: %2").arg(Database::getLastJournalEntryDate().toString()).arg(QDateTime::currentDateTime().toString()),
                               QMessageBox::Yes | QMessageBox::No,
                               this);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("QRK beenden?"));
        messageBox.setButtonText(QMessageBox::No, QObject::tr("Weiter machen"));
        messageBox.setWindowFlags(messageBox.windowFlags() | Qt::WindowStaysOnTopHint);

        if (messageBox.exec() == QMessageBox::Yes )
        {
            QrkSettings settings;
            settings.removeSettings("QRK_RUNNING", false);
            if ( !isFullScreen() ) {
                settings.save2Settings("mainWindowGeometry", saveGeometry(), false);
                settings.save2Settings("mainWindowState", saveState(), false);
            }
            PluginManager::instance()->uninitialize();
            QApplication::exit();
        }
    }
}

void QRK::setStatusBarProgressBar(int value, bool add)
{
    if (value < 0) {
        if (m_progressBar->value() > -1)
            m_progressBar->setValue(100);

        QTimer::singleShot(2000, m_progressBar, SLOT(reset()));
        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();

    } else {
        if (!QApplication::overrideCursor())
            QApplication::setOverrideCursor(Qt::BusyCursor);

        if (add)
            value = m_progressBar->value() + value;
        m_progressBar->setValue(value);
    }
}

void QRK::setStatusBarProgressBarWait(bool on_off)
{
    m_progressBar->setMinimum(0);
    if (on_off) {
        m_progressBar->setMaximum(0);
        m_progressBar->setValue(0);

        if (!QApplication::overrideCursor())
            QApplication::setOverrideCursor(Qt::BusyCursor);
    } else {
        m_progressBar->setMaximum(100);
        m_progressBar->reset();

        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();
    }
}

void QRK::setSafetyDevice(bool active)
{
    if (active != m_previousSafetyDeviceState) {
        m_previousSafetyDeviceState = active;
        init();
    }
}

//--------------------------------------------------------------------------------

void QRK::init()
{

    m_currentRegisterYearLabel->setText(QObject::tr(" KJ: %1 ").arg(getCurrentRegisterYear()));
    m_cashRegisterIdLabel->setText(QObject::tr(" KID: %1 ").arg(Database::getCashRegisterId()));

    ui->menuDEMOMODUS->setEnabled(DemoMode::isDemoMode());
    ui->menuDEMOMODUS->menuAction()->setVisible(DemoMode::isDemoMode());
    ui->export_JSON->setVisible(RKSignatureModule::isDEPactive());
    ui->actionInfos_zur_Registrierung_bei_FON->setVisible(RKSignatureModule::isDEPactive() && !DemoMode::isDemoMode());

    setShopName();
    m_cashRegisterId = Database::getCashRegisterId();

    m_qrk_home->init();

    /**
   * @brief DEPaktive
   */
    QPixmap pm1(32, 32);
    QPixmap pm2(32, 32);

    QString DEPtoolTip = tr("DEP-7 Inaktiv");
    QString DEPaktive = tr("Inaktiv");
    m_dep->setText(tr("DEP-7"));

    if (Database::getTaxLocation() == "AT") {
        m_qrk_home->setExternalDepLabels(true);
        m_dep->setVisible(true);
        m_depPX->setVisible(true);
        m_safetyDevicePX->setVisible(true);

        if (RKSignatureModule::isDEPactive()) {
            RKSignatureModule *signaturinfo = RKSignatureModuleFactory::createInstance("", DemoMode::isDemoMode());

            DEPaktive = tr("Aktiviert");

            QString serial = "0";
            QString cardType = "keine";


            if (signaturinfo->selectApplication()) {
                m_previousSafetyDeviceState = true;
                serial = signaturinfo->getCertificateSerial(true);
                cardType = signaturinfo->getCardType();
                DEPtoolTip = tr("DEP-7 (RKSV Daten Erfassungs Protokoll) aktiv, Kartentype: %1 Seriennummer: %2").arg(cardType).arg(serial);
                pm2.fill(Qt::green);
                m_safetyDevicePX->setToolTip(tr("SignaturErstellungsEinheit aktiv, Kartentype: %1 Seriennummer: %2").arg(cardType).arg(serial));
                m_qrk_register->safetyDevice(true);
                m_qrk_home->safetyDevice(true);
            } else {
                m_previousSafetyDeviceState = false;
                DEPtoolTip = tr("DEP-7 (RKSV Daten Erfassungs Protokoll) aktiv, SignaturErstellungsEinheit ausgefallen");
                pm2.fill(Qt::red);
                m_safetyDevicePX->setToolTip(tr("SignaturErstellungsEinheit ausgefallen"));
                m_qrk_register->safetyDevice(false);
                m_qrk_home->safetyDevice(false);
            }

            pm1.fill(Qt::green);
            m_depPX->setPixmap(pm1);
            m_depPX->setToolTip(tr("DEP-7 (RKSV Daten Erfassungs Protokoll) aktiv"));
            m_safetyDevicePX->setPixmap(pm2);

            delete signaturinfo;
        } else {
            pm1.fill(Qt::red);
            m_depPX->setPixmap(pm1);
            m_safetyDevicePX->setVisible(false);
        }
        m_dep->setToolTip(DEPtoolTip);
    } else {
        m_qrk_home->setExternalDepLabels(false);
        m_dep->setVisible(false);
        m_depPX->setVisible(false);
        m_safetyDevicePX->setVisible(false);
    }
}

void QRK::logOnOff(bool)
{
    if (RBAC::Instance()->Login()) {
        if (RBAC::Instance()->getUserId() > 0) {
            RBAC::Instance()->setuserId(-1);
            init();
            UserLogin *login = new UserLogin(this);
            connect(login, &UserLogin::accepted, this, &QRK::init);
            login->exec();
        }
    } else {
        RBAC::Instance()->setuserId(0);
    }
}

void QRK::setResuscitationCashRegister(bool visible)
{
    ui->menuNEUE_KASSE_ERSTELLEN->setEnabled(visible);
    ui->menuNEUE_KASSE_ERSTELLEN->menuAction()->setVisible(visible);
}

//--------------------------------------------------------------------------------

void QRK::setShopName()
{
    m_shopName = Database::getShopName();
    m_cashRegisterIdLabel->setText(QObject::tr(" KID: %1 ").arg(Database::getCashRegisterId()));
}

//--------------------------------------------------------------------------------

void QRK::actionAclManager()
{
    if (!RBAC::Instance()->hasPermission("admin_access", true)) return;
    AclWizard::createFirstRoleAndUser();
    if (RBAC::Instance()->getAllUsers().isEmpty())
        return;

    AclManager am;
    am.exec();
    emit init();
}

void QRK::actionAbout_QRK()
{
    AboutDlg dlg;
    dlg.exec();
}

void QRK::actionQRK_Forum()
{
    QString link = "http://forum.ckvsoft.at/";
    QDesktopServices::openUrl(QUrl(link));
}

//--------------------------------------------------------------------------------
void QRK::import_CSV()
{
    if(!RBAC::Instance()->hasPermission("import_csv", true)) return;

    CsvImportWizard importWizard;
    importWizard.exec();
}

void QRK::export_CSV()
{
    ExportJournal xport;
    xport.Export();
}

void QRK::export_JSON()
{
    ExportDEP xport;
    xport.dExport();
}

void QRK::infoFON()
{
    setStatusBarProgressBarWait(true);
    ui->menubar->setEnabled(false);
    FONInfo finfo;
    setStatusBarProgressBarWait(false);
    ui->menubar->setEnabled(true);
    finfo.exec();
}

//--------------------------------------------------------------------------------

void QRK::endOfDaySlot()
{
    if (RBAC::Instance()->hasPermission("tasks_create_eod", true)) {
        Reports rep;
        rep.endOfDay();
    }
}

//--------------------------------------------------------------------------------

void QRK::endOfMonthSlot()
{
    if (RBAC::Instance()->hasPermission("tasks_create_eom", true)) {
        Reports rep;
        rep.endOfMonth();
    }
}

void QRK::onCancelDocumentButton_clicked()
{
    m_stackedWidget->setCurrentWidget(m_qrk_home);
    m_qrk_home->init();
}

//--------------------------------------------------------------------------------

void QRK::onRegisterButton_clicked()
{
    if(!RBAC::Instance()->hasPermission("register_access")) {
        QMessageBox::warning(0, tr("Information!"), tr("Leider haben Sie keine Berechtigung.\nFehlende Berechtigung '%1'").arg(RBAC::Instance()->getPermNameFromID(RBAC::Instance()->getPermIDfromKey("register_access"))));
        return;
    }

    m_qrk_register->init();
    m_qrk_register->newOrder();
    m_stackedWidget->setCurrentWidget(m_qrk_register);
}

void QRK::onManagerButton_clicked()
{
    if(!RBAC::Instance()->hasPermission("manager_access")) {
        QMessageBox::warning(0, tr("Information!"), tr("Leider haben Sie keine Berechtigung.\nFehlende Berechtigung '%1'").arg(RBAC::Instance()->getPermNameFromID(RBAC::Instance()->getPermIDfromKey("manager_access"))));
        return;
    }

    ManagerDialog manager;
    manager.exec();
}

//--------------------------------------------------------------------------------

void QRK::onCancelRegisterButton_clicked()
{
    m_qrk_register->clearModel();
    m_stackedWidget->setCurrentWidget(m_qrk_home);
    m_qrk_home->init();
}

void QRK::finishedReceipt()
{
    m_qrk_register->clearModel();
    m_qrk_home->init();
    //    m_qrk_register->init();
    m_qrk_register->newOrder();
}

//--------------------------------------------------------------------------------

void QRK::onDocumentButton_clicked()
{
    if(!RBAC::Instance()->hasPermission("documents_access")) {
        QMessageBox::warning(0, tr("Information!"), tr("Leider haben Sie keine Berechtigung.\nFehlende Berechtigung '%1'").arg(RBAC::Instance()->getPermNameFromID(RBAC::Instance()->getPermIDfromKey("documents_access"))));
        return;
    }

    m_stackedWidget->setCurrentWidget(m_qrk_document);
    m_qrk_document->documentList(m_qrk_home->isServerMode());
}

//--------------------------------------------------------------------------------

void QRK::fullScreenSlot()
{
    QrkSettings settings;
    if ( isFullScreen() )
    {
        setWindowState(windowState() ^ Qt::WindowFullScreen);
        restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
        restoreState(settings.value("mainWindowState").toByteArray());
    } else {
        settings.value("mainWindowGeometry", saveGeometry());
        settings.value("mainWindowState", saveState());
        showFullScreen();
    }
}

//--------------------------------------------------------------------------------

void QRK::closeEvent (QCloseEvent *event)
{
    if (m_qrk_home->isServerMode()) {
        emit setServerMode(false);
        connect(m_qrk_home, &QRKHome::servermodefinished, this, &QRK::exitSlot);

//        QMessageBox::information(this, tr("Server Modus aktiv"), tr("Beenden Sie bitte zuerst den Servermodus."));
        event->ignore();
        return;
    }

    disconnect(m_qrk_home, &QRKHome::servermodefinished, 0,0);
    QrkTimedMessageBox mb(5, tr("Beenden"),
                   tr("Möchten sie wirklich beenden ?"),
                   QMessageBox::Question,
                   QMessageBox::Yes | QMessageBox::Default,
                   QMessageBox::No | QMessageBox::Escape,
                   QMessageBox::NoButton);
    mb.setButtonText(QMessageBox::Yes, tr("Ja"));
    mb.setButtonText(QMessageBox::No, tr("Nein"));
    mb.setDefaultButton(QMessageBox::Yes);

    if (mb.exec() == QMessageBox::Yes) {
        QrkSettings settings;
        settings.removeSettings("QRK_RUNNING", false);
        if ( !isFullScreen() ) {
            settings.save2Settings("mainWindowGeometry", saveGeometry(), false);
            settings.save2Settings("mainWindowState", saveState(), false);
        }
        PluginManager::instance()->uninitialize();
        DatabaseManager::clear();
        QApplication::exit();
    } else {
        event->ignore();
    }
}

//--------------------------------------------------------------------------------

void QRK::exitSlot()
{
    QCloseEvent *event = new QCloseEvent();
    closeEvent(event);
}

void QRK::actionLeaveDemoMode()
{
    Database::resetAllData();
    DemoMode::leaveDemoMode();
    restartApplication();
}

void QRK::actionResetDemoData()
{
    Database::resetAllData();
    restartApplication();
}

void QRK::closeCashRegister()
{
    Reports rep(this, true);
    rep.endOfMonth();
    rep.createNullReceipt(PAYED_BY_CONCLUSION_RECEIPT);
    Database::setCashRegisterInAktive();
    backupDEP();
    restartApplication();
}

void QRK::actionResuscitationCashRegister()
{
    backupDEP();
    Database::resetAllData();
    RKSignatureModule::setDEPactive(false);
    restartApplication();
}

void QRK::backupDEP()
{
    if (Export::getLastMonthReceiptId() == -1) {
        QMessageBox::information(this, tr("DEP-7 Datensicherung"), tr("Es wurde kein Monatsbeleg gefunden. Versuchen Sie die Sicherung nach einen Monatsabschlusses erneut."));
        return;
    }

    QrkSettings settings;
    QString directoryname = settings.value("externalDepDirectory", "").toString();
    if (!Utils::isDirectoryWritable(directoryname)) {
        QMessageBox messageBox(QMessageBox::Question,
                               QObject::tr("Externes DEP-7 Backup"),
                               QObject::tr("Das externe Medium %1 ist nicht vorhanden oder nicht beschreibbar.\nBitte beheben Sie den Fehler und drücken OK. Wenn das Backup zu einen späteren Zeitpunkt durchgeführt werden soll klicken Sie abbrechen.").arg(directoryname),
                               QMessageBox::Yes | QMessageBox::No,
                               0);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
        messageBox.setButtonText(QMessageBox::No, QObject::tr("Abbrechen"));

        if (messageBox.exec() == QMessageBox::No )
        {
            return;
        }
    }

    if (Utils::isDirectoryWritable(directoryname)) {
        Export xDep;
        bool ok = xDep.createBackup();
        Spread::Instance()->setProgressBarValue(-1);
        if (ok) {
            QMessageBox::information(this, tr("DEP-7 Datensicherung"), tr("DEP-7 Datensicherung abgeschlossen."));
            return;
        } else {
            QMessageBox::information(this, tr("DEP-7 Datensicherung"), tr("DEP-7 Datensicherung fehlgeschlagen."));
            return;
        }
    }

    backupDEP();
}

void QRK::backup()
{
    setStatusBarProgressBarWait(true);
    Backup::create();
    setStatusBarProgressBarWait(false);
    QMessageBox::information(this, tr("Datensicherung"), tr("Datensicherung abgeschlossen."));
}

void QRK::restartApplication()
{
    // Spawn a new instance of myApplication:
    QString app = QApplication::applicationFilePath();
    QStringList arguments = QApplication::arguments();
    arguments << "-r";
    QString wd = QDir::currentPath();
    QProcess::startDetached(app, arguments, wd);
    QApplication::exit(0);
}

void QRK::newApplicationVersionAvailable(QString version)
{    
    disconnect(m_versionChecker, &VersionChecker::Version, 0, 0);

    QString currentVersion = qApp->applicationVersion().right(6);
    if (QString::compare(version.right(6), currentVersion, Qt::CaseInsensitive) > 0) {
        QMessageBox messageBox(QMessageBox::Information,
                               QObject::tr("Out of Date"),
                               QObject::tr("Hinweis: Ihre QRK Version (%1) ist möglicherweise veraltet. QRK Version %2 ist verfügbar").arg(qApp->applicationVersion()).arg(version),
                               QMessageBox::Yes,
                               this);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
        messageBox.setWindowFlags(messageBox.windowFlags() | Qt::WindowStaysOnTopHint);

        messageBox.exec();
    }
    connect(m_versionChecker, &VersionChecker::Version, this, &QRK::newApplicationVersionAvailable);
}
