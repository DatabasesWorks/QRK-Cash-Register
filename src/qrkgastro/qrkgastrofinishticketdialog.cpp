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
 * LillePOS parts Copyright 2010, Martin Koller, kollix@aon.at
 */

#include "defines.h"
#include "qrkgastrofinishticketdialog.h"
#include "3rdparty/qbcmath/bcmath.h"
#include "3rdparty/ckvsoft/rbac/acl.h"
#include "ui_qrkgastrofinishticketdialog.h"

#include <QStyle>

//--------------------------------------------------------------------------------

QRKGastroFinishTicketDialog::QRKGastroFinishTicketDialog(QBCMath sum, QWidget *parent)
    : QDialog(parent), ui(new Ui::QRKGastroFinishTicketDialog), m_origSum(sum)
{

    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    int iconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, Q_NULLPTR, this);
    QIcon tmpIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion, Q_NULLPTR, this));
    ui->icon->setPixmap(tmpIcon.pixmap(iconSize, iconSize));

    showSum(m_origSum);

    connect(ui->cashReceipt, &QPushButton::clicked, this, &QRKGastroFinishTicketDialog::cashTicket);
    connect(ui->debitcardReceipt, &QPushButton::clicked, this, &QRKGastroFinishTicketDialog::debitcardTicket);
    connect(ui->creditcardReceipt, &QPushButton::clicked, this, &QRKGastroFinishTicketDialog::creditcardTicket);

    connect(ui->privateTicket, &QPushButton::clicked, this, &QRKGastroFinishTicketDialog::privateTicket);
    connect(ui->employeeTicket, &QPushButton::clicked, this, &QRKGastroFinishTicketDialog::employeeTicket);
    connect(ui->advertisingTicket, &QPushButton::clicked, this, &QRKGastroFinishTicketDialog::advertisingTicket);

    layout()->setSizeConstraint(QLayout::SetFixedSize);

}

void QRKGastroFinishTicketDialog::cashTicket()
{
    done(CASHRECEIPT_TICKET );
}

void QRKGastroFinishTicketDialog::creditcardTicket()
{
    done(CREDITCARD_TICKET);
}

void QRKGastroFinishTicketDialog::debitcardTicket()
{
    done(DEBITCARD_TICKET);
}

void QRKGastroFinishTicketDialog::privateTicket()
{
    if(!RBAC::Instance()->hasPermission("gastro_private_ticked", true)) {
        return;
    }
    done(PRIVATE_TICKET);
}

void QRKGastroFinishTicketDialog::employeeTicket()
{
    if(!RBAC::Instance()->hasPermission("gastro_employee_ticked", true)) {
        return;
    }
    done(EMPLOYEE_TICKET);
}

void QRKGastroFinishTicketDialog::advertisingTicket()
{
    if(!RBAC::Instance()->hasPermission("gastro_advertising_ticked", true)) {
        return;
    }
    done(ADVERTISING_TICKET);
}

void QRKGastroFinishTicketDialog::showSum(QBCMath sum)
{
    ui->label->setText(tr("<html>Bon abschließen?<br>Nicht vergessen <font style='color:red;'>%1 %2</font> zu kassieren!</html>")
                       .arg(sum.toLocale()).arg(QLocale().currencySymbol()));
}
