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

#ifndef PRINTERDEFINITIONEDIT_H
#define PRINTERDEFINITIONEDIT_H

#include <QDialog>
#include <QSqlQueryModel>

#include <QJsonObject>

namespace Ui {
  class PrinterDefinitionEdit;
}

class PrinterDefinitionEdit : public QDialog
{
  Q_OBJECT

  public:
    PrinterDefinitionEdit(QWidget *parent, int theId = -1);
    ~PrinterDefinitionEdit();

  public slots:
    virtual void accept();

  private:
    Ui::PrinterDefinitionEdit *ui;
    void typeChanged(int idx);
    void typeChanged(const QString &text);

    void paperHeightChanged(int height);
    void paperWidthChanged(int width);
    void marginTopChanged(int margin);
    void marginLeftChanged(int margin);
    void marginRightChanged(int margin);
    void marginBottomChanged(int margin);

    void pdfChanged(bool checked);

    bool updateData(int id, const QString &name);

    int m_id;
    QString m_name;
    QJsonObject m_json;
};

#endif // PRINTERDDEFINITIONEDIT_H
