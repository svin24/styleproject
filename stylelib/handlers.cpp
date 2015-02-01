﻿#include "handlers.h"
#include "xhandler.h"
#include "macros.h"
#include "color.h"
#include "render.h"
#include "ops.h"
#include "../config/settings.h"
#include "shadowhandler.h"

#include <QMainWindow>
#include <QTabBar>
#include <QToolBar>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QEvent>
#include <QLayout>
#include <QStyle>
#include <QTimer>
#include <QPainter>
#include <QMenuBar>
#include <QDebug>
#include <QMouseEvent>
#include <QApplication>
#include <QStatusBar>
#include <QLabel>
#include <QDialog>
#include <QToolButton>
#include <qmath.h>
#include <QAbstractScrollArea>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QBuffer>
#include <QHeaderView>
#include <QToolTip>
#include <QDesktopWidget>
#include <QGroupBox>

static QRegion paintRegion(QMainWindow *win)
{
    QRegion r(win->rect());
    QList<QToolBar *> children(win->findChildren<QToolBar *>());
    for (int i = 0; i < children.count(); ++i)
    {
        QToolBar *c(children.at(i));
        if (c->parentWidget() != win || win->toolBarArea(c) != Qt::TopToolBarArea
                || c->orientation() != Qt::Horizontal || !c->isVisible())
            continue;
        r -= QRegion(c->geometry());
    }
    if (QStatusBar *bar = win->findChild<QStatusBar *>())
        if (bar->isVisible())
        {
            if (bar->parentWidget() == win)
                r -= QRegion(bar->geometry());
            else
                r -= QRegion(QRect(bar->mapTo(win, bar->rect().topLeft()), bar->size()));
        }
    if (QTabBar *bar = win->findChild<QTabBar *>())
        if (bar->isVisible() && Ops::isSafariTabBar(bar))
        {
            QRect geo;
            if (bar->parentWidget() == win)
                geo = bar->geometry();
            else
                geo = QRect(bar->mapTo(win, bar->rect().topLeft()), bar->size());

            if (QTabWidget *tw = qobject_cast<QTabWidget *>(bar->parentWidget()))
            {
                int right(tw->mapTo(win, tw->rect().topRight()).x());
                int left(tw->mapTo(win, tw->rect().topLeft()).x());
                geo.setRight(right);
                geo.setLeft(left);
            }
            if (qApp->applicationName() == "konsole")
            {
                geo.setLeft(win->rect().left());
                geo.setRight(win->rect().right());
            }
            r -= QRegion(geo);
        }
    return r;
}


//-------------------------------------------------------------------------------------------------

Buttons::Buttons(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *bl(new QHBoxLayout(this));
    bl->setContentsMargins(4, 4, 4, 4);
    bl->setSpacing(4);
    bl->addWidget(new Button(Button::Close, this));
    bl->addWidget(new Button(Button::Min, this));
    bl->addWidget(new Button(Button::Max, this));
    setLayout(bl);
    if (parent)
        parent->window()->installEventFilter(this);
}

bool
Buttons::eventFilter(QObject *o, QEvent *e)
{
    if (!o || !e || !o->isWidgetType() || !static_cast<QWidget *>(o)->testAttribute(Qt::WA_WState_Created))
        return false;

    if (e->type() == QEvent::WindowDeactivate || e->type() == QEvent::WindowActivate || e->type() == QEvent::WindowTitleChange)
    {
        update();
        if (parentWidget())
            parentWidget()->update();
    }
    return false;
}

//-------------------------------------------------------------------------------------------------

void
TitleWidget::paintEvent(QPaintEvent *)
{
    const QString title(window()->windowTitle());
    QPainter p(this);
    QFont f(p.font());
    f.setBold(window()->isActiveWindow());
    f.setPointSize(f.pointSize()*1.2f);
    p.setFont(f);
    int align(Qt::AlignVCenter);
    switch (dConf.titlePos)
    {
    case Left:
        align |= Qt::AlignLeft;
        break;
    case Center:
        align |= Qt::AlignHCenter;
        break;
    case Right:
        align |= Qt::AlignRight;
        break;
    default: break;
    }

    style()->drawItemText(&p, rect(), align, palette(), true, p.fontMetrics().elidedText(title, Qt::ElideRight, rect().width()), foregroundRole());
    p.end();
}

//-------------------------------------------------------------------------------------------------

using namespace Handlers;

Q_DECL_EXPORT ToolBar ToolBar::s_instance;

ToolBar
*ToolBar::instance()
{
    return &s_instance;
}

void
ToolBar::manage(QToolButton *tb)
{
    if (!qobject_cast<QToolBar *>(tb->parentWidget()))
        return;
    tb->removeEventFilter(instance());
    tb->installEventFilter(instance());
}

void
ToolBar::checkForArrowPress(QToolButton *tb, const QPoint pos)
{
    QStyleOptionToolButton option;
    option.initFrom(tb);
    option.features = QStyleOptionToolButton::None;
    if (tb->popupMode() == QToolButton::MenuButtonPopup)
    {
        option.subControls |= QStyle::SC_ToolButtonMenu;
        option.features |= QStyleOptionToolButton::MenuButtonPopup;
    }
    if (tb->arrowType() != Qt::NoArrow)
        option.features |= QStyleOptionToolButton::Arrow;
    if (tb->popupMode() == QToolButton::DelayedPopup)
        option.features |= QStyleOptionToolButton::PopupDelay;
    if (tb->menu())
        option.features |= QStyleOptionToolButton::HasMenu;
    QRect r = tb->style()->subControlRect(QStyle::CC_ToolButton, &option, QStyle::SC_ToolButtonMenu, tb);
    if (r.contains(pos))
        tb->setProperty(MENUPRESS, true);
    else
        tb->setProperty(MENUPRESS, false);
}

bool
ToolBar::isArrowPressed(const QToolButton *tb)
{
    return tb&&tb->property(MENUPRESS).toBool();
}

void
ToolBar::updateToolBarLater(QToolBar *bar, const int time)
{
    QTimer *t(bar->findChild<QTimer *>(TOOLTIMER));
    if (!t)
    {
        t = new QTimer(bar);
        t->setObjectName(TOOLTIMER);
        connect(t, SIGNAL(timeout()), instance(), SLOT(updateToolBarSlot()));
    }
    t->start(time);
}

