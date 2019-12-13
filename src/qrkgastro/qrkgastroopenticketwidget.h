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

#ifndef QRKGASTROOPENTICKETWIDGET_H
#define QRKGASTROOPENTICKETWIDGET_H

#include <QFrame>
#include <QTreeWidget>

class QrkGastroOpenTicketWidget : public QFrame
{
  Q_OBJECT

  public:
    QrkGastroOpenTicketWidget(QWidget *parent, int theId, int theTable);

    bool isSelected() const { return selected; }
    void setSelected(bool sel = true);

    int getId() const { return id; }
    int getTable() const { return table; }

signals:
    void selectionChanged();

  private slots:
    void selectedSlot();
    void ordersServed();

  private:
    void resizeColumnsToContents(QTreeWidget &treeWidget);
    int id;
    int table;
    bool selected;
    QWidget *toServeWidget;
};

//--------------------------------------------------------------------------------
// helper to allow selection on clicking anywhere in tree

class TreeWidget : public QTreeWidget
{
  Q_OBJECT

  public:
    TreeWidget(QWidget *parent);

  signals:
    void selected();

  protected:
    virtual bool viewportEvent(QEvent *event);
};

#endif
