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

#include "dragpushbutton.h"
#include <QMouseEvent>
#include <QPoint>
#include <QStyle>

DragPushButton::DragPushButton(QWidget *parent) : QToolButton(parent)
{
    initialize();
}

DragPushButton::DragPushButton(const QIcon &icon, const QString &text, QWidget *parent) : QToolButton(parent)
{
    QToolButton::setIcon(icon);
    QToolButton::setText(text);
    initialize();
}

DragPushButton::DragPushButton(const QString &text, QWidget *parent) : QToolButton(parent)
{
    QToolButton::setText(text);
    initialize();
}

void DragPushButton::initialize()
{
    QSize size(150, 60);
    QToolButton::setFixedHeight(size.height());
    QToolButton::setMinimumWidth(size.width());

    m_flashenabled = false;
    m_timer.setInterval(500);
    connect(&m_timer, &QTimer::timeout, this, &DragPushButton::timeout, Qt::DirectConnection);
    setAttribute(Qt::WA_DeleteOnClose);
    QByteArray ba;
/*
    setStyleSheet(
                "QToolButton {"
                "margin: 3px;"
                "border-color: black;"
                "border-style: outset;"
                "border-radius: 3px;"
                "border-width: 1px;"
                "background-color: #757575;"
                "}"
                "QToolButton:pressed {"
                "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #c5c5c5, stop: 1 #555555);"
                "}"
                );
*/
    connect(&m_flashtimer, &QTimer::timeout,this, &DragPushButton::flash);
    m_flashtimer.start(500);
}

void DragPushButton::setMinimumSize(const QSize newsize)
{
    QSize size(150, 60);
    size.setWidth(newsize.width());
    QToolButton::setMinimumSize(size);
}

void DragPushButton::mousePressEvent(QMouseEvent *event)
{
    if(!m_fixed && event->button() == Qt::LeftButton) {
        m_pos = event->pos();
        m_timer.start();
    }
    QToolButton::mousePressEvent(event);
}

void DragPushButton::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_fixed && event->button() == Qt::LeftButton) {
        if (m_timer.isActive()) {
            m_timer.stop();
        } else {
            event->ignore();
            return;
        }
    }
    QToolButton::mouseReleaseEvent(event);
}

void DragPushButton::timeout()
{
    m_timer.stop();
    if (m_pos != QPoint())
        emit mouseLongPressEvent(pos());

    m_pos = QPoint();
}

void DragPushButton::setFixedButton(bool fixed)
{
    m_fixed = fixed;
}

bool DragPushButton::isFixedButton()
{
    return m_fixed;
}

void DragPushButton::setId(int id)
{
    m_buttonid = id;
}

int DragPushButton::getId()
{
    return m_buttonid;
}

void DragPushButton::setButtonColor(const QString &color)
{
    if (m_firstStyleSheet.isEmpty())
        m_firstStyleSheet = styleSheet();

    QString stylesheet = m_firstStyleSheet + QString("QToolButton[flashing=\"true\"] {"
                                         "background-color: %1;"
                                         "}").arg(color);

    QToolButton::setStyleSheet(stylesheet);
}

void DragPushButton::restoreButtonColor()
{
    if (m_firstStyleSheet.isEmpty())
        m_firstStyleSheet = styleSheet();

    QToolButton::setStyleSheet(m_firstStyleSheet);
}

void DragPushButton::setBorderColor(const QString &color)
{
    if (m_firstStyleSheet.isEmpty())
        m_firstStyleSheet = styleSheet();

    QString stylesheet = m_firstStyleSheet + QString("QToolButton {"
                                                     "border-color: %1;"
                                                     "border-style: inset;"
                                                     "border-width: 2px;}").arg(color);

    QToolButton::setStyleSheet(stylesheet);
}

void DragPushButton::restoreBorderColor()
{
    if (m_firstStyleSheet.isEmpty())
        m_firstStyleSheet = styleSheet();

    QToolButton::setStyleSheet(m_firstStyleSheet);
}

void DragPushButton::setText(const QString &text)
{
    m_buttontext = text;
    QToolButton::setText(text);
}

void DragPushButton::setPriceText(const QString &pricetext)
{
    QToolButton::setText(m_buttontext + "\n" + pricetext);
}

void DragPushButton::setFlashEnabled(bool enable)
{
    m_flashenabled = enable;
}

void DragPushButton::flash()
{
    if (m_flashenabled) {
        if (m_flashState) {
            m_flashState = false;
            this->setProperty("flashing", true);
        } else {
            m_flashState = true;
            this->setProperty("flashing", false);
        }
        style()->unpolish(this);
        style()->polish(this);
    }
    else { // don't flash
        if (!m_flashState) {
            this->setProperty("flashing", true);
            style()->unpolish(this);
            style()->polish(this);
        }
    }
}
