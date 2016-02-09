#ifndef OPS_H
#define OPS_H

#include <QColor>
#include <QWidget>
#include <QQueue>
#include <QTimer>
#include "gfx.h"

/**
 * This class is a mess atm, I just add stuff here
 * that I need outta the way... most of this stuff
 * need to be properly categorised in some classes
 * that actually makes sense, for now, deal w/ it.
 */

class QToolBar;
class QTabBar;
class QToolButton;
class QStyle;
class QStyleOption;
class QStyleOptionToolButton;

namespace DSP
{

typedef void (QWidget::*Function)();
class QueueItem
{
public:
    QWidget *w;
    Function func;
};

class Q_DECL_EXPORT Ops : public QObject
{
    Q_OBJECT
public:
    static Ops *instance();
    static void deleteInstance();
    static QWidget *window(QWidget *w);
    static bool isSafariTabBar(const QTabBar *tabBar);
    static QPalette::ColorRole opposingRole(const QPalette::ColorRole &role);
    static QPalette::ColorRole bgRole(const QWidget *w, const QPalette::ColorRole fallBack = QPalette::Window);
    static QPalette::ColorRole fgRole(const QWidget *w, const QPalette::ColorRole fallBack = QPalette::WindowText);
    static bool hasMenu(const QToolButton *tb, const QStyleOptionToolButton *stb = 0);
    static void swap(int &t1, int &t2);
    template<typename T>static void swap(T &t1, T &t2)
    {
        const T tmp(t1);
        t1 = t2;
        t2 = tmp;
    }

    template<typename T> static inline bool isOrInsideA(const QWidget *widget)
    {
        if (!widget)
            return false;
        QWidget *w = const_cast<QWidget *>(widget);
        while (w->parentWidget())
        {
            if (qobject_cast<T>(w))
                return true;
            w = w->parentWidget();
        }
        return false;
    }
    template<typename T> static inline T getAncestor(const QWidget *widget)
    {
        if (!widget)
            return 0;
        QWidget *w = const_cast<QWidget *>(widget);
        while (w->parentWidget())
        {
            if (T t = qobject_cast<T>(w))
                return t;
            w = w->parentWidget();
        }
        return 0;
    }
    //convenience delayer... you can call other functions then slots later....
    static void callLater(QWidget *w, Function func, int time = 500);

public slots:
    void updateGeoFromSender();
    void later();

private:
    static Ops *s_instance;
    static QQueue<QueueItem> m_laterQueue;
};
} //namespace

#endif // OPS_H