void
ToolBar::forceButtonSizeReRead(QToolBar *bar)
{
    const QSize &iconSize(bar->iconSize());
    bar->setIconSize(iconSize - QSize(1, 1));
    bar->setIconSize(iconSize);
#if 0
    for (int i = 0; i < bar->actions().count(); ++i)
    {
        QAction *a(bar->actions().at(i));
        if (QWidget *w = bar->widgetForAction(a))
        {
            bool prevIsToolBtn(false);
            QAction *prev(0);
            if (i)
                prev = bar->actions().at(i-1);
            if (!prev)
                continue;
            prevIsToolBtn = qobject_cast<QToolButton *>(bar->widgetForAction(prev));
            if (!prevIsToolBtn && !prev->isSeparator() && !a->isSeparator())
                bar->insertSeparator(a);
        }
    }
#endif

    if (dConf.removeTitleBars
            && bar->geometry().topLeft() == QPoint(0, 0)
            && bar->orientation() == Qt::Horizontal
            && qobject_cast<QMainWindow *>(bar->parentWidget()))
        setupNoTitleBarWindow(bar);
}

void
ToolBar::adjustMargins(QToolBar *toolBar)
{
    QMainWindow *win = qobject_cast<QMainWindow *>(toolBar->parentWidget());
    if (!win || !toolBar || toolBar->actions().isEmpty())
      return;
    if (toolBar->isWindow() && toolBar->isFloating())
    {
        toolBar->setContentsMargins(0, 0, 0, 0);
        if (toolBar->layout())
            toolBar->layout()->setContentsMargins(4, 4, 4, 4);
    }
    else if (win->toolBarArea(toolBar) == Qt::TopToolBarArea && toolBar->layout())
    {
        if (toolBar->geometry().top() <= win->rect().top() && !toolBar->isWindow())
        {
            if (win->windowFlags() & Qt::FramelessWindowHint || !win->mask().isEmpty())
            {
                const int pm(6/*toolBar->style()->pixelMetric(QStyle::PM_ToolBarFrameWidth)*/);
                toolBar->layout()->setContentsMargins(pm, pm, pm, pm);
                toolBar->setContentsMargins(0, 0, 0, 0);
            }
            else
            {
                toolBar->layout()->setContentsMargins(0, 0, 0, 0);
                toolBar->QWidget::setContentsMargins(0, 0, dConf.uno.enabled*toolBar->style()->pixelMetric(QStyle::PM_ToolBarHandleExtent), 6);
                if (!win->property(CSDBUTTONS).toBool() && !toolBar->property(TOOLPADDING).toBool())
                {
                    QWidget *w(new QWidget(toolBar));
                    const int sz(dConf.uno.enabled?8:4);
                    w->setFixedSize(sz, sz);
                    toolBar->insertWidget(toolBar->actions().first(), w);
                    toolBar->setProperty(TOOLPADDING, true);
                }
            }
        }
        else if (toolBar->findChild<QTabBar *>()) //sick, put a tabbar in a toolbar... eiskaltdcpp does this :)
        {
            toolBar->layout()->setContentsMargins(0, 0, 0, 0);
            toolBar->setMovable(false);
        }
        else
            toolBar->layout()->setContentsMargins(2, 2, 2, 2);
    }
    Window::updateWindowDataLater(toolBar->window());
}

void
ToolBar::updateToolBarSlot()
{
    QTimer *t = qobject_cast<QTimer *>(sender());
    if (!t)
        return;
    QToolBar *bar = qobject_cast<QToolBar *>(t->parent());
    if (!bar)
        return;

    forceButtonSizeReRead(bar);
    adjustMargins(bar);

    t->stop();
    t->deleteLater();
}

void
ToolBar::manage(QToolBar *tb)
{
    if (!tb)
        return;
    tb->removeEventFilter(instance());
    tb->installEventFilter(instance());
    if (qobject_cast<QMainWindow *>(tb->parentWidget()))
        connect(tb, SIGNAL(topLevelChanged(bool)), ToolBar::instance(), SLOT(adjustMarginsSlot()));
}

void
ToolBar::adjustMarginsSlot()
{
    adjustMargins(static_cast<QToolBar *>(sender()));
}

bool
ToolBar::eventFilter(QObject *o, QEvent *e)
{
    if (!o || !e || !o->isWidgetType())
        return false;
    QToolBar *tb = qobject_cast<QToolBar *>(o);
    QToolButton *tbn = qobject_cast<QToolButton *>(o);
    switch (e->type())
    {
    case QEvent::Show:
    {
        if (tbn)
        {
            updateToolBarLater(static_cast<QToolBar *>(tbn->parentWidget()));
//            forceButtonSizeReRead(static_cast<QToolBar *>(tbn->parentWidget()));
//            if (dConf.removeTitleBars)
//                setupNoTitleBarWindow(static_cast<QToolBar *>(tbn->parentWidget()));
        }
        else if (tb && dConf.uno.enabled)
            Window::updateWindowDataLater(tb->window());
        return false;
    }
    case QEvent::Hide:
    {
        if (tb && dConf.uno.enabled)
            Window::updateWindowDataLater(tb->window());
        return false;
    }
    case QEvent::MouseButtonPress:
    {
        if (tbn && Ops::hasMenu(tbn))
            checkForArrowPress(tbn, static_cast<QMouseEvent *>(e)->pos());
        return false;
    }
    case QEvent::ActionChanged:
    {
        if (tb)
            tb->update();
        return false;
    }
    default: return false;
    }
    return false;
}

static TitleWidget *canAddTitle(QToolBar *toolBar, bool &canhas)
{
    canhas = false;
    const QList<QAction *> actions(toolBar->actions());
    if (toolBar->toolButtonStyle() == Qt::ToolButtonIconOnly)
    {
        canhas = true;
        for (int i = 0; i < actions.count(); ++i)
        {
            QAction *a(actions.at(i));
            if (a->isSeparator())
                continue;
            QWidget *w(toolBar->widgetForAction(actions.at(i)));
            if (!w || qobject_cast<Buttons *>(w) || qobject_cast<TitleWidget *>(w))
                continue;
            if (!qobject_cast<QToolButton *>(w))
            {
                canhas = false;
                break;
            }
        }
    }
    TitleWidget *tw(toolBar->findChild<TitleWidget *>());
    if (!canhas && tw)
    {
        toolBar->removeAction(toolBar->actionAt(tw->geometry().topLeft()));
        tw->deleteLater();
        toolBar->setProperty(HASSTRETCH, false);
    }
    return tw;
}

