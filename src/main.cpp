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

#if defined(_WIN32) || defined(__APPLE__)
#include "3rdparty/fervor-autoupdate/fvupdater.h"
#endif

#include "qrk.h"
#include "qrktimedmessagebox.h"
#include "preferences/settingsdialog.h"
#include "preferences/qrksettings.h"
#include "utils/demomode.h"
#include "utils/utils.h"
#include "backup.h"
#include "reports.h"
#include "3rdparty/ckvsoft/rbac/userlogin.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "3rdparty/ckvsoft/uniquemachinefingerprint.h"

#include "defines.h"
#include "database.h"
#include "stdio.h"
#include "signal.h"

#include <QApplication>
#include <QStatusBar>
#include <QDir>
#include <QTranslator>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSharedMemory>
#include <QStyleFactory>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QSplashScreen>
#include <QGraphicsBlurEffect>
#include <QThread>

//--------------------------------------------------------------------------------
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QDebug>

void QRKMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & str)
{
    if (str.startsWith("QXcbConnection"))
        return;

    QString txt = "";
    bool debug = qApp->property("debugMsg").toBool();
    switch (type) {
    case QtDebugMsg:
        if (debug)
            txt = QString("%1 %2 %3 Debug: %4").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz")).arg((long)QThread::currentThread(), 16).arg(QApplication::applicationVersion()).arg(str);
        break;
    case QtInfoMsg:
        txt = QString("%1 %2 %3 Info: %4").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz")).arg((long)QThread::currentThread(), 16).arg(QApplication::applicationVersion()).arg(str);
        break;
    case QtWarningMsg:
        txt = QString("%1 %2 %3 Warning: %4").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz")).arg((long)QThread::currentThread(), 16).arg(QApplication::applicationVersion()).arg(str);
        break;
    case QtCriticalMsg:
        txt = QString("%1 %2 %3 Critical: %4").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz")).arg((long)QThread::currentThread(), 16).arg(QApplication::applicationVersion()).arg(str);
        break;
    case QtFatalMsg:
        txt = QString("%1 %2 %3 Fatal: %4").arg(QDateTime::currentDateTime().toString("yyyy.MM.dd hh:mm:ss.zzz")).arg((long)QThread::currentThread(), 16).arg(QApplication::applicationVersion()).arg(str);
        break;
    }

    QString confname = qApp->property("configuration").toString();
    if (!confname.isEmpty())
        confname =  "_" + confname;
    QFile outFile( QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QString("/qrk%1.log").arg(confname));
    if (outFile.size() > 20000000) /*20 Mega*/
        Backup::pakLogFile();

    if (!txt.isEmpty()) {
        outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        QTextStream ts(&outFile);
        ts << txt.simplified() << endl;
        outFile.close();
    }
}

void createAppDataLocation()
{
  QDir dir;
  dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

void setApplicationFont()
{
  QrkSettings settings;
  QList<QString> systemFontList = settings.value("systemfont").toString().split(",");
  if (systemFontList.count() < 3) {
      QFont font = QApplication::font();
      font.setPointSize(11);
      QApplication::setFont(font);
      return;
  }

  QFont sysFont(systemFontList.at(0));
  sysFont.setPointSize(systemFontList.at(1).toInt());
  sysFont.setStretch(systemFontList.at(2).toInt());

  QApplication::setFont(sysFont);

}

void loadStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (file.exists()) {
        file.open(QFile::ReadOnly);
        QString styleSheet = QString::fromLatin1(file.readAll());

        qApp->setStyleSheet(styleSheet);
    } else {
        qWarning() << "Function Name: " << Q_FUNC_INFO << " QSS File <" << sheetName << "> not found";
    }
}

void setNoPrinter(bool noPrinter = false)
{
    QrkSettings settings;
    settings.save2Settings("noPrinter", noPrinter, false);
}

bool isQRKrunning()
{
  QString s;
  QMessageBox msgWarning(QMessageBox::Warning,"","");

  QrkSettings settings;

  bool rc = settings.value("QRK_RUNNING", false).toBool();

  if(rc)
  {
    msgWarning.setWindowTitle(QObject::tr("Warnung"));
    s = QObject::tr("Das Programm läuft bereits oder wurde das letzte Mal gewaltsam beendet. Bitte überprüfen Sie das!\n(beim nächsten Programmstart wird diese Meldung nicht mehr angezeigt)");
    msgWarning.setText(s);
    msgWarning.exec();
    settings.removeSettings("QRK_RUNNING", false);
  }
  else
  {
    settings.save2Settings("QRK_RUNNING", true, false);
  }
  return rc;
}

//--------------------------------------------------------------------------------

