#include "texteditdialog.h"
#include "qrkpushbutton.h"

#include <QVBoxLayout>

TextEditDialog::TextEditDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Beschreibungstext"));

    QVBoxLayout *vboxLayout = new QVBoxLayout(this);
    QVBoxLayout *editLayout = new QVBoxLayout();

    m_textEdit = new QTextEdit(this);
    editLayout->addWidget(m_textEdit);
    QHBoxLayout *hboxLayout = new QHBoxLayout();
    QSpacerItem *spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacer);
    QrkPushButton *saveButton = new QrkPushButton(this);
    saveButton->setIcon(QIcon(":src/icons/save.png"));
    saveButton->setIconSize(QSize(32,32));
    saveButton->setText(tr("Speichern"));
    hboxLayout->addWidget(saveButton);
    QrkPushButton *exitButton = new QrkPushButton(this);
    exitButton->setText(tr("Verlassen"));
    exitButton->setIcon(QIcon(":src/icons/cancel.png"));
    exitButton->setIconSize(QSize(32,32));
    hboxLayout->addWidget(exitButton);
    editLayout->addLayout(hboxLayout);

    vboxLayout->addLayout(editLayout);

    connect (saveButton, &QPushButton::clicked, this, &TextEditDialog::accept);
    connect (exitButton, &QPushButton::clicked, this, &TextEditDialog::close);

}

TextEditDialog::~TextEditDialog()
{
}

void TextEditDialog::setText(const QString &text)
{
    m_textEdit->setText(text);
}

QString TextEditDialog::getText()
{
    return m_textEdit->toPlainText();
}