static void applyMask(QWidget *widget)
{
    if (XHandler::opacity() < 1.0f)
    {
        widget->clearMask();
        if (!widget->windowFlags().testFlag(Qt::FramelessWindowHint) && widget->property(CSDBUTTONS).toBool())
        {
            widget->setWindowFlags(widget->windowFlags()|Qt::FramelessWindowHint);
            widget->show();
            ShadowHandler::manage(widget);
        }
        unsigned int d(0);
        XHandler::setXProperty<unsigned int>(widget->winId(), XHandler::KwinBlur, XHandler::Long, &d);
    }
    else
    {
        const int w(widget->width()), h(widget->height());
        QRegion r(0, 2, w, h-4);
        r += QRegion(1, 1, w-2, h-2);
        r += QRegion(2, 0, w-4, h);
        widget->setMask(r);
    }
}

void
ToolBar::setupNoTitleBarWindow(QToolBar *toolBar)
{
    if (!toolBar
            || !toolBar->parentWidget()
            || toolBar->parentWidget()->parentWidget()
            || toolBar->actions().isEmpty()
            || !qobject_cast<QMainWindow *>(toolBar->parentWidget())
            || toolBar->geometry().topLeft() != QPoint(0, 0)
            || toolBar->orientation() != Qt::Horizontal)
        return;

    Buttons *b(toolBar->findChild<Buttons *>());
    if (!b)
    {
        toolBar->insertWidget(toolBar->actions().first(), new Buttons(toolBar));
        toolBar->window()->installEventFilter(instance());
        toolBar->window()->setProperty(CSDBUTTONS, true);
    }
    else
    {
        QAction *a(toolBar->actionAt(b->geometry().topLeft()));
        if (a)
        {
            toolBar->removeAction(a);
            toolBar->insertAction(toolBar->actions().first(), a);
        }
    }

    applyMask(toolBar->window());

    bool cantitle;
    TitleWidget *t = canAddTitle(toolBar, cantitle);
    if (cantitle)
    {
        QAction *ta(0);
        if (t)
        {
            ta = toolBar->actionAt(t->geometry().topLeft());
            toolBar->removeAction(ta);
        }
        TitleWidget *title(t?t:new TitleWidget(toolBar));
        title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QAction *a(0);
        if (dConf.titlePos == TitleWidget::Center)
        {
            int at(qFloor((float)toolBar->actions().count()/2.0f));
            int i(at);
            const QList<QAction *> actions(toolBar->actions());
            a = actions.at(i);
            while (a && i > (at-3))
            {
                if (!a)
                {
                    toolBar->removeAction(toolBar->actionAt(title->geometry().topLeft()));
                    title->deleteLater();
                    return;
                }
                if (a->isSeparator())
                    break;
                else
                    a = actions.at(--i);
            }
        }
        else if (dConf.titlePos == TitleWidget::Left)
            a = toolBar->actions().at(1);
        if (ta)
            toolBar->insertAction(a, ta);
        else
            toolBar->insertWidget(a, title);
        toolBar->setProperty(HASSTRETCH, true);
    }
//    if (TitleWidget *tw = toolBar->findChild<TitleWidget *>())
//    {

//    }
    adjustMargins(toolBar);
//    fixTitleLater(toolBar->parentWidget());
}

//-------------------------------------------------------------------------------------------------

Window Window::s_instance;
QMap<uint, QVector<QPixmap> > Window::s_unoPix;
QMap<QWidget *, Handlers::Data> Window::s_unoData;
QMap<QWidget *, QPixmap> Window::s_bgPix;

Window::Window(QObject *parent)
    : QObject(parent)
{
}

void
Window::manage(QWidget *w)
{
    if (!w)
        return;
    w->removeEventFilter(instance());
    w->installEventFilter(instance());
}

void
Window::release(QWidget *w)
{
    if (!w)
        return;
    w->removeEventFilter(instance());
}

Window
*Window::instance()
{
    return &s_instance;
}

void
Window::addCompactMenu(QWidget *w)
{
    if (instance()->m_menuWins.contains(w)
            || !qobject_cast<QMainWindow *>(w))
        return;
    instance()->m_menuWins << w;
    w->removeEventFilter(instance());
    w->installEventFilter(instance());
}

void
Window::menuShow()
{
    QMenu *m(static_cast<QMenu *>(sender()));
    m->clear();
    QMainWindow *mw(static_cast<QMainWindow *>(m->parentWidget()->window()));
    if (QMenuBar *menuBar = mw->findChild<QMenuBar *>())
        m->addActions(menuBar->actions());
}

