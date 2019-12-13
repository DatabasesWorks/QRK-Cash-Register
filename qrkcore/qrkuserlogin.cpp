#include "qrkuserlogin.h"
#include "preferences/qrksettings.h"

QrkUserLogin::QrkUserLogin(QWidget* parent)
    : UserLogin(parent)
{
    QrkSettings settings;
    settings.beginGroup("ckvsoft");
    bool saveUserName = settings.value("savelastUserName", false).toBool();
    QString userName = settings.value("lastUserName", "").toString();
    settings.endGroup();

    if (saveUserName) {
        userLineEdit->setText(userName);
        passwordLineEdit->setFocus();
    } else {
        userLineEdit->setFocus();
    }

    savePasswordCheckBox->setChecked(saveUserName);

}

void QrkUserLogin::OnLogin()
{
    QString username = userLineEdit->text();
    SecureByteArray password = passwordLineEdit->text().toUtf8();

    Crypto crypto;
    QString cryptPassword = crypto.encrypt(password);
    QString cryptPassword2 = RBAC::Instance()->getPasswordByUserName(username);

//    cryptPassword2 = cryptPassword;
    // Checking if username or password is empty
    if (username.isEmpty() || password.isEmpty())
        QMessageBox::warning(this, tr("Information!"), tr("Benutzername oder Kennwort darf nicht leer sein"));
    else if (cryptPassword2.isEmpty() || cryptPassword.compare(cryptPassword2) != 0)
        QMessageBox::critical(this, tr("Information!"), tr("Benutzername oder Kennwort falsch."));
    else {
        QrkSettings settings;
        settings.beginGroup("ckvsoft");
        if (savePasswordCheckBox->isChecked()) {
            settings.save2Settings("savelastUserName", true, false);
            settings.save2Settings("lastUserName", userLineEdit->text(), false);
        } else {
            settings.removeSettings("savelastUserName", false);
            settings.removeSettings("lastUserName", false);
        }
        settings.endGroup();

        RBAC::Instance()->setuserId(RBAC::Instance()->getUserIdByName(username));
        QDialog::accept();
        QDialog::close();
    }
}
