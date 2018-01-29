#include "resetpassword.h"

ResetPassword::ResetPassword(int userId, QWidget *parent) : base_login(parent), m_userid(userId)
{
    setWindowTitle(tr("Passwort für %1 ändern").arg(RBAC::Instance()->getUsername(m_userid)));

    loginLabel->setText(tr("Neues Kennwort"));
    passwordLabel->setText(tr("Neues Kennwort (wiederholen)"));
    userLineEdit->setEchoMode(QLineEdit::Password);
    userLineEdit->setFocus();
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    okPushButton->setText(tr("Ändern"));
    cancelPushButton->setText(tr("Zurück"));

    savePasswordCheckBox->setVisible(false);
    windowNameLabel->setText(tr("Kennwort ändern"));
    windowCommentLabel->setText("ACHTUNG: Änderungen müssen danach noch im Manager gespeichert werden.");
    windowCommentLabel->setWordWrap(true);
    programVersionLabel->setText("");
    programVersionNoLabel->setText("");
    logoLabel->setPixmap(QPixmap(":src/icons/password.png").scaled(48, 48, Qt::KeepAspectRatio));

    connect(cancelPushButton, &QPushButton::clicked, this, &ResetPassword::reject);
    connect(okPushButton, &QPushButton::clicked, this, &ResetPassword::OnChange);

}

ResetPassword::~ResetPassword()
{
}

void ResetPassword::OnChange()
{
    m_password1 = "";
    m_password2 = "";

    SecureByteArray password = userLineEdit->text().toUtf8();
    SecureByteArray password2 = passwordLineEdit->text().toUtf8();

    Crypto crypto;
    QString cryptPassword = crypto.encrypt(password);
    QString cryptPassword2 = crypto.encrypt(password2);

    if (password.isEmpty() || password2.isEmpty())
        QMessageBox::warning(this, tr("Information!"), tr("Kennwort Felder dürfen nicht leer sein"));
    else if (cryptPassword.compare(cryptPassword2) != 0)
        QMessageBox::critical(this, tr("Information!"), tr("Die eingegebenen Kennwörter sind unterschiedlich."));
    else {
        m_password1 = password;
        m_password2 = password2;
        QDialog::accept();
        QDialog::close();
    }
}

void ResetPassword::getPassword(QString &pw1, QString &pw2)
{
    pw1 = m_password1;
    pw2 = m_password2;
}