bool
Window::eventFilter(QObject *o, QEvent *e)
{
    if (!o || !e || !o->isWidgetType())
        return false;
    QWidget *w = static_cast<QWidget *>(o);
    switch (e->type())
    {
    case QEvent::MouseButtonPress:
    {
        if (w->isWindow() && !qobject_cast<QDialog *>(w))
            return false;
        QMouseEvent *me(static_cast<QMouseEvent *>(e));
        if (QTabBar *tb = qobject_cast<QTabBar *>(w))
            if (tb->tabAt(me->pos()) != -1)
                return false;
        if (w->cursor().shape() != Qt::ArrowCursor
                || QApplication::overrideCursor()
                || w->mouseGrabber()
                || me->button() != Qt::LeftButton)
            return false;
        return false;
    }
    case QEvent::Paint:
    {
        if (dConf.hackDialogs
                && qobject_cast<QDialog *>(w)
                && w->testAttribute(Qt::WA_TranslucentBackground)
                && w->windowFlags() & Qt::FramelessWindowHint)
        {
            QPainter p(w);
            p.setPen(Qt::NoPen);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(w->palette().color(w->backgroundRole()));
            p.setOpacity(XHandler::opacity());
            p.drawRoundedRect(w->rect(), 4, 4);
            p.end();
            return false;
        }
        else if (w->isWindow())
        {
            unsigned char *addV(XHandler::getXProperty<unsigned char>(w->winId(), XHandler::DecoData));
            if (addV || w->property(CSDBUTTONS).toBool())
            {
                const QColor bgColor(w->palette().color(w->backgroundRole()));
                QPainter p(w);
                p.setOpacity(XHandler::opacity());
                Render::Sides sides(Render::All);
                if (!w->property(CSDBUTTONS).toBool())
                    sides &= ~Render::Top;
                QBrush b;
                if (dConf.uno.enabled)
                {
                    b = bgColor;
                    if (XHandler::opacity() < 1.0f
                            && qobject_cast<QMainWindow *>(w))
                    {
                        p.setOpacity(1.0f);
                        p.setClipRegion(paintRegion(static_cast<QMainWindow *>(w)));
                    }
                }
                else
                {
                    if (addV)
                    {
                        const int add(addV?*addV:0);
                        QPixmap pix(s_bgPix.value(w));
                        b = pix.copy(0, add, pix.width(), pix.height()-add);
                    }
                    else
                        b = s_bgPix.value(w);
                }
                p.fillRect(w->rect(), b);
                if (XHandler::opacity() < 1.0f)
                    Render::shapeCorners(w, &p, sides);
                p.setPen(Qt::black);
                p.setOpacity(dConf.shadows.opacity);
                if (dConf.uno.enabled)
                    if (int th = Handlers::unoHeight(w, ToolBars))
                        p.drawLine(0, th, w->width(), th);
                p.end();
                return false;
            }
            else
            {
//                QStyleOption opt;
//                opt.initFrom(w);
//                QPainter p(w);
//                w->style()->drawPrimitive(QStyle::PE_FrameWindow, &opt, &p, w);
//                p.end();
                return false;
            }
        }
        return false;
    }
    case QEvent::Show:
    {
        if (dConf.hackDialogs
                && qobject_cast<QDialog *>(w)
                && w->isModal())
        {
            QWidget *p = w->parentWidget();

            if  (!p && qApp)
                p = qApp->activeWindow();

            if (!p
                    || !qobject_cast<QMainWindow *>(p)
                    || p == w
                    || p->objectName() == "BE::Desk")
                return false;

            w->setWindowFlags(w->windowFlags() | Qt::FramelessWindowHint);
            w->setVisible(true);

            if (XHandler::opacity() < 1.0f)
            {
                w->setAttribute(Qt::WA_TranslucentBackground);
                unsigned int d(0);
                XHandler::setXProperty<unsigned int>(w->winId(), XHandler::KwinBlur, XHandler::Long, &d);
            }

            int y(p->mapToGlobal(p->rect().topLeft()).y());
            if (p->windowFlags() & Qt::FramelessWindowHint)
            if (QToolBar *bar = p->findChild<QToolBar *>())
            if (bar->orientation() == Qt::Horizontal
                    && bar->geometry().topLeft() == QPoint(0, 0))
                y+=bar->height();
            const int x(p->mapToGlobal(p->rect().center()).x()-(w->width()/2));

            w->move(x, y);
        }
        else
        {
            if (dConf.compactMenu
                    && w->testAttribute(Qt::WA_WState_Created)
                    && m_menuWins.contains(w))
            {
                QMainWindow *mw(static_cast<QMainWindow *>(w));
                QMenuBar *menuBar = mw->findChild<QMenuBar *>();
                if (!menuBar)
                    return false;

                QToolBar *tb(mw->findChild<QToolBar *>());
                if (!tb || tb->findChild<QToolButton *>("DSP_menubutton"))
                    return false;

                QToolButton *tbtn(new QToolButton(tb));
                tbtn->setText("Menu");
                QMenu *m = new QMenu(tbtn);
                m->addActions(menuBar->actions());
                connect(m, SIGNAL(aboutToShow()), this, SLOT(menuShow()));
                tbtn->setObjectName("DSP_menubutton");
                tbtn->setMenu(m);
                tbtn->setPopupMode(QToolButton::InstantPopup);
                QAction *before(tb->actions().first());
                tb->insertWidget(before, tbtn);
                tb->insertSeparator(before);
                menuBar->hide();
            }
        }
        return false;
    }
    case QEvent::Resize:
    {
        if (w->isWindow())
        {
            if (!dConf.uno.enabled)
                updateWindowDataLater(w);
            if (w->property(CSDBUTTONS).toBool())
                applyMask(w);
        }
        return false;
    }
    case QEvent::WindowStateChange:
    {
        if (dConf.uno.enabled
                || !qobject_cast<QMainWindow *>(w))
            return false;

        if (w->isMaximized() || w->isFullScreen())
            w->setContentsMargins(0, 0, 0, 0);
        else
            w->setContentsMargins(4, 0, 4, 4);
        return false;
    }
    case QEvent::Close:
    {
        if (unsigned long *bg = XHandler::getXProperty<unsigned long>(w->winId(), XHandler::DecoBgPix))
        {
            XHandler::deleteXProperty(w->winId(), XHandler::DecoBgPix);
            XHandler::freePix(*bg);
        }
        break;
    }
    case QEvent::PaletteChange:
    {
        if (w->isWindow())
            updateWindowDataLater(w);
        break;
    }
    default: break;
    }
    return false;
}

bool
Window::drawUnoPart(QPainter *p, QRect r, const QWidget *w, const QPoint &offset, float opacity)
{
    QWidget *win(w->window());
    const int clientUno(unoHeight(win, ToolBarAndTabBar));
    if (!clientUno)
        return false;
    if (const QToolBar *tb = qobject_cast<const QToolBar *>(w))
//        if (QMainWindow *mwin = qobject_cast<QMainWindow *>(tb->parentWidget()))
//            if (mwin->toolBarArea(const_cast<QToolBar *>(tb)) != Qt::TopToolBarArea)
//                if (tb->orientation() != Qt::Horizontal)
        if (tb->geometry().top() > clientUno)
            return false;

    if (!w->isWindow() && w->height() > w->width())
        return false;

    if (qobject_cast<QMainWindow *>(w->parentWidget()) && w->parentWidget()->parentWidget())
        return false;

    const int allUno(unoHeight(win, All));
    uint check = allUno;
    if (dConf.uno.hor)
        check = check<<16|win->width();
    if (s_unoPix.contains(check))
    {
        const float savedOpacity(p->opacity());
        p->setOpacity(opacity);
        p->drawTiledPixmap(r, s_unoPix.value(check).at(0), offset);
        if (dConf.uno.contAware && ScrollWatcher::hasBg((qlonglong)win) && w->mapTo(win, QPoint(0, 0)).y() < clientUno)
        {
            p->setOpacity(dConf.uno.opacity);
            p->drawImage(QPoint(0, 0), ScrollWatcher::bg((qlonglong)win), r.translated(offset+QPoint(0, unoHeight(win, TitleBar))));
        }
        p->setOpacity(savedOpacity);
        return true;
    }
    return false;
}

