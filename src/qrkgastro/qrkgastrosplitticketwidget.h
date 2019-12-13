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

#ifndef QRKGASTROSPLITTICKETWIDEGET_H
#define QRKGASTROSPLITTICKETWIDEGET_H

#include <QDialog>
#include <QEventLoop>

class QTreeWidget;
class QTreeWidgetItem;
class History;
class QSqlQueryModel;

namespace Ui {
class QRKGastroSplitTicketWidget;
}

class QRKGastroSplitTicketWidget : public QDialog
{
  Q_OBJECT

  public:
    QRKGastroSplitTicketWidget(bool moveTable, QWidget *parent);

    enum ReturnValue
    {
      MOVED,
      SPLITTED,
      CANCELED
    };

    // @ticket holds the selected ticket id as input and the new ticket id as output
    using QDialog::exec;
    using QDialog::done;
    ReturnValue exec(int &ticket, int table);

  private slots:
    void toNew(QTreeWidgetItem *selected, int column);
    void fromNew(QTreeWidgetItem *selected, int column);
    void done();
    void cancel();

  private:  // methods
    static QTreeWidgetItem *findSameOrder(QTreeWidget *tree, const QTreeWidgetItem *order);
    static void moveItem(QTreeWidgetItem *selected, QTreeWidget *target);

  private:  // data
    Ui::QRKGastroSplitTicketWidget *ui;
    int getNewTableId(int table);
    void tableData(int id);

    QEventLoop eventLoop;
    bool m_movetable = false;
    QSqlQueryModel *m_roomModel;
    QSqlQueryModel *m_tableModel;

};

#endif // QRKGASTROSPLITTICKETWIDEGET_H
