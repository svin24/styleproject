#include "unohandler.h"
#include "xhandler.h"
#include "macros.h"
#include "color.h"
#include "render.h"
#include "ops.h"
#include "settings.h"
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
#include <QScrollBar>

Q_DECL_EXPORT UNOHandler UNOHandler::s_instance;
Q_DECL_EXPORT QMap<int, QPixmap> UNOHandler::s_pix;

void
DButton::onClick(QMouseEvent *e, const Type &t)
{
    switch (t)
    {
    case Close: window()->close(); break;
    case Min: window()->showMinimized(); break;
    case Max: window()->isMaximized()?window()->showNormal():window()->showMaximized(); break;
    default: break;
    }
}

Buttons::Buttons(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *bl(new QHBoxLayout(this));
    bl->setContentsMargins(4, 4, 4, 4);
    bl->setSpacing(4);
    bl->addWidget(new DButton(Button::Close, this));
    bl->addWidget(new DButton(Button::Min, this));
    bl->addWidget(new DButton(Button::Max, this));
    setLayout(bl);
    if (parent)
        parent->window()->installEventFilter(this);
}

bool
Buttons::eventFilter(QObject *o, QEvent *e)
{
    if (!o || !e || !o->isWidgetType())
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
    switch (Settings::conf.titlePos)
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

UNOHandler::UNOHandler(QObject *parent)
    : QObject(parent)
{
}

UNOHandler
*UNOHandler::instance()
{
    return &s_instance;
}

void
UNOHandler::manage(QWidget *mw)
{
    mw->removeEventFilter(instance());
    mw->installEventFilter(instance());
    QList<QToolBar *> toolBars = mw->findChildren<QToolBar *>();
    for (int i = 0; i < toolBars.count(); ++i)
    {
        QToolBar *bar(toolBars.at(i));
        bar->removeEventFilter(instance());
        bar->installEventFilter(instance());
    }
    fixTitleLater(mw);
}

void
UNOHandler::release(QWidget *mw)
{
    mw->removeEventFilter(instance());
    QList<QToolBar *> toolBars = mw->findChildren<QToolBar *>();
    for (int i = 0; i < toolBars.count(); ++i)
        toolBars.at(i)->removeEventFilter(instance());
}

#define HASCHECK "DSP_hascheck"

static void applyMask(QWidget *widget)
{
    if (XHandler::opacity() < 1.0f)
    {
        widget->clearMask();
        if (!widget->windowFlags().testFlag(Qt::FramelessWindowHint) && Settings::conf.removeTitleBars)
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
    widget->setProperty(HASCHECK, true);
}

#define HASSTRETCH "DSP_hasstretch"
#define HASBUTTONS "DSP_hasbuttons"

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

void
UNOHandler::setupNoTitleBarWindow(QToolBar *toolBar)
{
    if (!toolBar
            || toolBar->actions().isEmpty()
            || !qobject_cast<QMainWindow *>(toolBar->parentWidget())
            || toolBar->geometry().topLeft() != QPoint(0, 0)
            || toolBar->orientation() != Qt::Horizontal)
        return;
    if (toolBar->parentWidget()->parentWidget())
        return;

    Buttons *b(toolBar->findChild<Buttons *>());
    if (!b)
    {
        toolBar->insertWidget(toolBar->actions().first(), new Buttons(toolBar));
        toolBar->window()->installEventFilter(instance());
        toolBar->window()->setProperty(HASBUTTONS, true);
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
        if (Settings::conf.titlePos == TitleWidget::Center)
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
        else if (Settings::conf.titlePos == TitleWidget::Left)
            a = toolBar->actions().at(1);
        if (ta)
            toolBar->insertAction(a, ta);
        else
            toolBar->insertWidget(a, title);
        toolBar->setProperty(HASSTRETCH, true);
    }
    updateToolBar(toolBar);
    fixTitleLater(toolBar->parentWidget());
}

bool
UNOHandler::eventFilter(QObject *o, QEvent *e)
{
    if (!o || !e)
        return false;
    if (!o->isWidgetType())
        return false;
    QWidget *w = static_cast<QWidget *>(o);
    switch (e->type())
    {
    case QEvent::Resize:
    {
        if (w->isWindow())
        {
            fixWindowTitleBar(w);
            if (qobject_cast<QMainWindow *>(w) && w->property(HASBUTTONS).toBool())
                applyMask(w);
        }
        if (castObj(QToolBar *, toolBar, o))
            if (castObj(QMainWindow *, win, toolBar->parentWidget()))
                updateToolBar(toolBar);
        return false;
    }
    case QEvent::Show:
    {
        fixWindowTitleBar(w);
        if (qobject_cast<QMainWindow *>(w) && XHandler::opacity() < 1.0f && !w->parentWidget())
        {
            unsigned int d(0);
            XHandler::setXProperty<unsigned int>(w->winId(), XHandler::KwinBlur, XHandler::Long, &d);
        }
        return false;
    }
    default: break;
    }
    return false;
}

bool
UNOHandler::drawUnoPart(QPainter *p, QRect r, const QWidget *w, int offset, float opacity)
{
    if (const QToolBar *tb = qobject_cast<const QToolBar *>(w))
        if (QMainWindow *mwin = qobject_cast<QMainWindow *>(tb->parentWidget()))
            if (mwin->toolBarArea(const_cast<QToolBar *>(tb)) != Qt::TopToolBarArea)
                if (tb->orientation() != Qt::Horizontal)
                    return false;

    if (!w->isWindow() && w->height() > w->width())
        return false;

    if (qobject_cast<QMainWindow *>(w->parentWidget()) && w->parentWidget()->parentWidget())
        return false;

    QWidget *win(w->window());

    QVariant var(win->property("DSP_UNOheight"));
    if (var.isValid())
    {
        p->save();
        p->setOpacity(opacity);
        p->drawTiledPixmap(r, s_pix.value(var.toInt()), QPoint(0, offset));
        p->setOpacity(1.0f);
        if (Settings::conf.contAware)
        if (w->mapTo(win, w->rect().topLeft()).y() < ScrollWatcher::paintRegion(qobject_cast<QMainWindow *>(win)).boundingRect().y())
        {
            p->setOpacity(0.33f);
            p->drawTiledPixmap(r, ScrollWatcher::bg((qlonglong)win));
        }
        p->restore();
        return true;
    }
    return false;
}

static QDBusMessage methodCall(const QString &method)
{
    return QDBusMessage::createMethodCall("org.kde.kwin", "/StyleProjectFactory", "org.kde.DBus.StyleProjectFactory", method);
}

void
UNOHandler::updateWindow(WId window)
{
    QDBusMessage msg = methodCall("update");
    msg << QVariant::fromValue((unsigned int)window);
    QDBusConnection::sessionBus().send(msg);
}


static unsigned int getHeadHeight(QWidget *win, unsigned int &needSeparator)
{
    unsigned char *h = XHandler::getXProperty<unsigned char>(win->winId(), XHandler::DecoData);
    if (!h && !(Settings::conf.removeTitleBars && win->property("DSP_hasbuttons").toBool()))
        return 0;

    unsigned int totheight(h?*h:0);
    win->setProperty("titleHeight", totheight);
    int tbheight(0);
    if (castObj(QMainWindow *, mw, win))
    {
        if (mw->menuBar() && mw->menuBar()->isVisible())
            totheight += mw->menuBar()->height();

        QList<QToolBar *> tbs(mw->findChildren<QToolBar *>());
        for (int i = 0; i<tbs.count(); ++i)
        {
            QToolBar *tb(tbs.at(i));
            if (tb->isVisible())
                if (!tb->findChild<QTabBar *>())
                    if (mw->toolBarArea(tb) == Qt::TopToolBarArea)
                    {
                        if (tb->geometry().bottom() > tbheight)
                            tbheight = tb->height();
                        needSeparator = 0;
                    }
        }
    }
    totheight += tbheight;
    if (Ops::isSafariTabBar(win->findChild<QTabBar *>()))
        needSeparator = 0;
    win->setProperty("DSP_headHeight", tbheight);
    win->setProperty("DSP_UNOheight", totheight);
    return totheight;
}

void
UNOHandler::fixWindowTitleBar(QWidget *win)
{
    if (!win || !win->isWindow())
        return;
    unsigned int ns(1);
    WindowData wd;
    wd.height = getHeadHeight(win, ns);
    wd.separator = ns;
    wd.opacity = win->testAttribute(Qt::WA_TranslucentBackground)?(unsigned int)(Settings::conf.opacity*100.0f):100;
    wd.top = Color::titleBarColors[0].rgba();
    wd.bottom = Color::titleBarColors[1].rgba();
    wd.text = win->palette().color(win->foregroundRole()).rgba();
    if (!wd.height)
        return;
    XHandler::setXProperty<unsigned int>(win->winId(), XHandler::WindowData, XHandler::Short, reinterpret_cast<unsigned int *>(&wd), 6);
    updateWindow(win->winId());
//    qDebug() << ((c & 0xff000000) >> 24) << ((c & 0xff0000) >> 16) << ((c & 0xff00) >> 8) << (c & 0xff);
//    qDebug() << QColor(c).alpha() << QColor(c).red() << QColor(c).green() << QColor(c).blue();
    if (!s_pix.contains(wd.height))
    {
        QLinearGradient lg(0, 0, 0, wd.height);
        lg.setColorAt(0.0f, Color::titleBarColors[0]);
        lg.setColorAt(1.0f, Color::titleBarColors[1]);
        QPixmap p(Render::noise().width(), wd.height);
        p.fill(Qt::transparent);
        QPainter pt(&p);
        pt.fillRect(p.rect(), lg);
        pt.end();
        s_pix.insert(wd.height, Render::mid(p, Render::noise(), 40, 1));
    }

    const QList<QToolBar *> toolBars = win->findChildren<QToolBar *>();
    for (int i = 0; i < toolBars.size(); ++i)
        toolBars.at(i)->update();
    if (QMenuBar *mb = win->findChild<QMenuBar *>())
        mb->update();
}

void
UNOHandler::updateToolBar(QToolBar *toolBar)
{
    QMainWindow *win = static_cast<QMainWindow *>(toolBar->parentWidget());
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
                toolBar->setMovable(false);
                const int pm(6/*toolBar->style()->pixelMetric(QStyle::PM_ToolBarFrameWidth)*/);
                toolBar->layout()->setContentsMargins(pm, pm, pm, pm);
                toolBar->setContentsMargins(0, 0, 0, 0);
            }
            else
            {
                toolBar->setMovable(true);
                toolBar->layout()->setContentsMargins(0, 0, 0, 0);
                toolBar->setContentsMargins(0, 0, toolBar->style()->pixelMetric(QStyle::PM_ToolBarHandleExtent), 6);
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
}

void
UNOHandler::fixTitleLater(QWidget *win)
{
    if (!win)
        return;
    QTimer *t = new QTimer(win);
    connect(t, SIGNAL(timeout()), instance(), SLOT(fixTitle()));
    t->start(250);
}

void
UNOHandler::fixTitle()
{
    if (QTimer *t = qobject_cast<QTimer *>(sender()))
    if (t->parent()->isWidgetType())
    if (QWidget *w = static_cast<QWidget *>(t->parent()))
    if (w->isWindow())
    {
        fixWindowTitleBar(w);
        t->stop();
        t->deleteLater();
    }
}

//-------------------------------------------------------------------------------------------------

Q_DECL_EXPORT WinHandler WinHandler::s_instance;

WinHandler::WinHandler(QObject *parent)
    : QObject(parent)
{
}

void
WinHandler::manage(QWidget *w)
{
    if (!w)
        return;
    w->removeEventFilter(instance());
    w->installEventFilter(instance());
}

void
WinHandler::release(QWidget *w)
{
    if (!w)
        return;
    w->removeEventFilter(instance());
}

WinHandler
*WinHandler::instance()
{
    return &s_instance;
}

bool
WinHandler::eventFilter(QObject *o, QEvent *e)
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
        if (castObj(QTabBar *, tb, w))
            if (tb->tabAt(me->pos()) != -1)
                return false;
        if (w->cursor().shape() != Qt::ArrowCursor
                || QApplication::overrideCursor()
                || w->mouseGrabber()
                || me->button() != Qt::LeftButton)
            return false;
        XHandler::mwRes(me->globalPos(), w->window()->winId());
        return false;
    }
    case QEvent::Paint:
    {
        if (qobject_cast<QDialog *>(w)
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
            return true;
        }
        return false;
    }
    case QEvent::Show:
    {
        if (Settings::conf.hackDialogs)
        if (qobject_cast<QDialog *>(w)
                && w->isModal())
        {
            QWidget *p = w->parentWidget();
            if  (!p)
                p = qApp->activeWindow();
            if (!p || p == w || p->objectName() == "BE::Desk")
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
            if (p->windowFlags() & Qt::FramelessWindowHint
                    && qobject_cast<QMainWindow *>(p))
                if (QToolBar *bar = p->findChild<QToolBar *>())
                    if (bar->orientation() == Qt::Horizontal
                            && bar->geometry().topLeft() == QPoint(0, 0))
                        y+=bar->height();
            int x(p->mapToGlobal(p->rect().center()).x()-(w->width()/2));
            w->move(x, y);
        }
        return false;
    }
    default: break;
    }
    return false;
}