unsigned int
Window::getHeadHeight(QWidget *win, unsigned int &needSeparator)
{
    unsigned char *h = XHandler::getXProperty<unsigned char>(win->winId(), XHandler::DecoData);
    const bool csd(dConf.removeTitleBars && win->property(CSDBUTTONS).toBool());
    if (!h && !csd)
        return 0;
    if (!dConf.uno.enabled)
    {
        needSeparator = 0;
        if (csd)
            return 0;
        if (h)
            return *h;
        return 0;
    }
    const int oldHead(unoHeight(win, All));
    int hd[HeightCount];
    hd[TitleBar] = (h?*h:0);
    hd[All] = hd[TitleBar];
    hd[ToolBars] = hd[ToolBarAndTabBar] = 0;
    if (QMainWindow *mw = qobject_cast<QMainWindow *>(win))
    {
        QMenuBar *menuBar = mw->findChild<QMenuBar *>();
        if (menuBar && menuBar->isVisible())
            hd[All] += menuBar->height();

        QList<QToolBar *> tbs(mw->findChildren<QToolBar *>());
        for (int i = 0; i<tbs.count(); ++i)
        {
            QToolBar *tb(tbs.at(i));
            if (tb->isVisible())
                if (!tb->findChild<QTabBar *>()) //ah eiskalt...
                    if (mw->toolBarArea(tb) == Qt::TopToolBarArea)
                    {
                        if (tb->geometry().bottom() > hd[ToolBars])
                            hd[ToolBars] = tb->geometry().bottom()+1;
                        needSeparator = 0;
                    }
        }
    }
    hd[ToolBarAndTabBar] = hd[ToolBars];
    QList<QTabBar *> tabbars = win->findChildren<QTabBar *>();
    QTabBar *possible(0);
    for (int i = 0; i < tabbars.count(); ++i)
    {
        const QTabBar *tb(tabbars.at(i));
        if (tb && tb->isVisible() && Ops::isSafariTabBar(tb))
        {
            possible = const_cast<QTabBar *>(tb);
            needSeparator = 0;
            const int y(tb->mapTo(win, tb->rect().bottomLeft()).y());
            if (y > hd[ToolBarAndTabBar])
                hd[ToolBarAndTabBar] = y;
        }
    }
    hd[All] += hd[ToolBarAndTabBar];
//    if (oldHead != hd[All] && qobject_cast<QMainWindow *>(win))
//        ScrollWatcher::detachMem(static_cast<QMainWindow *>(win));
    s_unoData.insert(win, Data(hd, possible));
    return hd[All];
}

static QVector<QPixmap> unoParts(QWidget *win, int h)
{
    const bool hor(dConf.uno.hor);
    QLinearGradient lg(0, 0, hor?win->width():0, hor?0:h);
    QPalette pal(win->palette());
    if (!pal.color(win->backgroundRole()).alpha() < 0xff)
        pal = QApplication::palette();
    QColor bc(pal.color(win->backgroundRole()));
    bc = Color::mid(bc, dConf.uno.tint.first, 100-dConf.uno.tint.second, dConf.uno.tint.second);
    lg.setStops(Settings::gradientStops(dConf.uno.gradient, bc));

    const unsigned int n(dConf.uno.noise);
    const int w(hor?win->width():(n?Render::noise().width():1));
    QPixmap p(w, h);
    p.fill(Qt::transparent);
    QPainter pt(&p);
    pt.fillRect(p.rect(), lg);
    pt.end();
    if (n)
        p = Render::mid(p, QBrush(Render::noise()), 100-n, n);
    QVector<QPixmap> values;
    values << p.copy(0, unoHeight(win, TitleBar), w, unoHeight(win, ToolBarAndTabBar)) << p.copy(0, 0, w, unoHeight(win, TitleBar));
    return values;
}

static QPixmap bgPix(QWidget *win)
{
    const bool hor(dConf.windows.hor);
    QLinearGradient lg(0, 0, hor*win->width(), !hor*win->height());
    QColor bc(win->palette().color(win->backgroundRole()));
    lg.setStops(Settings::gradientStops(dConf.windows.gradient, bc));

    unsigned char *addV(XHandler::getXProperty<unsigned char>(win->winId(), XHandler::DecoData));
    if (!addV)
        return QPixmap();
    const unsigned int n(dConf.windows.noise);
    const int w(hor?win->width():(n?Render::noise().width():1));
    const int h(!hor?win->height()+*addV:(n?Render::noise().height():1));
    QPixmap p(w, h);
    p.fill(Qt::transparent);
    QPainter pt(&p);
    pt.fillRect(p.rect(), lg);

    if (n)
//        p = Render::mid(p, QBrush(Render::noise()), 100-n, n);
    {
        pt.setOpacity(n*0.01f);
        pt.drawTiledPixmap(p.rect(), Render::noise());
    }
    pt.end();
    Window::s_bgPix.insert(win, p);
    return Window::s_bgPix.value(win);
}

template<typename T> static void updateChildren(QWidget *win)
{
    const QList<T> kids = win->findChildren<T>();
    const int size(kids.size());
    for (int i = 0; i < size; ++i)
        kids.at(i)->update();
}

