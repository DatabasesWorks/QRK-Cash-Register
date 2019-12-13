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

#include "qrkgastromanagerroomedit.h"
#include "database.h"
#include "utils/utils.h"
#include "ui_qrkgastromanagerroomedit.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

//--------------------------------------------------------------------------------

QRKGastroManagerRoomEdit::QRKGastroManagerRoomEdit(QWidget *parent, int id)
  : QDialog(parent), ui(new Ui::QRKGastroManagerRoomEdit), m_id(id)
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

  if ( m_id != -1 ) {
    QSqlDatabase dbc = Database::database();

    QSqlQuery query(QString("SELECT name, color, isHotel FROM rooms WHERE id=%1").arg(id), dbc);
    query.next();

    ui->name->setText(query.value("name").toString());

    int i;
    for (i = 0; i <= ui->colorComboBox->count(); i++) {
        QString color = ui->colorComboBox->model()->index(i, 0).data(Qt::BackgroundColorRole).toString();
        if ( query.value(2).toString() == color )
            break;
    }

    if (i > ui->colorComboBox->count())
      i = 0;

    ui->colorComboBox->setCurrentIndex(i);
    QPalette palette(ui->colorComboBox->palette());
    QString colorValue = query.value("color").toString();
    if (!colorValue.isEmpty()) {
        QColor color(query.value("color").toString());
        palette.setColor(QPalette::Active,QPalette::Button, color);
        palette.setColor(QPalette::Highlight, color);
        //    palette.setColor(QPalette::ButtonText, Qt::white);
        ui->colorComboBox->setPalette(palette);
    }

    ui->isHotel->setChecked(query.value("isHotel").toBool());

  }

  connect (ui->colorComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QRKGastroManagerRoomEdit::colorComboChanged);
  connect (ui->okButton, &QPushButton::clicked, this, &QRKGastroManagerRoomEdit::accept);
  connect (ui->cancelButton, &QPushButton::clicked, this, &QRKGastroManagerRoomEdit::reject);

}

QRKGastroManagerRoomEdit::~QRKGastroManagerRoomEdit()
{
    delete ui;
}

void QRKGastroManagerRoomEdit::colorComboChanged(int idx)
{

    QString colorValue = ui->colorComboBox->itemData(idx, Qt::BackgroundColorRole).toString(); // ->itemText(idx); //   ->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();
    QPalette palette(ui->colorComboBox->palette());
    QColor color(colorValue);
    palette.setColor(QPalette::Active,QPalette::Button, color);
    palette.setColor(QPalette::Highlight, color);
    ui->colorComboBox->setPalette(palette);
}

//--------------------------------------------------------------------------------

void QRKGastroManagerRoomEdit::accept()
{
  QSqlDatabase dbc = Database::database();
  QSqlQuery query(dbc);

  QString color = ui->colorComboBox->model()->index(ui->colorComboBox->currentIndex(), 0).data(Qt::BackgroundColorRole).toString();

  if ( m_id == -1 ) {   // new entry
    query.prepare(QString("INSERT INTO rooms (name, color, isHotel) VALUES(:name, :color, :isHotel)"));
  } else {
    query.prepare(QString("UPDATE rooms SET name=:name, color=:color, isHotel=:isHotel WHERE id=:id"));
    query.bindValue(":id", m_id);
  }

  query.bindValue(":name", ui->name->text());
  query.bindValue(":color", color);
  query.bindValue(":isHotel", ui->isHotel->isChecked());

  bool ok = query.exec();
  if (!ok) {
    qWarning() << "Function Name: " << Q_FUNC_INFO << " Error: " << query.lastError().text();
    qWarning() << "Function Name: " << Q_FUNC_INFO << " Query: " << Database::getLastExecutedQuery(query);
  }

  QDialog::accept();
}

//--------------------------------------------------------------------------------
