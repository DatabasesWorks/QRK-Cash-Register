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

#ifndef CSVIMPORTWIZARDPAGE4_H
#define CSVIMPORTWIZARDPAGE4_H

#include <QWizardPage>
#include <QMap>
#include <QStandardItemModel>

namespace Ui {
  class CsvImportWizardPage4;
}

class CsvImportWizardPage4 : public QWizardPage
{
    Q_OBJECT

  public:
    explicit CsvImportWizardPage4(QWidget *parent = Q_NULLPTR);
    ~CsvImportWizardPage4();
    bool isComplete() const;
    void setMap(QMap<QString, QJsonObject> *m);

  private slots:
    void importFinished();
    void cancelButton(bool);
    void newButton(bool);
    void changeButton(bool);
    void info(QString info);

  private:
    Ui::CsvImportWizardPage4 *ui;
    void initializePage();
    void setWidgetData();

    QMap<QString, QJsonObject> *m_errormap;
    bool m_autoGroup;
    bool m_guessGroup;
    bool m_ignoreExistingProduct;
    bool m_updateExistingProduct;
    QString m_importType;
    bool m_visibleGroup;
    bool m_visibleProduct;

};

#endif // CSVIMPORTWIZARDPAGE4_H