void
Window::updateWindowData(QWidget *win)
{
    if (!win || !win->isWindow() || !win->testAttribute(Qt::WA_WState_Created))
        return;

    const int oldHeight(unoHeight(win, All));
    unsigned int ns(1);
    WindowData wd;
    const unsigned int height(getHeadHeight(win, ns));
    wd.data = 0;
    wd.data |= dConf.uno.enabled*WindowData::Uno;
    wd.data |= bool(dConf.uno.enabled&&dConf.uno.contAware)*WindowData::ContAware;
    wd.data |= (height<<16)&WindowData::UnoHeight;
    wd.data |= WindowData::Separator*ns;
    wd.data |= (int(win->testAttribute(Qt::WA_TranslucentBackground)?(unsigned int)(XHandler::opacity()*100.0f):100)<<8&WindowData::Opacity);
    wd.data |= (dConf.deco.buttons+1)<<24;
    wd.data |= (dConf.deco.frameSize)<<28;

    QPalette pal(win->palette());
    if (!pal.color(win->backgroundRole()).alpha() < 0xff) //im looking at you spotify you faggot!!!!!!!!!!!111111
        pal = QApplication::palette();

    wd.text = pal.color(win->foregroundRole()).rgba();
    wd.bg = pal.color(win->backgroundRole()).rgba();

    if (!height && dConf.uno.enabled)
        return;

    const WId id(win->winId());

    unsigned long handle(0);
    unsigned long *x11pixmap = XHandler::getXProperty<unsigned long>(id, XHandler::DecoBgPix);

    if (x11pixmap)
        handle = *x11pixmap;

    if (dConf.uno.enabled)
    {
        uint check = height;
        if (dConf.uno.hor)
            check = check<<16|win->width();
        if (!s_unoPix.contains(check))
            s_unoPix.insert(check, unoParts(win, height));
        if (oldHeight != height)
            XHandler::x11Pix(s_unoPix.value(check).at(1), handle, win);
    }
    else
        XHandler::x11Pix(bgPix(win), handle, win);

    if ((handle && !x11pixmap) || (x11pixmap && *x11pixmap != handle))
        XHandler::setXProperty<unsigned long>(id, XHandler::DecoBgPix, XHandler::Long, reinterpret_cast<unsigned long *>(&handle));

    win->update();
    if (dConf.uno.enabled)
    {
        updateChildren<QToolBar *>(win);
        updateChildren<QMenuBar *>(win);
        updateChildren<QTabBar *>(win);
        updateChildren<QStatusBar *>(win);
    }

    XHandler::setXProperty<unsigned int>(id, XHandler::WindowData, XHandler::Short, reinterpret_cast<unsigned int *>(&wd), 3);
    updateDeco(win->winId(), 1);
    emit instance()->windowDataChanged(win);
}

void
Window::updateWindowDataLater(QWidget *win)
{
    if (!win)
        return;

    QTimer *t = win->findChild<QTimer *>(TIMERNAME);
    if (!t)
    {
        t = new QTimer(win);
        t->setObjectName(TIMERNAME);
        connect(t, SIGNAL(timeout()), instance(), SLOT(updateWindowDataSlot()));
    }
    t->start(250);
}

void
Window::updateWindowDataSlot()
{
    if (QTimer *t = qobject_cast<QTimer *>(sender()))
    {
        if (t->parent()->isWidgetType())
        if (QWidget *w = static_cast<QWidget *>(t->parent()))
        if (w->isWindow())
            updateWindowData(w);
        t->stop();
    }
}

static QDBusMessage methodCall(const QString &method)
{
    return QDBusMessage::createMethodCall("org.kde.kwin", "/StyleProjectFactory", "org.kde.DBus.StyleProjectFactory", method);
}

void
Window::updateDeco(WId window, unsigned int changed)
{
    QDBusMessage msg = methodCall("update");
    msg << QVariant::fromValue((unsigned int)window) << QVariant::fromValue(changed);
    QDBusConnection::sessionBus().send(msg);
}

//-------------------------------------------------------------------------------------------------

Drag Drag::s_instance;

Drag
*Drag::instance()
{
    return &s_instance;
}

void
Drag::manage(QWidget *w)
{
    w->removeEventFilter(instance());
    w->installEventFilter(instance());
}

bool
Drag::eventFilter(QObject *o, QEvent *e)
{
    if (!o || !e || !o->isWidgetType() || e->type() != QEvent::MouseButtonPress || QApplication::overrideCursor() || QWidget::mouseGrabber())
        return false;
    QWidget *w(static_cast<QWidget *>(o));
    if (w->cursor().shape() != Qt::ArrowCursor)
        return false;
    QMouseEvent *me(static_cast<QMouseEvent *>(e));
    if (!w->rect().contains(me->pos()))
        return false;
    bool cd(canDrag(w));
    if (QTabBar *tabBar = qobject_cast<QTabBar *>(w))
        cd = tabBar->tabAt(me->pos()) == -1;
    if (cd)
        XHandler::mwRes(me->globalPos(), w->window()->winId());
    return false;
}

bool
Drag::canDrag(QWidget *w)
{
    return (qobject_cast<QToolBar *>(w)
            || qobject_cast<QLabel *>(w)
            || qobject_cast<QTabBar *>(w)
            || qobject_cast<QStatusBar *>(w)
            || qobject_cast<QDialog *>(w));
}

//-------------------------------------------------------------------------------------------------

Q_DECL_EXPORT ScrollWatcher ScrollWatcher::s_instance;
QMap<qlonglong, QImage> ScrollWatcher::s_winBg;
QMap<QMainWindow *, QSharedMemory *> ScrollWatcher::s_mem;
static QList<QWidget *> s_watched;

ScrollWatcher::ScrollWatcher(QObject *parent) : QObject(parent)
{
    connect(this, SIGNAL(updateRequest()), this, SLOT(updateLater()), Qt::QueuedConnection);
}

void
ScrollWatcher::watch(QAbstractScrollArea *area)
{
    if (!area || s_watched.contains(area->viewport()))
        return;
    if (!dConf.uno.contAware || !qobject_cast<QMainWindow *>(area->window()))
        return;
    s_watched << area->viewport();
//    area->viewport()->removeEventFilter(instance());
    area->viewport()->installEventFilter(instance());
    area->window()->installEventFilter(instance());
}

void
ScrollWatcher::detachMem(QMainWindow *win)
{
    if (s_mem.contains(win))
    {
        QSharedMemory *m(s_mem.value(win));
        if (m->isAttached())
            m->detach();
    }
    if (s_winBg.contains((qulonglong)win))
        s_winBg.remove((qulonglong)win);
}

static QMainWindow *s_win(0);

void
ScrollWatcher::updateLater()
{
    if (s_win)
        updateWin(s_win);
}

