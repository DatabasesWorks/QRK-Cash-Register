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

#ifndef QRKTIMEDMESSAGEBOX_H
#define QRKTIMEDMESSAGEBOX_H

#include "qrkpushbutton.h"
#include "qrkcore_global.h"

#include <QMessageBox>
#include <QTimer>

class QRK_EXPORT QrkTimedMessageBox : public QMessageBox
{
    Q_OBJECT

    public:
        QrkTimedMessageBox(int timeoutSeconds, Icon icon, const QString & title, const QString & text, StandardButtons buttons = NoButton, QWidget * parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::Dialog|Qt::MSWindowsFixedSizeDialogHint)
            : QMessageBox(icon, title, text, buttons, parent, flags)
            , m_timeoutSeconds(timeoutSeconds + 1)
        {
//            connect(&m_timer, SIGNAL(timeout()), this, SLOT(Tick()));
            connect(&m_timer, &QTimer::timeout, this, &QrkTimedMessageBox::Tick);
            m_timer.setInterval(1000);
        }
        QrkTimedMessageBox(int timeoutSeconds, const QString & title, const QString & text, Icon icon, int button0, int button1, int button2, QWidget * parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::Dialog|Qt::MSWindowsFixedSizeDialogHint)
            : QMessageBox(title, text, icon, button0, button1, button2, parent, flags)
            , m_timeoutSeconds(timeoutSeconds + 1)
        {
            connect(&m_timer, &QTimer::timeout, this, &QrkTimedMessageBox::Tick);
            m_timer.setInterval(1000);
        }

        virtual void showEvent(QShowEvent * e)
        {
            if (defaultButton())
                m_text = defaultButton()->text() + "\n" + QObject::tr("in %1 Sek.");
            QMessageBox::showEvent(e);
            Tick();
            m_timer.start();
        }

    private slots:
        void Tick()
        {
            if (defaultButton()) {
                if (--m_timeoutSeconds >= 0) {
                    defaultButton()->setText(m_text.arg(m_timeoutSeconds));
                } else {
                    m_timer.stop();
                    defaultButton()->animateClick();
                }
            } else {
                m_timer.stop();
                return;
            }
        }

    private:
        int m_timeoutSeconds;
        QString m_text;
        QTimer m_timer;
};

#endif // QRKTIMEDMESSAGEBOX_H
