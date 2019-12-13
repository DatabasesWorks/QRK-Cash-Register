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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QMainWindow>
#include <QDate>
#include <QUdpSocket>

class QRKHome;
class QRKDocument;
class QRKRegister;
class QRKGastro;
class QStackedWidget;
class QLCDNumber;
class QLabel;
class QProgressBar;
class VersionChecker;

namespace Ui {
  class MainWindow;
}

class QRK : public QMainWindow
{
    Q_OBJECT

public:

    QRK(bool servermode = false);
    ~QRK();

    void setShopName();
    int getCurrentRegisterYear(){return m_currentRegisterYear;}
    void setResuscitationCashRegister(bool visible);
    void closeCashRegister();

    QLabel *m_currentRegisterYearLabel;
    QLabel *m_cashRegisterIdLabel;

protected:
    void iniStack();

signals:
    void setServerMode(bool);

private slots:
    void endOfDaySlot();
    void endOfMonthSlot();
    void onRegisterButton_clicked();
    void onGRegisterButton_clicked();
    void onCancelRegisterButton_clicked();
    void onCancelGastroButton_clicked();
    void onManagerButton_clicked();
    void finishedReceipt();
    void init();
    void initPlugins();

    void onDocumentButton_clicked();
    void onCancelDocumentButton_clicked();
    void import_CSV();
    void export_CSV();
    void export_JSON();
    void exportProducts_CSV();
    void infoFON();
    void backupDEP();
    void backup();
    void runPlugin();
    void viewPlugins();

    void setSafetyDevice(bool active);
    void updateRKStatus();

    void actionAclManager();
    void actionAbout_QRK();
    void actionQRK_Forum();
    void actionLeaveDemoMode();
    void actionResetDemoData();
    void actionResuscitationCashRegister();
    void actionGastro();

    void fullScreenSlot();
    void setFullScreenMode(bool);
    void exitSlot();
    void logOnOff(bool);
    void restore();
    void newApplicationVersionAvailable(QString version);
    void sendDatagram(QString what, QString data);

protected slots:
    virtual void timerDone();

public slots:
    void setStatusBarProgressBar(int value, bool add);
    void setStatusBarProgressBarWait(bool on_off);

private:
    void DateTimeCheck();
    void closeEvent (QCloseEvent *event);
    Ui::MainWindow *ui;

    bool m_servermode;
    QString shopname();
    QLCDNumber *m_dateLcd;

    void restartApplication();

    QLabel *m_dep;
    QLabel *m_depPX;
    QLabel *m_safetyDevicePX;

    QProgressBar *m_progressBar;

    QRKHome *m_qrk_home;
    QRKRegister *m_qrk_register;
    QRKDocument *m_qrk_document;
    QRKGastro *m_qrk_gastro;
    QStackedWidget *m_stackedWidget;

    QTimer *m_timer;
    QDateTime m_checkDateTime;

    int m_currentRegisterYear;
    QString m_cashRegisterId;

    QString m_shopName;
    bool m_previousSafetyDeviceState;
    QThread *m_checkerThread;
    VersionChecker *m_versionChecker;
    QUdpSocket udpSocket;

};

#endif // MAINWIDGET_H
