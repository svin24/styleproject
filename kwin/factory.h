#ifndef FACTORY_H
#define FACTORY_H

#include <kdecorationfactory.h>
#include <QObject>

#include "kwinclient.h"

class Factory : public QObject, public KDecorationFactory
{
    Q_OBJECT
public:
    Factory();
    KDecoration *createDecoration(KDecorationBridge *bridge);
    bool supports(Ability ability) const;
    void update(WId window);
};


#endif //FACTORY_H