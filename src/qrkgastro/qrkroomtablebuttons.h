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

#ifndef QRKROOMTABLEBUTTONS_H
#define QRKROOMTABLEBUTTONS_H

#include "3rdparty/ckvsoft/quickbuttons.h"

class DragPushButton;

class QrkRoomTableButtons : public QuickButtons
{
    Q_OBJECT

public:
    explicit QrkRoomTableButtons(QWidget *parent = Q_NULLPTR);
    DragPushButton *getRoomButton(int id);
    DragPushButton *getTableButton(int id);

signals:
    void tableOrder(int id);

public slots:
    void backToTopButton(bool clicked) override;
    void backToMiddleButton(bool clicked) override;
    void quickBottomButtons(int id) override;

private:
    void quickTopButtons() override;
    void quickMiddleButtons(int id = 1) override;
    void readSettings() override;
    int m_currentRoomId = 0;
    int m_currentTableId = 0;
};

#endif // QRKROOMTABLEBUTTONS_H
