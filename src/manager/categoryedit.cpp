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

#include "categoryedit.h"
#include "database.h"
#include "utils/utils.h"
#include <ui_categoryedit.h>

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

//--------------------------------------------------------------------------------

CategoryEdit::CategoryEdit(QWidget *parent, int id)
  : QDialog(parent), ui(new Ui::CategoryEdit), m_id(id)
{
  ui->setupUi(this);

  const QStringList colorNames = QColor::colorNames();
  int index = 0;
  ui->colorComboBox->addItem(tr("Standard Farbe"));
  const QModelIndex idx = ui->colorComboBox->model()->index(index++, 0);
  ui->colorComboBox->model()->setData(idx, "", Qt::BackgroundColorRole);

  foreach (const QString &colorName, colorNames) {
      const QColor color(colorName);
      QString fg = Utils::color_best_contrast(color.name());
      const QColor fg_color(fg);
      ui->colorComboBox->addItem(colorName, color);
      const QModelIndex idx = ui->colorComboBox->model()->index(index++, 0);
      ui->colorComboBox->model()->setData(idx, color, Qt::BackgroundColorRole);
      ui->colorComboBox->model()->setData(idx, fg_color, Qt::ForegroundRole);
  }

  QJsonArray printers = Database::getPrinters();
  ui->printerComboBox->addItem(tr("kein Drucker"), 0);
  foreach (const QJsonValue & value, printers) {
      QJsonObject obj = value.toObject();
      ui->printerComboBox->addItem(obj["name"].toString(), obj["id"].toInt());
  }

  if ( m_id != -1 ) {
    QSqlDatabase dbc = Database::database();

    QSqlQuery query(QString("SELECT name, color, printerid FROM categories WHERE id=%1").arg(id), dbc);
    query.next();

    ui->name->setText(query.value(0).toString());

    int i;
    for (i = 0; i <= ui->colorComboBox->count(); i++) {
        QString color = ui->colorComboBox->model()->index(i, 0).data(Qt::BackgroundColorRole).toString();
        if ( query.value(2).toString() == color )
            break;
    }

    if (i > ui->colorComboBox->count())
      i = 0;

    QPalette palette(ui->colorComboBox->palette());
    QString colorValue = query.value(2).toString();
    if (!colorValue.isEmpty()) {
        QColor color(query.value(2).toString());
        palette.setColor(QPalette::Active,QPalette::Button, color);
        palette.setColor(QPalette::Highlight, color);
        //    palette.setColor(QPalette::ButtonText, Qt::white);
        ui->colorComboBox->setPalette(palette);
    }
    ui->colorComboBox->setCurrentIndex(i);

    for (i = 0; i <= ui->printerComboBox->count(); i++) {
        if ( query.value("printerid").toInt() == i )
            break;
    }

    if (i > ui->printerComboBox->count())
      i = 0;

    ui->printerComboBox->setCurrentIndex(i);

  }

  connect (ui->colorComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CategoryEdit::colorComboChanged);
  connect (ui->okButton, &QPushButton::clicked, this, &CategoryEdit::accept);
  connect (ui->cancelButton, &QPushButton::clicked, this, &CategoryEdit::reject);

}

CategoryEdit::~CategoryEdit()
{
    delete ui;
}

void CategoryEdit::colorComboChanged(int idx)
{

    QString colorValue = ui->colorComboBox->itemData(idx, Qt::BackgroundColorRole).toString(); // ->itemText(idx); //   ->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();
    QPalette palette(ui->colorComboBox->palette());
    QColor color(colorValue);
    palette.setColor(QPalette::Active,QPalette::Button, color);
    palette.setColor(QPalette::Highlight, color);
    ui->colorComboBox->setPalette(palette);
}

//--------------------------------------------------------------------------------

void CategoryEdit::accept()
{
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);

  QString color = ui->colorComboBox->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();

  if ( m_id == -1 ) {
    query.prepare(QString("INSERT INTO categories (name, color, printerid) VALUES(:name, :color, :printerid)"));
  } else {
    query.prepare(QString("UPDATE categories SET name=:name, color=:color, printerid=:printerid WHERE id=:id"));
    query.bindValue(":id", m_id);
  }

  int printerid = ui->printerComboBox->currentData().toInt();

  query.bindValue(":name", ui->name->text());
  query.bindValue(":color", color);
  if (printerid > 0)
    query.bindValue(":printerid", printerid);
  else
    query.bindValue(":printerid", QVariant(QVariant::Int));

  bool ok = query.exec();
  if (!ok) {
    qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
    qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
  }

  QDialog::accept();
}

//--------------------------------------------------------------------------------
