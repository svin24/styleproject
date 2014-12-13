#include <QTableWidget>
#include <QPainter>
#include <QStyleOptionTabWidgetFrameV2>
#include <QStyleOptionToolButton>
#include <QMainWindow>
#include <QToolBar>
#include <QLayout>
#include <QAction>
#include <QResizeEvent>
#include <QMenu>
#include <QMenuBar>
#include <QCheckBox>
#include <QLabel>
#include <QApplication>
#include <QToolButton>
#include <QTimer>
#include <QStatusBar>
#include <QPushButton>
#include "styleproject.h"
#include "stylelib/xhandler.h"
#include "stylelib/ops.h"
#include "stylelib/handlers.h"
#include "stylelib/settings.h"
#include "overlay.h"
#include "stylelib/animhandler.h"

bool
StyleProject::eventFilter(QObject *o, QEvent *e)
{
    if (!e || !o || !o->isWidgetType())
        return false;
    if (e->type() < EVSize && m_ev[e->type()])
        return (this->*m_ev[e->type()])(o, e);

    QWidget *w(static_cast<QWidget *>(o));
    switch (e->type())
    {
//    case QEvent::Show:
//    case QEvent::Leave:
//    case QEvent::HoverLeave:
//    case QEvent::Enter:
//    case QEvent::HoverEnter:
//    {
//        qDebug() << w << w->parentWidget();
//    }
//    case QEvent::Close:
//        if (w->testAttribute(Qt::WA_TranslucentBackground) && w->isWindow())
//            XHandler::deleteXProperty(w->winId(), XHandler::KwinBlur);
    case QEvent::Hide:
    {
        if (dConf.uno.enabled && (qobject_cast<QTabBar *>(w) || qobject_cast<QMenuBar*>(o)))
            Handlers::Window::fixWindowTitleBar(w->window());
    }
    case QEvent::LayoutRequest:
    {
        if (w->inherits("KTitleWidget"))
        {
            QList<QLabel *> lbls = w->findChildren<QLabel *>();
            for (int i = 0; i < lbls.count(); ++i)
                lbls.at(i)->setAlignment(Qt::AlignCenter);
        }
        else if (dConf.uno.enabled && qobject_cast<QMenuBar *>(w))
            Handlers::Window::fixWindowTitleBar(w->window());
    }
    case QEvent::MetaCall:
    {
        if (w->property("DSP_KTitleLabel").toBool())
            static_cast<QLabel *>(w)->setAlignment(Qt::AlignCenter);
    }
    default: break;
    }
    return QCommonStyle::eventFilter(o, e);
}

bool
StyleProject::paintEvent(QObject *o, QEvent *e)
{
    /* for some reason KTabWidget is an idiot and
     * doesnt use the style at all for painting, only
     * for the tabBar apparently (that inside the
     * KTabWidget) so we simply override the painting
     * for the KTabWidget and paint what we want
     * anyway.
     */
    QWidget *w(static_cast<QWidget *>(o));
    if (o->inherits("KTabWidget"))
    {
        QTabWidget *tabWidget = static_cast<QTabWidget *>(o);
        QPainter p(tabWidget);
        QStyleOptionTabWidgetFrameV2 opt;
        opt.initFrom(tabWidget);
        drawTabWidget(&opt, &p, tabWidget);
        p.end();
        return true;
    }
    else if (o->objectName() == "konsole_tabbar_parent")
    {
        QWidget *w = static_cast<QWidget *>(o);
        QStyleOptionTabBarBaseV2 opt;
        opt.rect = w->rect();
        QTabBar *tb = w->findChild<QTabBar *>();
        if (!Ops::isSafariTabBar(tb))
            return false;

        opt.rect.setHeight(tb->height());
        QPainter p(w);
        QRect geo(tb->mapTo(w, tb->rect().topLeft()), tb->size());
        p.setClipRegion(QRegion(w->rect())-QRegion(geo));
        drawTabBar(&opt, &p, tb);
        p.end();
        return true;
    }
    else if (w->inherits("KMultiTabBarInternal"))
    {
        QPainter p(w);
        QRect r(w->rect());
        const bool hor(w->width()>w->height());
        QLinearGradient lg(r.topLeft(), !hor?r.topRight():r.bottomLeft());
        lg.setStops(Settings::gradientStops(dConf.tabs.gradient, w->palette().color(QPalette::Button)));
        p.fillRect(r, lg);
        p.end();
        return false;
    }
    else if (w->inherits("KMultiTabBarTab"))
    {
        if (w->underMouse() || static_cast<QPushButton *>(w)->isChecked())
            return false;
        QPainter p(w);
        QStyleOptionToolButton opt;
        opt.initFrom(w);
        drawToolButtonBevel(&opt, &p, w);
        p.end();

        w->removeEventFilter(this);
        QCoreApplication::sendEvent(o, e);
        w->installEventFilter(this);
        return true;
    }
    return QCommonStyle::eventFilter(o, e);
}

bool
StyleProject::resizeEvent(QObject *o, QEvent *e)
{
    if (!o->isWidgetType())
        return false;
    QWidget *w(static_cast<QWidget *>(o));
    if (w->inherits("KTitleWidget"))
    {
        QList<QLabel *> lbls = w->findChildren<QLabel *>();
        for (int i = 0; i < lbls.count(); ++i)
            lbls.at(i)->setAlignment(Qt::AlignCenter);
    }
    return QCommonStyle::eventFilter(o, e);
}

bool
StyleProject::showEvent(QObject *o, QEvent *e)
{
    if (!o->isWidgetType())
        return QCommonStyle::eventFilter(o, e);
    QWidget *w(static_cast<QWidget *>(o));
    if ((qobject_cast<QMenuBar*>(o)||qobject_cast<QMenu *>(o)))
    {
        w->setMouseTracking(true);
        w->setAttribute(Qt::WA_Hover);
        if (dConf.uno.enabled && qobject_cast<QMenuBar*>(o))
            Handlers::Window::fixWindowTitleBar(w->window());
    }
    else if (qobject_cast<QTabBar *>(w))
    {
        Handlers::Window::fixWindowTitleBar(w->window());
    }
    else if (w->inherits("KTitleWidget"))
    {
        QList<QLabel *> lbls = w->findChildren<QLabel *>();
        for (int i = 0; i < lbls.count(); ++i)
            lbls.at(i)->setAlignment(Qt::AlignCenter);
    }
    return QCommonStyle::eventFilter(o, e);
}
