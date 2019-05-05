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

#ifndef CHECKABLELISTDIALOG_H
#define CHECKABLELISTDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QListView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

class CheckableListModel;

class CheckableListDialog : public QDialog
{
    Q_OBJECT

public:
    CheckableListDialog(QWidget *parent = 0);
    void setModelData(QStringList list, QStringList selected);
    QStringList getRequiredList();

public slots:
    void continueButtonClicked();

private:
    CheckableListModel* m_model;
    QListView* m_view;
    QDialogButtonBox* m_buttonBox;
    QGroupBox* m_viewBox;
    QPushButton* m_continueButton;
    QPushButton* m_closeButton;
    QStringList m_requiredList;

};

#endif // CHECKABLELISTDIALOG_H
