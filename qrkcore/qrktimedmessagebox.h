#ifndef QRKTIMEDMESSAGEBOX_H
#define QRKTIMEDMESSAGEBOX_H

#include "qrkcore_global.h"

#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

class QRK_EXPORT QrkTimedMessageBox : public QMessageBox
{
    Q_OBJECT

    public:
        QrkTimedMessageBox(int timeoutSeconds, Icon icon, const QString & title, const QString & text, StandardButtons buttons = NoButton, QWidget * parent = 0, Qt::WindowFlags flags = Qt::Dialog|Qt::MSWindowsFixedSizeDialogHint)
            : QMessageBox(icon, title, text, buttons, parent, flags)
            , m_timeoutSeconds(timeoutSeconds + 1)
        {
//            connect(&m_timer, SIGNAL(timeout()), this, SLOT(Tick()));
            connect(&m_timer, &QTimer::timeout, this, &QrkTimedMessageBox::Tick);
            m_timer.setInterval(1000);
        }
        QrkTimedMessageBox(int timeoutSeconds, const QString & title, const QString & text, Icon icon, int button0, int button1, int button2, QWidget * parent = 0, Qt::WindowFlags flags = Qt::Dialog|Qt::MSWindowsFixedSizeDialogHint)
            : QMessageBox(title, text, icon, button0, button1, button2, parent, flags)
            , m_timeoutSeconds(timeoutSeconds + 1)
        {
            connect(&m_timer, &QTimer::timeout, this, &QrkTimedMessageBox::Tick);
            m_timer.setInterval(1000);
        }

        virtual void showEvent(QShowEvent * e)
        {
            if (defaultButton())
                m_text = defaultButton()->text() + "\n" + QObject::tr("in %1 Sek.");
            QMessageBox::showEvent(e);
            Tick();
            m_timer.start();
        }

    private slots:
        void Tick()
        {
            if (defaultButton()) {
                if (--m_timeoutSeconds >= 0) {
                    defaultButton()->setText(m_text.arg(m_timeoutSeconds));
                } else {
                    m_timer.stop();
                    defaultButton()->animateClick();
                }
            } else {
                m_timer.stop();
                return;
            }
        }

    private:
        int m_timeoutSeconds;
        QString m_text;
        QTimer m_timer;
};

#endif // QRKTIMEDMESSAGEBOX_H
