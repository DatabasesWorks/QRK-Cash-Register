#include "qrkgastrocurfewchecker.h"
#include "database.h"

#include <QDateTime>

QrkGastroCurfewChecker::QrkGastroCurfewChecker(QObject *parent) :
    QObject(parent), m_timer(Q_NULLPTR)
{
}

QrkGastroCurfewChecker::~QrkGastroCurfewChecker()
{
    m_timer->stop();
    emit finished();
}

void QrkGastroCurfewChecker::run()
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &QrkGastroCurfewChecker::getCurfewDiff);
    m_timer->start(2000);
}

void QrkGastroCurfewChecker::getCurfewDiff()
{
    m_timer->stop();

    int diff = 0;
    QDateTime current = QDateTime::currentDateTime();
    QTime curfew = Database::getCurfewTime();
    if (current.time() > curfew) {
        diff = current.time().secsTo(QTime(23,59,59));
        diff = diff + QTime(0,0,0).secsTo(curfew);
    } else {
        diff = current.time().secsTo(curfew);
    }

    if (diff < 1800) {
        emit curFew(diff);
        m_timer->start(180000);
    }
    m_timer->start((diff - 1800) * 1000);
}