bool
WinHandler::canDrag(QWidget *w)
{
    if (qobject_cast<QToolBar *>(w)
            || qobject_cast<QLabel *>(w)
            || qobject_cast<QTabBar *>(w)
            || qobject_cast<QStatusBar *>(w)
            || qobject_cast<QDialog *>(w))
        return true;
    return false;
}

//-------------------------------------------------------------------------------------------------

Q_DECL_EXPORT ScrollWatcher ScrollWatcher::s_instance;
QMap<qlonglong, QPixmap> ScrollWatcher::s_winBg;

ScrollWatcher::ScrollWatcher(QObject *parent) : QObject(parent), m_timer(new QTimer(this))
{
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updateLater()));
}

void
ScrollWatcher::watch(QAbstractScrollArea *area)
{
    if (!Settings::conf.contAware)
        return;
    area->window()->removeEventFilter(instance());
    area->window()->installEventFilter(instance());
    connect(area->window(), SIGNAL(destroyed()), instance(), SLOT(removeFromQueue()));
    area->viewport()->removeEventFilter(instance());
    area->viewport()->installEventFilter(instance());
}

void
ScrollWatcher::removeFromQueue()
{
    QMainWindow *mw(static_cast<QMainWindow *>(sender()));
    if (m_wins.contains(mw))
        m_wins.removeOne(mw);
}

