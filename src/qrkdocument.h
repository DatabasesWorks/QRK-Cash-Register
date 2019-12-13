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

#ifndef QRKDOCUMENT_H
#define QRKDOCUMENT_H

#include "qsortfiltersqlquerymodel.h"

#include <QWidget>
#include <QItemSelection>

namespace Ui {
  class QRKDocument;
}

class QRKDocument : public QWidget
{
    Q_OBJECT

  public:

    explicit QRKDocument(QWidget *parent = Q_NULLPTR);
    ~QRKDocument();
    void documentList(bool);

  signals:
    void cancelDocumentButton();
    void documentButton_clicked();

  private slots:
    void onCancellationButton_clicked();
    void onPrintcopyButton_clicked(bool = false);
    void onInvoiceCompanyButton_clicked();
    void sortChanged();
    void toogleButton_clicked();

  protected slots:
    void onDocumentSelectionChanged(const QItemSelection &, const QItemSelection &);
    void cancelDocumentButton_clicked();


  private:
    Ui::QRKDocument *ui;
    QSqlQueryModel *m_documentContentModel = Q_NULLPTR;
    QSortFilterSqlQueryModel *m_documentListModel = Q_NULLPTR;

    int m_currentReceipt;
    bool m_receiptPrintDialog;
    bool m_servermode;
    bool m_hiddeproductnumber;
    bool m_report_by_productgroup = false;
};

#endif // QRKDOCUMENT_H
