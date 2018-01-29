#ifndef INDEPENDENTTERFACE_H
#define INDEPENDENTTERFACE_H

#include "plugininterface.h"

class QRK_EXPORT IndependentInterface : public PluginInterface
{

        Q_OBJECT
        Q_INTERFACES(PluginInterface)

    public:
        virtual ~IndependentInterface() {}
        virtual bool process() = 0;
        virtual QString getPluginName() = 0;

};

#define IndependentInterface_iid "at.ckvsoft.IndependentInterface"

Q_DECLARE_INTERFACE(IndependentInterface, IndependentInterface_iid)

#endif // INDEPENDENTINTERFACE_H
