#ifndef QRKGASTROQUICKPRODUCT_H
#define QRKGASTROQUICKPRODUCT_H

#include <QDialog>

namespace Ui {
class QrkGastroQuickProduct;
}

class QrkGastroQuickProduct : public QDialog
{
    Q_OBJECT

public:
    explicit QrkGastroQuickProduct(QWidget *parent = nullptr);
    ~QrkGastroQuickProduct();

    int getProductId();

private:
    Ui::QrkGastroQuickProduct *ui;
    void setTaxes();
    void setCompleter();
    void accept();

    int m_productId = 0;
};

#endif // QRKGASTROQUICKPRODUCT_H
