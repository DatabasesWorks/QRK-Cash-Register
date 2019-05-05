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

#include "checkablelistdialog.h"
#include "checkablelistmodel.h"

CheckableListDialog::CheckableListDialog(QWidget *parent): QDialog(parent)
{
    setWindowTitle(tr("CSV Export"));
    int nWidth = 350;
    int nHeight = 500;
    if (parent != NULL)
        setGeometry(parent->x() + parent->width()/2 - nWidth/2,
                    parent->y() + parent->height()/2 - nHeight/2,
                    nWidth, nHeight);
    else
        resize(nWidth, nHeight);

    m_view = new QListView;
    m_model = new CheckableListModel;
    m_view->setModel(m_model);
    m_viewBox = new QGroupBox(tr("Erforderliche Spalten"));
    m_buttonBox = new QDialogButtonBox;
    m_continueButton = m_buttonBox->addButton(QDialogButtonBox::Ok);
    m_continueButton->setText(tr("Weiter"));
    m_closeButton = m_buttonBox->addButton(QDialogButtonBox::Close);

    QVBoxLayout* viewLayout = new QVBoxLayout;
    viewLayout->addWidget(m_view);
    m_viewBox->setLayout(viewLayout);

    QHBoxLayout* horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(m_buttonBox);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_viewBox);
    mainLayout->addLayout(horizontalLayout);

    setLayout(mainLayout);

    connect(m_continueButton, &QPushButton::clicked, this, &CheckableListDialog::continueButtonClicked);
    connect(m_closeButton, &QPushButton::clicked, this, &CheckableListDialog::close);

    m_requiredList.clear();
}

void CheckableListDialog::setModelData(QStringList list, QStringList selected)
{
    foreach (QString text, list) {
        QStandardItem *item =new QStandardItem(text);
        item->setCheckable(true);
        item->setCheckState(selected.contains(text)? Qt::Checked: Qt::Unchecked);
        m_model->setItem(m_model->rowCount(), item);
    }
}

QStringList CheckableListDialog::getRequiredList()
{
    return m_requiredList;
}

void CheckableListDialog::continueButtonClicked()
{
    m_requiredList = m_model->getSelectedList();
    accept();
}
