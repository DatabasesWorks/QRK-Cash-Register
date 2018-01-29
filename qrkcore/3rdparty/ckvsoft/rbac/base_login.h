#ifndef BASE_LOGIN_H
#define BASE_LOGIN_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"
#include "acl.h"
#include "crypto.h"

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

class CKVSOFT_EXPORT base_login : public QDialog
{
        Q_OBJECT

    public:
        explicit base_login(QWidget *parent = nullptr);
        ~base_login();

    protected:
        QLineEdit *userLineEdit;
        QLineEdit *passwordLineEdit;
        QPushButton *okPushButton;
        QPushButton *cancelPushButton;
        QCheckBox *savePasswordCheckBox;

        QLabel *windowIconLabel;
        QLabel *windowNameLabel;
        QLabel *windowCommentLabel;
        QLabel *programVersionLabel;
        QLabel *programVersionNoLabel;
        QLabel *loginLabel;
        QLabel *passwordLabel;
        QLabel *label;
        QLabel *logoLabel;
};

#endif // BASE_LOGIN_H