void sighandler(int /*sig*/)
{
    QrkSettings settings;
    settings.removeSettings("QRK_RUNNING", false);
    qApp->exit();
}

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    app.setProperty("debugMsg", false);

    QLocale::setDefault(QLocale::German);

    QSplashScreen *splash = new QSplashScreen;
    splash->setPixmap(QPixmap(":src/icons/splash.png"));
    splash->setMinimumHeight(228);
    splash->show();

    Qt::Alignment topRight = Qt::AlignLeft | Qt::AlignBottom;
    splash->showMessage(QObject::tr("QRK wird gestartet ..."),topRight, Qt::black);

#ifndef QT_DEBUG
    qInstallMessageHandler(QRKMessageHandler);
#endif

    qSetMessagePattern("%{file}(%{line}): %{message}");

    QApplication::setOrganizationName("ckvsoft");
    QApplication::setOrganizationDomain("ckvsoft.at");
    QApplication::setApplicationName("QRK");
    QApplication::setApplicationVersion(QString("%1.%2").arg(QRK_VERSION_MAJOR).arg(QRK_VERSION_MINOR));

    splash->showMessage(QObject::tr("Datenverzeichnisse werden erstellt ..."),topRight, Qt::black);
    createAppDataLocation();

    splash->showMessage(QObject::tr("Übersetzungen werden geladen ..."),topRight, Qt::black);
    QString locale = QLocale::system().name();
    QTranslator trans;
    QString translationPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    if ( (trans.load(QLocale(), QLatin1String("qt"), QLatin1String("_"),translationPath) ||
          trans.load(QLocale(), QLatin1String("qt"), QLatin1String("_"),QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1String("i18n"))) )
        app.installTranslator(&trans);

    // QRK translation file is searched in the application install dir (e.g. /opt/qrk)
    // and also in /usr/share/qrk
    QTranslator trans2;  // if current language is not de and translation not found, use english
    if ( !locale.startsWith(QLatin1String("de")) &&
         (trans2.load(QLatin1String("QRK_") + locale, QCoreApplication::applicationDirPath() + QDir::separator() + QLatin1String("i18n")) ||
          trans2.load(QLatin1String("QRK_") + locale, QLatin1String("/usr/share/qrk")) ||
          trans2.load(QLatin1String("QRK_en"), QCoreApplication::applicationDirPath()  + QDir::separator() + QLatin1String("i18n")) ||
          trans2.load(QLatin1String("QRK_en"), QLatin1String("/usr/share/qrk"))) )
        app.installTranslator(&trans2);

    splash->showMessage(QObject::tr("Kommandozeilen Parameter werden verarbeitet ..."),topRight, Qt::black);
    QCommandLineParser parser;
    parser.setApplicationDescription("QRK");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption fullScreenOption(QStringList() << "f" << "fullscreen", QObject::tr("Startet im Vollbild Modus."));
    parser.addOption(fullScreenOption);

    QCommandLineOption minimizeScreenOption(QStringList() << "m" << "minimize", QObject::tr("Startet im Minimize Modus."));
    parser.addOption(minimizeScreenOption);

    QCommandLineOption noPrinterOption(QStringList() << "n" << "noprinter", QObject::tr("Verwendet den internen PDF Drucker."));
    parser.addOption(noPrinterOption);

    QCommandLineOption dbSelectOption("db", QObject::tr("Datenbank Definition Dialog."));
    parser.addOption(dbSelectOption);

    QCommandLineOption restartOption("r", QObject::tr("Erzwingt den Start."));
    parser.addOption(restartOption);

    QCommandLineOption styleSheetOption(QStringList() << "s" << "stylesheet", QObject::tr("StyleSheet <QSS Datei>."), QObject::tr("QSS Datei"));
    parser.addOption(styleSheetOption);

    QCommandLineOption serverModeOption(QStringList() << "servermode", QObject::tr("Startet QRK im Servermode"));
    parser.addOption(serverModeOption);

    QCommandLineOption configurationFileOption(QStringList() << "c" << "config", QObject::tr("Alternative Config <zB. Kasse1>"), QObject::tr("Name"));
    parser.addOption(configurationFileOption);

    QCommandLineOption debugModeOption(QStringList() << "d" << "debug", QObject::tr("Schreibt DEBUG Ausgaben in die Log-Datei"));
    parser.addOption(debugModeOption);

    parser.process(app);

    if (parser.isSet(configurationFileOption)) {
        app.setProperty("configuration", parser.value(configurationFileOption));
    }

    bool dbSelect = parser.isSet(dbSelectOption);
    bool fullScreen = parser.isSet(fullScreenOption);
    bool minimize = parser.isSet(minimizeScreenOption);
    bool noPrinter = parser.isSet(noPrinterOption);
    bool servermode = parser.isSet(serverModeOption);
    bool debugMsg = parser.isSet(debugModeOption);

