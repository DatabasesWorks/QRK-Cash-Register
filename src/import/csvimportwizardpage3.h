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

#ifndef CSVIMPORTWIZARDPAGE3_H
#define CSVIMPORTWIZARDPAGE3_H

#include "database.h"

#include <QWizardPage>
#include <QMap>
#include <QStandardItemModel>

namespace Ui {
  class CsvImportWizardPage3;
}

class CsvImportWizardPage3 : public QWizardPage
{
    Q_OBJECT

  public:
    explicit CsvImportWizardPage3(QWidget *parent = 0);
    ~CsvImportWizardPage3();
    bool isComplete() const;
    void setModel(QStandardItemModel *model);
    void setMap(QMap<QString, QVariant> *m);
    void setMap(QMap<QString, QJsonObject> *m);

signals:
    void removePage(int id);

private slots:
    void importFinished();
    void save(bool);
    void info(QString info);

  private:
    Ui::CsvImportWizardPage3 *ui;
    void initializePage();
    QStandardItemModel *m_model;
    QMap<QString, QVariant> *m_map;
    QMap<QString, QJsonObject> *m_errormap;
    bool m_autoGroup;
    bool m_guessGroup;
    bool m_ignoreExistingProduct;
    bool m_updateExistingProduct;
    QString m_importType;
    bool m_finished;
    bool m_visibleGroup;
    bool m_visibleProduct;
    bool m_autoitemnum;
    int  m_autominitemnum;

};

class ImportData : public QObject
{
    Q_OBJECT

  public:
    ImportData(QStandardItemModel *model, QMap<QString, QVariant> *map, QMap<QString, QJsonObject> *errormap, bool ignoreExistingProduct = false, bool guessGroup = false, bool autogroup = false,  bool visibleGroup = false, bool visibleProduct = false, bool updateExistingProduct = false, bool autoitemnum = true, int autominitemnum = Database::getNextProductNumber().toInt());
    ~ImportData();

    void run();

  signals:
    void percentChanged(int percent);
    void finished();
    void info(QString);

  private:
    QString getItemValue(int roe, int col, bool replace = false);
    int exists(QString itemnum, QString barcode, QString name, int &type);
    QString getGuessGroup(QString name);
    int createGroup(QString name);
    QString getGroupById(int id);
    int getGroupByName(QString name);

    QStandardItemModel *m_model;
    QMap<QString, QVariant> *m_map;
    QMap<QString, QJsonObject> *m_errormap;
    bool m_ignoreExistingProduct;
    bool m_updateExistingProduct;
    bool m_guessGroup;
    bool m_autoGroup;
    bool m_visibleGroup;
    bool m_visibleProduct;
    bool m_autoitemnum;
    int m_autominitemnum;
    double m_defaulttax;

};

#endif // CSVIMPORTWIZARDPAGE3_H
