#ifndef TEXTEDITDIALOG_H
#define TEXTEDITDIALOG_H

#include "globals_ckvsoft.h"

#include <QDialog>
#include <QTextEdit>

class CKVSOFT_EXPORT TextEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TextEditDialog(QWidget *parent = Q_NULLPTR);
    ~TextEditDialog();

    void setText(const QString &text);
    QString getText();

private:
    QTextEdit *m_textEdit;
};

#endif // TEXTEDITDIALOG_H
