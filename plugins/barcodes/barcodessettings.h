#ifndef BARCODESSETTINGS_H
#define BARCODESSETTINGS_H


#include <QWidget>
#include <QLineEdit>

class BarcodesSettings : public QWidget
{
    Q_OBJECT

public:
    explicit BarcodesSettings(QWidget *parent = 0);

private:
    QLineEdit *m_barcode_finishReceipt;
    QLineEdit *m_barcode_removeLastPosition;
    QLineEdit *m_barcode_endOfDay;
    QLineEdit *m_barcode_discount;
    QLineEdit *m_barcode_editPrice;

    QLineEdit *m_barcode_printLastReceiptAgain;
    QLineEdit *m_barcode_cancelLastReceipt;

    QLineEdit *m_barcode_amount_0;
    QLineEdit *m_barcode_amount_1;
    QLineEdit *m_barcode_amount_2;
    QLineEdit *m_barcode_amount_3;
    QLineEdit *m_barcode_amount_4;
    QLineEdit *m_barcode_amount_5;
    QLineEdit *m_barcode_amount_6;
    QLineEdit *m_barcode_amount_7;
    QLineEdit *m_barcode_amount_8;
    QLineEdit *m_barcode_amount_9;
    QLineEdit *m_barcode_amount_00;
    QLineEdit *m_barcode_amount_000;
    QLineEdit *m_barcode_amount_250;
    QLineEdit *m_barcode_amount_500;

};


#endif // BARCODESSETTINGS_H
