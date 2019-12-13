#ifndef WIDGET_H
#define WIDGET_H

#include "3rdparty/ckvsoft/globals_ckvsoft.h"

#include <QWidget>

class CKVSOFT_EXPORT Widget : public QWidget
{
        Q_OBJECT
    public:
        explicit Widget(QWidget *parent = Q_NULLPTR) : QWidget(parent)
        {
        }

        void disableWidgets()
        {
            QList<QWidget *> widgets = this->findChildren<QWidget *>();

            foreach(QWidget * widget, widgets){
                widget->setEnabled(false);
            }
        }
};

#endif // WIDGET_H
