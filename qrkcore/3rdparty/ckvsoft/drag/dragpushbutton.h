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

#ifndef DRAGPUSHBUTTON_H
#define DRAGPUSHBUTTON_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"

#include <QToolButton>
#include <QTimer>

class CKVSOFT_EXPORT DragPushButton : public QToolButton
{
    Q_OBJECT

public:
    DragPushButton(QWidget *parent = Q_NULLPTR);
    DragPushButton(const QString &text, QWidget *parent = Q_NULLPTR);
    DragPushButton(const QIcon &icon, const QString &text, QWidget *parent = Q_NULLPTR);

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void setText(const QString &text);
    void setPriceText(const QString &pricetext);

    void setMinimumSize(const QSize);
    void setFixedButton(bool fixed);
    bool isFixedButton();
    void setId(int id);
    int getId();
    void setButtonColor(const QString &color);
    void restoreButtonColor();
    void setBorderColor(const QString &color);
    void restoreBorderColor();
    void setFlashEnabled(bool enable);

signals:
    void mouseLongPressEvent(QPoint pos);

private:
    void initialize();
    void timeout();
    void flash();
    QTimer m_timer;
    QPoint m_pos;
    bool m_fixed = false;
    int m_buttonid = 0;
    QString m_firstStyleSheet;
    QString m_buttontext;
    bool m_flashState;
    bool m_flashenabled;
    QTimer m_flashtimer;
};

#endif // DRAGPUSHBUTTON_H
