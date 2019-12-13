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

#ifndef PRINTERSETTINGEDIT_H
#define PRINTERSETTINGEDIT_H

#include <QDialog>
#include <QSqlQueryModel>

#include <QJsonArray>

class QJsonTableModel;

namespace Ui {
  class PrinterSettingEdit;
}

class PrinterSettingEdit : public QDialog
{
  Q_OBJECT

  public:
    PrinterSettingEdit(const QStringList &availablePrinters, QWidget *parent, int theId = -1);
    ~PrinterSettingEdit();

  public slots:
    virtual void accept();

  private:
    Ui::PrinterSettingEdit *ui;
    void textChanged(const QString &text);

    void addNew();
    void remove();

    bool updateData(int id, const QString &name);
    void setDefinitionNames();

    int m_id;
    QJsonTableModel *m_model;
    QString m_name;
    QJsonArray m_json;
};

#endif // PRINTERSETTINGEDIT_H
