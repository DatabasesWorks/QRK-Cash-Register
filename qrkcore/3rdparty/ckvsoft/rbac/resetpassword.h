#ifndef RESETPASSWORD_H
#define RESETPASSWORD_H

#include "base_login.h"

class CKVSOFT_EXPORT ResetPassword : public base_login
{
        Q_OBJECT

    public:
        explicit ResetPassword(int userId, QWidget *parent = nullptr);
        ~ResetPassword();
        void getPassword(QString &pw1, QString &pw2);

    private slots:
        void OnChange();

    private:

        int m_userid;

        SecureByteArray m_password1;
        SecureByteArray m_password2;
};

#endif // RESETPASSWORD_H
