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
 * LillePOS parts Copyright 2010, Martin Koller, kollix@aon.at
*/

#ifndef QRKGASTROOPENTICKETSLISTWIDGET_H_
#define QRKGASTROOPENTICKETSLISTWIDGET_H_

#include <QScrollArea>
#include <QList>

class QrkGastroOpenTicketsListWidget : public QScrollArea
{
  Q_OBJECT

  public:
    explicit QrkGastroOpenTicketsListWidget(QWidget *parent = Q_NULLPTR);
    // call to requery and show open tickets
    void refreshTickets(int tableid);
    void selectTicket(int id);
    // return list of selected ticket ids; can be empty
    QList<int> getSelectedTickets() const;
    // return list of displayed ticket ids; can be empty
    QList<int> getTickets() const;
    int getTableOfTicket(int id);

signals:
    void selectionChanged();
};

#endif // QRKGASTROOPENTICKETSLISTWIDGET_H
