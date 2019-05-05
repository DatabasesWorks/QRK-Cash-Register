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

#ifndef QRKREGISTER_H
#define QRKREGISTER_H

#include "receiptitemmodel.h"
#include "preferences/qrksettings.h"
#include "pluginmanager/Interfaces/barcodesinterface.h"
#include "defines.h"

class QButtonGroup;

namespace Ui {
  class QRKRegister;
}

class QRKRegister : public QWidget
{
    Q_OBJECT
  public:
    explicit QRKRegister(QWidget *parent = 0);
    ~QRKRegister();
    void init();
    void newOrder();
    void clearModel();

  signals:
    void cancelRegisterButton_clicked();
    void finishedReceipt();
    void fullScreen(bool);

  public slots:
    void safetyDevice(bool active);
    void singlePriceChanged(QString product, QString singleprice, QString tax);

  private slots:
    void barcodeChangedSlot();
    void plusSlot();
    void minusSlot();
    void onButtonGroup_payNow_clicked(int payedBy);
    void quickProductButtons(int id);
    void addProductToOrderList(int id);
    void receiptToInvoiceSlot();
    void onCancelRegisterButton_clicked();
    void setButtonGroupEnabled(bool enabled);
    void finishedItemChanged();
    void finishedPlus();
    void createCheckReceipt(bool);
    void setColumnHidden(int col);
    void splitterMoved(int pos, int idx);
    void numPadToogle(bool);
    void barcodeFinderButton_clicked(bool);

  protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void keyPressEvent(QKeyEvent *event);

  private:
    Ui::QRKRegister *ui;

    void updateOrderSum();
    void setCurrentReceiptNum(int id);
    void quickGroupButtons();
    void handleAmount(QString amount);
    void initAmount();
    void resetAmount();
    void initAppendType();
    void init(int col, QString val);
    void appendToAmount(QString digits);
    void appendToPrice(QString digits);

    bool finishReceipts(int, int = 0, bool = false);

    void initPlugins();
    void categoryButton(bool clicked);
    void writeSettings();
    void readSettings();
    void writeHeaderColumnSettings(int, int, int);
    void readHeaderColumnSettings();
    void upPushButton(bool clicked);
    void downPushButton(bool clicked);
    void setButtonsHidden();

    void numPadValueButtonPressed(const QString &text, REGISTER_COL column);

    BarcodesInterface *barcodesInterface = 0;

    int m_currentReceipt;
    ReceiptItemModel *m_orderListModel;

    bool m_useInputNetPrice;
    bool m_useDiscount;
    bool m_useMaximumItemSold;
    bool m_useDecimalQuantity;
    bool m_usePriceChangedDialog;
    bool m_useGivenDialog;
    bool m_isR2B;
    bool m_receiptPrintDialog;
    bool m_minstockDialog;
    bool m_useInputProductNumber;
    bool m_registerHeaderMoveable;

    bool m_orderlistTaxColumnHidden = false;
    bool m_orderlistSinglePriceColumnHidden = false;

    bool m_barcodeInputLineEditDefault = false;

    QButtonGroup *m_buttonGroupGroups;
    QButtonGroup *m_buttonGroupProducts;

    int m_barcodeReaderPrefix;
    int m_decimaldigits = 2;
    int m_maximumWindowsHeight;
    QSize m_quickButtonSize;

};

#endif // REGISTER_H
