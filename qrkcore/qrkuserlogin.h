#ifndef QRKUSERLOGIN_H
#define QRKUSERLOGIN_H

#include "3rdparty/ckvsoft/rbac/userlogin.h"

class CKVSOFT_EXPORT QrkUserLogin : public UserLogin
{
    Q_OBJECT

public:
    QrkUserLogin(QWidget* parent = Q_NULLPTR);

private slots:
    void OnLogin();

};

#endif // QRKUSERLOGIN_H