void
ScrollWatcher::updateWin(QMainWindow *win)
{
    blockSignals(true);
    regenBg(win);
    const QList<QToolBar *> toolBars(win->findChildren<QToolBar *>());
    const int uno(unoHeight(win, Handlers::ToolBarAndTabBar));
    for (int i = 0; i < toolBars.count(); ++i)
    {
        QToolBar *bar(toolBars.at(i));
        if (!bar->isVisible())
            continue;

        if (bar->geometry().bottom() < uno)
            bar->update();
    }
    if (QTabBar *tb = win->findChild<QTabBar *>())
        if (tb->isVisible())
            if (tb->mapTo(win, tb->rect().bottomLeft()).y() <=  uno)
            {
                if (QTabWidget *tw = qobject_cast<QTabWidget *>(tb->parentWidget()))
                {
                    const QList<QWidget *> kids(tw->findChildren<QWidget *>());
                    for (int i = 0; i < kids.count(); ++i)
                    {
                        QWidget *kid(kids.at(i));
                        if (kid->isVisible() && !kid->geometry().top())
                            kid->update();
                    }
                }
                else
                    tb->update();
            }
    if (!dConf.removeTitleBars)
        Handlers::Window::updateDeco(win->winId(), 64);
    blockSignals(false);
}

void
ScrollWatcher::regenBg(QMainWindow *win)
{
    const int uno(unoHeight(win, Handlers::All)), titleHeight(unoHeight(win, Handlers::TitleBar));;
    if (uno == titleHeight)
        return;
    QSharedMemory *m(0);
    if (s_mem.contains(win))
        m = s_mem.value(win);
    else
    {
        m = new QSharedMemory(QString::number(win->winId()), win);
        s_mem.insert(win, m);
    }

    if (!m->isAttached())
    {
        const int dataSize(2048*128*4);
        if (!m->create(dataSize))
            return;
    }

    if (m->lock())
    {
        uchar *data = reinterpret_cast<uchar *>(m->data());
        QImage img(data, win->width(), uno, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);

        const QList<QAbstractScrollArea *> areas(win->findChildren<QAbstractScrollArea *>());
        for (int i = 0; i < areas.count(); ++i)
        {
            QAbstractScrollArea *area(areas.at(i));
            if (!area->isVisible()
                    || area->verticalScrollBar()->minimum() == area->verticalScrollBar()->maximum()
                    || area->verticalScrollBar()->value() == area->verticalScrollBar()->minimum())
                continue;
            const QPoint topLeft(area->mapTo(win, QPoint(0, 0)));
            if (topLeft.y()-1 > (uno-unoHeight(win, Handlers::TitleBar)) || area->verticalScrollBar()->value() == area->verticalScrollBar()->minimum())
                continue;
            QWidget *vp(area->viewport());
            area->blockSignals(true);
            area->verticalScrollBar()->blockSignals(true);
            vp->blockSignals(true);
            const int offset(vp->mapToParent(QPoint(0, 0)).y());
            QAbstractItemView *view = qobject_cast<QAbstractItemView *>(area);
            QAbstractItemView::ScrollMode mode;
            if (view)
            {
                mode = view->verticalScrollMode();
                view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            }

            const int prevVal(area->verticalScrollBar()->value());
            const int realVal(qMax(0, prevVal-(uno+offset)));

            area->verticalScrollBar()->setValue(realVal);
            const bool needRm(s_watched.contains(vp));
            if (needRm)
                vp->removeEventFilter(this);
            vp->render(&img, vp->mapTo(win, QPoint(0, titleHeight-qMin(uno+offset, prevVal))), QRegion(0, 0, area->width(), uno), QWidget::DrawWindowBackground);

            area->verticalScrollBar()->setValue(prevVal);
            if (needRm)
                vp->installEventFilter(this);

            if (view)
            {
                mode = view->verticalScrollMode();
                view->setVerticalScrollMode(mode);
            }

            vp->blockSignals(false);
            area->verticalScrollBar()->blockSignals(false);
            area->blockSignals(false);
        }
        if (int blurRadius = dConf.uno.blur)
            Render::expblur(img, blurRadius);
        s_winBg.insert((qlonglong)win, img);
        m->unlock();
    }
}

QImage
ScrollWatcher::bg(qlonglong win)
{
    return s_winBg.value(win, QImage());
}

bool
ScrollWatcher::eventFilter(QObject *o, QEvent *e)
{
    static bool s_block(false);
    if (e->type() == QEvent::Close && qobject_cast<QMainWindow *>(o))
            detachMem(static_cast<QMainWindow *>(o));
    else if (e->type() == QEvent::Paint && !s_block && qobject_cast<QAbstractScrollArea *>(o->parent()))
    {
        s_win = 0;
        QWidget *w(static_cast<QWidget *>(o));
        QAbstractScrollArea *a(static_cast<QAbstractScrollArea *>(w->parentWidget()));
        if (!s_watched.contains(w)
                || a->verticalScrollBar()->minimum() == a->verticalScrollBar()->maximum())
            return false;
        QMainWindow *win(static_cast<QMainWindow *>(w->window()));
        s_block = true;
        o->removeEventFilter(this);
        QCoreApplication::sendEvent(o, e);
        s_win = win;
        if (w->parentWidget()->mapTo(win, QPoint(0, 0)).y()-1 <= unoHeight(win, Handlers::ToolBarAndTabBar))
            emit updateRequest();
        o->installEventFilter(this);
        s_block = false;
        return true;
    }
    else if (e->type() == QEvent::Hide && qobject_cast<QAbstractScrollArea *>(o->parent()))
        updateWin(static_cast<QMainWindow *>(static_cast<QWidget *>(o)->window()));
    return false;
}

static QPainterPath balloonPath(const QRect &rect)
{
    QPainterPath path;
    static const int m(8);
    path.addRoundedRect(rect.adjusted(8, 8, -8, -8), 4, 4);
    path.closeSubpath();
    const int halfH(rect.x()+(rect.width()/2.0f));
    const int halfV(rect.y()+(rect.height()/2.0f));
    QPolygon poly;
    switch (Handlers::BalloonHelper::mode())
    {
    case Handlers::BalloonHelper::Left:
    {
        const int pt[] = { rect.x(),halfV, rect.x()+m,halfV-m, rect.x()+m,halfV+m };
        poly.setPoints(3, pt);
        break;
    }
    case Handlers::BalloonHelper::Top:
    {
        const int pt[] = { halfH,rect.y(), halfH-m,rect.y()+m, halfH+m,rect.y()+m };
        poly.setPoints(3, pt);
        break;
    }
    case Handlers::BalloonHelper::Right:
    {
        break;
    }
    case Handlers::BalloonHelper::Bottom:
    {
        break;
    }
    default:
        break;
    }
    path.addPolygon(poly);
    path.closeSubpath();
    return path.simplified();
}

