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

#include "preferences/qrksettings.h"
#include "qrkpushbutton.h"

QrkPushButton::QrkPushButton(QWidget *parent) : QPushButton(parent)
{
    initialize();
}

QrkPushButton::QrkPushButton(const QIcon &icon, const QString &text, QWidget *parent) : QPushButton(icon, text, parent)
{
    initialize();
    QPushButton::setIconSize(QSize(32,32));
}

QrkPushButton::QrkPushButton(const QString &text, QWidget *parent) : QPushButton(text, parent)
{
    initialize();
}

void QrkPushButton::setMinimumSize(const QSize newsize)
{
    QrkSettings settings;
    QSize size = settings.value("ButtonSize", QSize(150, 60)).toSize();
    size.setWidth(newsize.width());
    QPushButton::setMinimumSize(size);
}

void QrkPushButton::initialize()
{
    QrkSettings settings;
    QSize size = settings.value("ButtonSize", QSize(150, 60)).toSize();
    QPushButton::setFixedHeight(size.height());
    QPushButton::setMinimumWidth(size.width());
}
