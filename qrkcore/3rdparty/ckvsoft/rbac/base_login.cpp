/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2018 Christian Kvasny <chris@ckvsoft.at>
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

#include "base_login.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

base_login::base_login(QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint)
{
    setModal(true);
    resize(500, 228);
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setMouseTracking(true);
    headerFrame->setAutoFillBackground(false);
    headerFrame->setStyleSheet("background-color:white;");
    headerFrame->setFrameShape(QFrame::StyledPanel);
    QGridLayout *gridLayout = new QGridLayout(headerFrame);
    gridLayout->setHorizontalSpacing(6);
    gridLayout->setVerticalSpacing(2);
    windowIconLabel = new QLabel(headerFrame);
    windowIconLabel->setMinimumSize(QSize(64, 64));
    windowIconLabel->setPixmap(QPixmap(":src/icons/logo_mini.png").scaled(64, 64, Qt::KeepAspectRatio));

    gridLayout->addWidget(windowIconLabel, 0, 0, 3, 1);

    windowNameLabel = new QLabel(headerFrame);
    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(windowNameLabel->sizePolicy().hasHeightForWidth());
    windowNameLabel->setSizePolicy(sizePolicy);
    QFont font;
    font.setBold(true);
    font.setWeight(75);
    windowNameLabel->setFont(font);
    windowNameLabel->setOpenExternalLinks(false);

    gridLayout->addWidget(windowNameLabel, 0, 1, 1, 2);

    windowCommentLabel = new QLabel(headerFrame);
    sizePolicy.setHeightForWidth(windowCommentLabel->sizePolicy().hasHeightForWidth());
    windowCommentLabel->setSizePolicy(sizePolicy);
    windowCommentLabel->setOpenExternalLinks(false);

    gridLayout->addWidget(windowCommentLabel, 1, 1, 1, 2);

    programVersionLabel = new QLabel(headerFrame);
    QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(programVersionLabel->sizePolicy().hasHeightForWidth());
    programVersionLabel->setSizePolicy(sizePolicy1);
    programVersionLabel->setMaximumSize(QSize(150, 16777215));

    gridLayout->addWidget(programVersionLabel, 2, 1, 1, 1);

    programVersionNoLabel = new QLabel(headerFrame);
    sizePolicy.setHeightForWidth(programVersionNoLabel->sizePolicy().hasHeightForWidth());
    programVersionNoLabel->setSizePolicy(sizePolicy);

    gridLayout->addWidget(programVersionNoLabel, 2, 2, 1, 1);

    verticalLayout->addWidget(headerFrame);

    QFrame *topLine = new QFrame(this);
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);

    verticalLayout->addWidget(topLine);

    QFrame *bodyFrame = new QFrame(this);
    QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(bodyFrame->sizePolicy().hasHeightForWidth());
    bodyFrame->setSizePolicy(sizePolicy2);
    bodyFrame->setFrameShape(QFrame::StyledPanel);
    bodyFrame->setFrameShadow(QFrame::Plain);
    QGridLayout *gridLayout1 = new QGridLayout(bodyFrame);
    gridLayout1->setSizeConstraint(QLayout::SetDefaultConstraint);
    logoLabel = new QLabel(bodyFrame);
    logoLabel->setMinimumSize(QSize(48, 48));
    logoLabel->setMaximumSize(QSize(48, 48));
    logoLabel->setPixmap(QPixmap(":src/icons/login.png"));
    logoLabel->setScaledContents(false);
    logoLabel->setTextInteractionFlags(Qt::NoTextInteraction);

    gridLayout1->addWidget(logoLabel, 0, 0, 1, 1);

    QSpacerItem *horizontalSpacer_2 = new QSpacerItem(20, 54, QSizePolicy::Minimum, QSizePolicy::Minimum);

    gridLayout1->addItem(horizontalSpacer_2, 0, 1, 1, 1);

    QGridLayout *gridLayout_2 = new QGridLayout();
    gridLayout_2->setSizeConstraint(QLayout::SetNoConstraint);
    gridLayout_2->setHorizontalSpacing(13);
    gridLayout_2->setVerticalSpacing(2);

    loginLabel = new QLabel(bodyFrame);
    sizePolicy.setHeightForWidth(loginLabel->sizePolicy().hasHeightForWidth());
    loginLabel->setSizePolicy(sizePolicy);
    loginLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    gridLayout_2->addWidget(loginLabel, 0, 0, 1, 1);

    userLineEdit = new QLineEdit(bodyFrame);
    QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(userLineEdit->sizePolicy().hasHeightForWidth());
    userLineEdit->setSizePolicy(sizePolicy3);
    userLineEdit->setMinimumSize(QSize(130, 0));
    userLineEdit->setContextMenuPolicy(Qt::NoContextMenu);
    userLineEdit->setMaxLength(16);

    gridLayout_2->addWidget(userLineEdit, 0, 1, 1, 1);

    passwordLabel = new QLabel(bodyFrame);
    sizePolicy.setHeightForWidth(passwordLabel->sizePolicy().hasHeightForWidth());
    passwordLabel->setSizePolicy(sizePolicy);
    passwordLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    gridLayout_2->addWidget(passwordLabel, 1, 0, 1, 1);

    passwordLineEdit = new QLineEdit(bodyFrame);
    passwordLineEdit->setEnabled(true);
    sizePolicy3.setHeightForWidth(passwordLineEdit->sizePolicy().hasHeightForWidth());
    passwordLineEdit->setSizePolicy(sizePolicy3);
    passwordLineEdit->setMinimumSize(QSize(130, 0));
    passwordLineEdit->setContextMenuPolicy(Qt::NoContextMenu);
    passwordLineEdit->setMaxLength(16);
    passwordLineEdit->setEchoMode(QLineEdit::Password);

    gridLayout_2->addWidget(passwordLineEdit, 1, 1, 1, 1);

    savePasswordCheckBox = new QCheckBox(bodyFrame);
    savePasswordCheckBox->setLayoutDirection(Qt::LeftToRight);

    gridLayout_2->addWidget(savePasswordCheckBox, 2, 1, 1, 1);

    label = new QLabel(bodyFrame);
    label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    gridLayout_2->addWidget(label, 2, 0, 1, 1);

    gridLayout1->addLayout(gridLayout_2, 0, 2, 2, 1);

    QSpacerItem *verticalSpacer = new QSpacerItem(20, 17, QSizePolicy::Minimum, QSizePolicy::Minimum);

    gridLayout1->addItem(verticalSpacer, 1, 0, 1, 2);

    QSpacerItem *spacerItem = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

    gridLayout1->addItem(spacerItem, 2, 2, 1, 1);

    verticalLayout->addWidget(bodyFrame);

    QFrame *bottomLine = new QFrame(this);
    bottomLine->setFrameShape(QFrame::HLine);
    bottomLine->setFrameShadow(QFrame::Sunken);

    verticalLayout->addWidget(bottomLine);

    QFrame *footerFrame = new QFrame(this);
    footerFrame->setFrameShape(QFrame::StyledPanel);
    QHBoxLayout *hboxLayout = new QHBoxLayout(footerFrame);
    hboxLayout->setContentsMargins(6, 6, 6, 6);

    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout->addItem(horizontalSpacer);

    okPushButton = new QPushButton(footerFrame);
    okPushButton->setMinimumHeight(60);
    okPushButton->setIconSize(QSize(32,32));
    QIcon icon;
    icon.addFile(":src/icons/ok.png", QSize(), QIcon::Normal, QIcon::Off);
    okPushButton->setIcon(icon);

    hboxLayout->addWidget(okPushButton);

    cancelPushButton = new QPushButton(footerFrame);
    cancelPushButton->setMinimumHeight(60);
    cancelPushButton->setIconSize(QSize(32,32));
    QIcon icon2;
    icon2.addFile(":src/icons/cancel.png", QSize(), QIcon::Normal, QIcon::Off);
    cancelPushButton->setIcon(icon2);

    hboxLayout->addWidget(cancelPushButton);

    verticalLayout->addWidget(footerFrame);

    loginLabel->setBuddy(userLineEdit);
    passwordLabel->setBuddy(passwordLineEdit);

    QWidget::setTabOrder(okPushButton, cancelPushButton);
    QWidget::setTabOrder(cancelPushButton, userLineEdit);
    QWidget::setTabOrder(userLineEdit, passwordLineEdit);
    QWidget::setTabOrder(passwordLineEdit, savePasswordCheckBox);

}

base_login::~base_login()
{

}