static QImage balloonTipShadow(const QRect &rect, const int radius)
{
    QImage img(rect.size()+QSize(radius*2, radius*2), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawPath(balloonPath(img.rect().adjusted(radius, radius, -radius, -radius)));
    p.end();

    Render::expblur(img, radius);
    return img;
}

Balloon::Balloon() : QWidget(), m_label(new QLabel(this))
{
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setAutoFillBackground(true);
//    setWindowFlags(windowFlags()|Qt::FramelessWindowHint|Qt::X11BypassWindowManagerHint);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setSpacing(0);
    l->addWidget(m_label);
    setLayout(l);
    setParent(0, windowFlags()|Qt::ToolTip);
    QPalette pal(palette());
    pal.setColor(QPalette::ToolTipBase, pal.color(QPalette::WindowText));
    pal.setColor(QPalette::ToolTipText, pal.color(QPalette::Window));
    setForegroundRole(QPalette::ToolTipText);
    setBackgroundRole(QPalette::ToolTipBase);
    setPalette(pal);
    m_label->setPalette(pal);
    QFont f(m_label->font());
    f.setPointSize(10);
    f.setBold(true);
    m_label->setFont(f);
}

static bool s_isInit(false);
static QPixmap pix[8];

static void clearPix()
{
    for (int i = 0; i < 8; ++i)
        XHandler::freePix(pix[i].handle());
    s_isInit = false;
}

Balloon::~Balloon()
{
    if (s_isInit)
        clearPix();
}

void
Balloon::paintEvent(QPaintEvent *e)
{
    QPainter p(this);
    p.fillRect(rect(), palette().color(QPalette::ToolTipBase));
    p.end();
}

static const int s_padding(20);

void
Balloon::genPixmaps()
{
    if (s_isInit)
        clearPix();

    QPixmap px(size()+QSize(s_padding*2, s_padding*2));
    px.fill(Qt::transparent);
    static const int m(8);
    const QRect r(px.rect().adjusted(m, m, -m, -m));
    QPainter p(&px);
    p.drawImage(px.rect().topLeft(), balloonTipShadow(r.translated(0, 4), 8));
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(255, 255, 255, 255), 2.0f));
    p.setBrush(palette().color(QPalette::ToolTipBase));
    p.drawPath(balloonPath(r));
    p.end();

    pix[0] = XHandler::x11Pix(px.copy(s_padding, 0, px.width()-s_padding*2, s_padding)); //top
    pix[1] = XHandler::x11Pix(px.copy(px.width()-s_padding, 0, s_padding, s_padding)); //topright
    pix[2] = XHandler::x11Pix(px.copy(px.width()-s_padding, s_padding, s_padding, px.height()-s_padding*2)); //right
    pix[3] = XHandler::x11Pix(px.copy(px.width()-s_padding, px.height()-s_padding, s_padding, s_padding)); //bottomright
    pix[4] = XHandler::x11Pix(px.copy(s_padding, px.height()-s_padding, px.width()-s_padding*2, s_padding)); //bottom
    pix[5] = XHandler::x11Pix(px.copy(0, px.height()-s_padding, s_padding, s_padding)); //bottomleft
    pix[6] = XHandler::x11Pix(px.copy(0, s_padding, s_padding, px.height()-s_padding*2)); //left
    pix[7] = XHandler::x11Pix(px.copy(0, 0, s_padding, s_padding)); //topleft
    s_isInit = true;
}

void
Balloon::updateShadow()
{
    unsigned long data[12];
    genPixmaps();
    for (int i = 0; i < 8; ++i)
        data[i] = pix[i].handle();
    for (int i = 8; i < 12; ++i)
        data[i] = s_padding;

    XHandler::setXProperty<unsigned long>(winId(), XHandler::KwinShadows, XHandler::Long, data, 12);
}

void
Balloon::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    BalloonHelper::updateBallon();
    updateShadow();
}

void
Balloon::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    BalloonHelper::updateBallon();
    updateShadow();
}

static Balloon *s_toolTip(0);

void
Balloon::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
    s_toolTip = 0;
    deleteLater();
}

void
Balloon::setToolTipText(const QString &text)
{
    m_label->setText(text);
}

BalloonHelper BalloonHelper::s_instance;

Balloon
*BalloonHelper::balloon()
{
    if (!s_toolTip)
        s_toolTip = new Balloon();
    return s_toolTip;
}

void
BalloonHelper::manage(QWidget *w)
{
    w->setAttribute(Qt::WA_Hover);
    w->removeEventFilter(instance());
    w->installEventFilter(instance());
}

static QWidget *s_widget(0);
static BalloonHelper::Mode s_mode(BalloonHelper::Top);

BalloonHelper::Mode
BalloonHelper::mode()
{
    return s_mode;
}

void
BalloonHelper::updateBallon()
{
    if (!s_widget)
        return;
    if (balloon()->toolTipText() != s_widget->toolTip())
        balloon()->setToolTipText(s_widget->toolTip());
    QPoint globPt;
    const QRect globRect(s_widget->mapToGlobal(s_widget->rect().topLeft()), s_widget->size());
    globPt = QPoint(globRect.center().x()-(balloon()->width()/2), globRect.bottom()+s_padding);
    if (balloon()->pos() != globPt)
        balloon()->move(globPt);
}

bool
BalloonHelper::eventFilter(QObject *o, QEvent *e)
{
    if (!o->isWidgetType())
        return false;
    if (o->inherits("QTipLabel"))
    {
        if (e->type() == QEvent::Show && s_widget)
        {
            static_cast<QWidget *>(o)->hide();
            return true;
        }
        return false;
    }
    switch (e->type())
    {
    case QEvent::ToolTip:
    case QEvent::ToolTipChange:
    {
        if (e->type() == QEvent::ToolTipChange && !balloon()->isVisible())
            return false;
        if (!static_cast<QWidget *>(o)->toolTip().isEmpty()
                && (qobject_cast<QAbstractButton *>(o)
                    || qobject_cast<QSlider *>(o)
                    || qobject_cast<QLabel *>(o)))
        {
            s_widget = static_cast<QWidget *>(o);
            updateBallon();
            if (QToolTip::isVisible())
                QToolTip::hideText();
            if (!balloon()->isVisible())
                balloon()->show();
            return true;
        }
        return false;
    }
    case QEvent::HoverLeave:
    {
        if (balloon()->isVisible())
            balloon()->hide();
        s_widget = 0;
        return false;
    }
    default: return false;
    }
}
