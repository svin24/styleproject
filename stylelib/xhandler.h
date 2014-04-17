#ifndef XHANDLER_H
#define XHANDLER_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "fixx11h.h"

#include <QWidget>
#include <QX11Info>
#include <QDebug>

class Q_DECL_EXPORT XHandler
{
public:
    enum Value { MainWindow, HeadColor, KwinShadows, StoreShadow, ValueCount };
    static Atom atom[ValueCount];
    template<typename T> static void setXProperty(const WId w, const Value v, T *d, unsigned int n = 1)
    {
        Atom a = (v == KwinShadows) ? XA_CARDINAL : XA_ATOM;
        XChangeProperty(QX11Info::display(), w, atom[v], a, 32, PropModeReplace, reinterpret_cast<unsigned char *>(d), n);
        XSync(QX11Info::display(), False);
    }
    static int _n;
    template<typename T> static T *getXProperty(const WId w, const Value v, int &n = _n)
    {
        Atom type;
        int format;
        unsigned long nitems, after;
        T *d = 0;
        Atom a = (v == KwinShadows) ? XA_CARDINAL : XA_ATOM;
        XGetWindowProperty(QX11Info::display(), w, atom[v], 0, (~0L), False, a, &type, &format, &nitems, &after, reinterpret_cast<unsigned char **>(&d));
        n = nitems;
        return d;
    }
    static void deleteXProperty(const WId w, const Value v)
    {
        XDeleteProperty(QX11Info::display(), w, atom[v]);
    }
};

#endif //XHANDLER_H