#ifdef QT_DEBUG
    debugMsg = true;
    noPrinter = true;
#endif

    QrkSettings settings;

    if (parser.isSet(restartOption)) {
        settings.removeSettings("QRK_RUNNING", false);
    }

    QSize buttonsize = settings.value("ButtonSize", QSize(150, 60)).toSize();

    qApp->setStyleSheet("QFileDialog QPushButton, QWizard QPushButton, QMessageBox QPushButton {"
                        "min-width: " + QString::number(buttonsize.width() * 0.85) + "px;"
                        "min-height: " + QString::number(buttonsize.height() * 0.85) + "px;}"
                        "QScrollBar:vertical {"
                        "width: 25px;}"
                        );

    if (parser.isSet(styleSheetOption)) {
        loadStyleSheet(parser.value(styleSheetOption));
    }

    splash->setHidden(true);
    if (isQRKrunning())
      return 0;

    splash->setVisible(true);
    splash->showMessage(QObject::tr("Schriftarten werden geladen ..."),topRight, Qt::black);
    setApplicationFont();

    splash->showMessage(QObject::tr("Verbindung zur Datenbank wird hergestellt ..."),topRight, Qt::black);
    if ( !Database::open( dbSelect) ) {
        sighandler(0);
        return 0;
    }

    // Cleanup unused old globals Database entries
    Database::cleanup();

    // DateTime check
    if (Database::getLastJournalEntryDate().secsTo(QDateTime::currentDateTime()) < 0) {
        splash->setHidden(true);
        QMessageBox messageBox(QMessageBox::Critical,
                               QObject::tr("Eventueller Datum/Uhrzeit Fehler"),
                               QObject::tr("ACHTUNG! Die Uhrzeit des Computers ist eventuell falsch.\n\nLetzter Datenbankeintrag: %1\nDatum/Uhrzeit: %2").arg(Database::getLastJournalEntryDate().toString()).arg(QDateTime::currentDateTime().toString()),
                               QMessageBox::Yes | QMessageBox::No,
                               0);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("QRK beenden?"));
        messageBox.setButtonText(QMessageBox::No, QObject::tr("Weiter machen"));

        if (messageBox.exec() == QMessageBox::Yes )
        {
            QrkSettings settings;
            settings.removeSettings("QRK_RUNNING", false);

            return 0;
        }
        splash->setVisible(true);
    }

    QRK mainWidget(servermode);
    UserLogin *userLogin = new UserLogin(&mainWidget);

    splash->showMessage(QObject::tr("DEP-7 wird überprüft ..."),topRight, Qt::black);

    // DEP-7 Check
    QStringList error;
    if (!Database::isCashRegisterInAktive() && !DemoMode::isDemoMode() && RKSignatureModule::isDEPactive() && !Utils::checkTurnOverCounter(error)) {
        splash->setHidden(true);
        QMessageBox messageBox(QMessageBox::Critical,
                               QObject::tr("DEP-7 Fehler"),
                               QObject::tr("ACHTUNG! Das gespeicherte DEP-7 hat einen oder mehrere Fehler.\nEvtl. gibt es Zugriffsprobleme auf Ihre Datenbank.\nBitte sichern Sie Ihre Daten. Melden Sie die Kasse bei FON ab und nochmals neu an.\nBis dahin müssen Sie Belege per Hand erstellen und in der neuen Kasse erfassen.\nFür Infos steht Ihnen das Forum zu Verfügung."),
                               QMessageBox::Yes | QMessageBox::No,
                               0);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Kasse außer\nBetrieb nehmen?"));
        messageBox.setButtonText(QMessageBox::No, QObject::tr("Weiter machen"));
        messageBox.setDetailedText(error.join('\n'));
        if (messageBox.exec() == QMessageBox::Yes )
        {
            RBAC::Instance()->setuserId(0);
            mainWidget.closeCashRegister();
            return 0;
        }
        splash->setVisible(true);
    }

    mainWidget.setResuscitationCashRegister(Database::isCashRegisterInAktive());

    splash->showMessage(QObject::tr("Einstellungen werden geladen ..."),topRight, Qt::black);

    mainWidget.statusBar()->setStyleSheet(
                "QStatusBar { border-top: 1px solid darkgrey; border-radius: 1px;"
                "background: darkgrey; spacing: 3px; /* spacing between items in the tool bar */ }"
                );

    setNoPrinter(noPrinter);

    if (DemoMode::isDemoMode())
        debugMsg = true;

    app.setProperty("debugMsg", debugMsg);

    if (fullScreen && !minimize)
        mainWidget.setWindowState(mainWidget.windowState() ^ Qt::WindowFullScreen);
    else if (minimize && !fullScreen)
        mainWidget.setWindowState(mainWidget.windowState() ^ Qt::WindowMinimized);

    /*check if we have SET Demomode*/
    if (DemoMode::isModeNotSet()) {
        splash->setHidden(true);

        QMessageBox messageBox(QMessageBox::Question,
                               QObject::tr("DEMOMODUS"),
                               QObject::tr("Wird QRK (Qt Registrier Kasse) im Echtbetrieb verwendet?\n\nJA wenn QRK im produktiven Einsatz ist.\n\nNEIN wenn DEMO Daten verwendet werden."),
                               QMessageBox::Yes | QMessageBox::No,
                               0);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("Ja"));
        messageBox.setButtonText(QMessageBox::No, QObject::tr("Nein"));

        if (messageBox.exec() == QMessageBox::Yes )
        {
            DemoMode::leaveDemoMode();
        }
        splash->setVisible(true);
    } else if (DemoMode::isDemoMode() && RKSignatureModule::isDEPactive()) {
        splash->setHidden(true);
        QrkTimedMessageBox messageBox(5,
                               QMessageBox::Warning,
                               QObject::tr("DEMOMODUS"),
                               QObject::tr("ACHTUNG! Ihre Kasse befindet sich im DEMOMODUS mit aktivierten DEP-7. Erkundigen Sie sich im Forum über den  DEMOMODUS. In diesen Modus ist die Kasse NICHT RKSV Konform."),
                               QMessageBox::Yes
                               );

        messageBox.setDefaultButton(QMessageBox::Yes);
        messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
        messageBox.exec();
        splash->setVisible(true);
    }

    /*Check for MasterData*/
    QString cri = Database::getCashRegisterId();
    if ( cri.isEmpty() ) {
        splash->setHidden(true);
        QMessageBox::warning(0, QObject::tr("Kassenidentifikationsnummer"), QObject::tr("Stammdaten müssen vollständig ausgefüllt werden.."));
        RBAC::Instance()->setuserId(0);
        SettingsDialog tab;
        tab.exec();
        if ( Database::getCashRegisterId().replace("DEMO-", "").isEmpty() ) {
            QrkTimedMessageBox messageBox(5,
                                   QMessageBox::Warning,
                                   QObject::tr("Kassenidentifikationsnummer"),
                                   QObject::tr("Stammdaten wurden nicht vollständig ausgefüllt. QRK wird beendet."),
                                   QMessageBox::Yes
                                   );

            messageBox.setDefaultButton(QMessageBox::Yes);
            messageBox.setButtonText(QMessageBox::Yes, QObject::tr("OK"));
            messageBox.exec();
            sighandler(0);
            return 0;
        }
        splash->setVisible(true);
    }

    mainWidget.setShopName();

    UniqueMachineFingerprint fp;
    qInfo() << "Serialnumber: " << fp.getSystemUniqueId();
    qInfo() << "Validate: " << fp.validate(fp.getSystemUniqueId());

    splash->finish(&mainWidget);