void
ScrollWatcher::updateLater()
{
    for (int i = 0; i < m_wins.count(); ++i)
        updateWin(m_wins.at(i));
    m_wins.clear();
}

void
ScrollWatcher::updateWin(QMainWindow *win)
{
    regenBg(win);
    const QList<QToolBar *> toolBars(win->findChildren<QToolBar *>());
    for (int i = 0; i < toolBars.count(); ++i)
        toolBars.at(i)->update();
    if (QStatusBar *sb = win->findChild<QStatusBar *>())
        sb->update();
    if (QTabBar *tb = win->findChild<QTabBar *>())
        tb->update();
}

void
ScrollWatcher::regenBg(QMainWindow *win)
{
    const QRegion clipReg(paintRegion(win));
    const QRect bound(clipReg.boundingRect());
//    QImage img(win->size()/*+QSize(0, 22)*/, QImage::Format_ARGB32);
    QImage img(win->width(), win->height()-(bound.height()-2), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setClipRegion(clipReg/*.translated(0, 22)*/);
    win->render(&p/*, QPoint(0, 22)*/);
    p.end();
    img = img.copy(0, clipReg.boundingRect().top()+1, img.width(), 1);
    img = Render::blurred(img, img.rect(), 128);

    QPixmap pix(QPixmap::fromImage(img));
    QPixmap decoPix = XHandler::x11Pix(pix/*.copy(0, 0, win->width(), 1)*/);
    unsigned long d(decoPix.handle());
    XHandler::setXProperty<unsigned long>(win->winId(), XHandler::DecoBgPix, XHandler::LongLong, &d);
    UNOHandler::updateWindow(win->winId());
    decoPix.detach();
    s_winBg.insert((qlonglong)win, pix);
}

QPixmap
ScrollWatcher::bg(qlonglong win)
{
    return s_winBg.value(win, QPixmap());
}

bool
ScrollWatcher::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Resize && qobject_cast<QMainWindow *>(o))
    {
        QMainWindow *win(static_cast<QMainWindow *>(o));
        if (!m_wins.contains(win))
            m_wins << win;
        m_timer->start(50);
    }
    if (e->type() == QEvent::Paint && !qobject_cast<QMainWindow *>(o))
    {
        QMainWindow *win(qobject_cast<QMainWindow *>(static_cast<QWidget *>(o)->window()));
        if (!m_wins.contains(win))
            m_wins << win;
        m_timer->start(50);
    }
    return false;
}

QRegion
ScrollWatcher::paintRegion(QMainWindow *win)
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