#if defined(_WIN32) || defined(__APPLE__)
    // Set feed URL before doing anything else
    FvUpdater::sharedUpdater()->SetFeedURL("http://service.ckvsoft.at/qrk/updates/v1.10/Appcast.xml");
    FvUpdater::sharedUpdater()->setRequiredSslFingerPrint("c3b038cb348c7d06328579fb950a48eb");	// Optional
    FvUpdater::sharedUpdater()->setHtAuthCredentials("username", "password");	// Optional
    FvUpdater::sharedUpdater()->setUserCredentials(Database::getShopName() + "/" + Database::getCashRegisterId() + "/" + QApplication::applicationVersion());
    FvUpdater::sharedUpdater()->setSkipVersionAllowed(false);	// Optional
    FvUpdater::sharedUpdater()->setRemindLaterAllowed(true);	// Optional
    // Finish Up old Updates
    FvUpdater::sharedUpdater()->finishUpdate();

    // Check for updates automatically
    FvUpdater::sharedUpdater()->CheckForUpdatesSilent();
#endif

    /*
    QGraphicsBlurEffect *effect = new QGraphicsBlurEffect;
    effect->setBlurHints(QGraphicsBlurEffect::PerformanceHint);
    effect->setBlurRadius(17);
    mainWidget.setGraphicsEffect(effect);
    */
    mainWidget.show();

    if (RBAC::Instance()->Login()) {
        userLogin->show();
    } else {
        RBAC::Instance()->setuserId(0);
    }

    delete splash;

    if (Database::getLastVersionInfo() < QApplication::applicationVersion()) {
        QrkSettings settings;
        settings.save2Database("version", QApplication::applicationVersion());
    }

    signal(SIGTERM, sighandler);

    return app.exec();
}
