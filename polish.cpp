#include <QToolBar>
#include <QFrame>
#include <QMainWindow>
#include <QLayout>
#include <QMenuBar>
#include <QToolButton>
#include <QAbstractItemView>
#include <QGroupBox>
#include <QCheckBox>
#include <QDockWidget>
#include <QDesktopWidget>
#include <QLabel>
#include <QProgressBar>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QScrollBar>
#include <QTextEdit>
#include <QDialog>
#include <QStatusBar>
#include <QToolBox>

#include "styleproject.h"
#include "overlay.h"
#include "stylelib/ops.h"
#include "stylelib/shadowhandler.h"
#include "stylelib/progresshandler.h"
#include "stylelib/animhandler.h"
#include "stylelib/unohandler.h"
#include "stylelib/xhandler.h"
#include "stylelib/settings.h"
#include "stylelib/color.h"

/* Thomas gave me his blessing to use
 * his macmenu! yeah! so now we get
 * macmenues in styleproject!
 */
#if !defined(QT_NO_DBUS)
#include "macmenu.h"
#endif


/* a non-const environment that is called
 * for for every widget directly after its
 * creation, one can manipulate them in
 * pretty much any way here.
 */

#define installFilter(_VAR_) { _VAR_->removeEventFilter(this); _VAR_->installEventFilter(this); }

void
StyleProject::polish(QWidget *widget)
{
    if (!widget)
        return;


//    qDebug() << widget;
//    if (widget->parentWidget())
//        qDebug() << widget << widget->parentWidget();

//    installFilter(widget);
    /* needed for mac os x lion like toolbuttons,
     * we need to repaint the toolbar when a action
     * has been triggered, otherwise we are painting
     * errors
     */
    if (castObj(QToolBar *, bar, widget))
    {
        if (qobject_cast<QMainWindow *>(widget->parentWidget()))
            connect(bar, SIGNAL(topLevelChanged(bool)), this, SLOT(fixMainWindowToolbar()));
        installFilter(bar);
        bar->setForegroundRole(QPalette::WindowText);
        bar->setBackgroundRole(QPalette::Window);
    }
    if (widget->inherits("KMultiTabBarTab"))
    {
        installFilter(widget);
        if (QFrame *frame = qobject_cast<QFrame *>(widget->parentWidget()))
        {
            frame->setFrameShadow(QFrame::Sunken);
            frame->setFrameShape(QFrame::StyledPanel);
            OverLay::manage(frame, 255.0f*Settings::conf.shadows.opacity);
            installFilter(frame);
        }
    }

    if (castObj(QStatusBar *, bar, widget))
    {
        bar->setForegroundRole(QPalette::WindowText);
        bar->setBackgroundRole(QPalette::Window);
    }
    if (Settings::conf.splitterExt)
    if (widget->objectName() == "qt_qmainwindow_extended_splitter" || qobject_cast<QSplitterHandle *>(widget))
        SplitterExt::manage(widget);
//    if (widget->layout() && (qobject_cast<QDockWidget *>(widget)||qobject_cast<QDockWidget *>(widget->parentWidget())))
//        widget->layout()->setContentsMargins(0, 0, 0, 0);

    if (qobject_cast<QPushButton *>(widget) ||
            qobject_cast<QCheckBox *>(widget) ||
            qobject_cast<QRadioButton *>(widget) ||
            qobject_cast<QSlider *>(widget) ||
            qobject_cast<QScrollBar *>(widget))
        Anim::Basic::manage(widget);
    if (castObj(QToolButton *, btn, widget))
    {
        Anim::ToolBtns::manage(btn);
        installFilter(btn);
        if (!qobject_cast<QToolBar *>(btn->parentWidget()))
        {
            btn->setBackgroundRole(QPalette::Window);
            btn->setForegroundRole(QPalette::WindowText);
        }
    }

    if (qobject_cast<QAbstractItemView *>(widget))
    {
        QPalette pal(widget->palette());
        const bool dark(Color::luminosity(pal.color(QPalette::Base)) < Color::luminosity(pal.color(QPalette::Text)));
        QColor (QColor::*function)(int) const = dark?&QColor::lighter:&QColor::darker;
        pal.setColor(QPalette::Button, (pal.color(QPalette::Base).*function)(120));
        pal.setColor(QPalette::ButtonText, pal.color(QPalette::Text));
        widget->setPalette(pal);
    }

    if (QLayout *l = widget->layout())
    {
        int m(qMax(l->contentsMargins().top(), widget->contentsMargins().top()));
        if (m < l->spacing() && qobject_cast<QVBoxLayout *>(l))
        {
            if (qobject_cast<QMainWindow *>(widget->window()))
            {
                l->setSpacing(m);
//                if (!widget->mapTo(widget->window(), QPoint()).y()) //at top of window, hi telepathy contactlist o/
//                {
//                    int l, t, r, b;
//                    widget->getContentsMargins(&l, &t, &r, &b);
//                    widget->setContentsMargins(l, t+2, r, b);
//                }
            }
        }
    }
    if (Settings::conf.uno.enabled &&
            (qobject_cast<QMainWindow *>(widget) ||
            widget->findChild<QToolBar *>() ||
            widget->findChild<QTabBar *>()))
        UNO::Handler::manage(widget);

    if (Settings::conf.uno.contAware && qobject_cast<QMainWindow *>(widget->window()) && qobject_cast<QAbstractScrollArea *>(widget))
        ScrollWatcher::watch(static_cast<QAbstractScrollArea *>(widget));

    if (WinHandler::canDrag(widget))
        WinHandler::manage(widget);

    if (/*castObj(QMainWindow *, win, widget)*/widget->isWindow())
    {
        if (!(Settings::conf.uno.enabled && !qobject_cast<QMainWindow *>(widget)))
        {
            if (Settings::conf.compactMenu && widget->findChild<QMenuBar *>())
                WinHandler::addCompactMenu(widget);
            if (XHandler::opacity() < 1.0f && !widget->parentWidget())
            {
                const QIcon icn = widget->windowIcon();
                const bool wasVisible= widget->isVisible();
                const bool wasMoved = widget->testAttribute(Qt::WA_Moved);
                if (wasVisible)
                    widget->hide();
                widget->setAttribute(Qt::WA_TranslucentBackground);
                widget->setWindowIcon(icn);
                widget->setAttribute(Qt::WA_Moved, wasMoved); // https://bugreports.qt-project.org/browse/QTBUG-34108
                widget->setVisible(wasVisible);
                QTimer::singleShot(0, widget, SLOT(update()));
                unsigned int d(0);
                XHandler::setXProperty<unsigned int>(widget->winId(), XHandler::KwinBlur, XHandler::Long, &d);
            }
        }
        installFilter(widget);
    }

    if (castObj(QHeaderView *, hw, widget))
    {
        hw->viewport()->setBackgroundRole(QPalette::Base);
        hw->viewport()->setForegroundRole(QPalette::Text);
    }

    if (castObj(QProgressBar *, pBar, widget))
        ProgressHandler::manage(pBar);

    if (castObj(QComboBox *, cBox, widget))
        if (!cBox->isEditable())
        {
            Anim::Basic::manage(cBox);
            cBox->setForegroundRole(QPalette::ButtonText);
            cBox->setBackgroundRole(QPalette::Button);
        }

//    if (castObj(QMainWindow *, mwin, widget))
//    {
//        if (QWidget *c = mwin->centralWidget())
//        {
//            c->setContentsMargins(0, 0, 0, 0);
//            if (QLayout *l = c->layout())
//            {
//                l->setContentsMargins(0, 0, 0, 0);
//                l->setSpacing(0);
//            }
//        }
//    }

    /** TODO:
     *  Now the shadowhandler handles *ALL*
     *  windows... that we dont want, we
     *  dont want desktop panels etc to have a
     *  shadow... fix.
     *
     * EDIT: started working on it
     */
    if (widget->isWindow())
    {
        if (!Settings::conf.uno.enabled)
        {
            UNO::Handler::fixTitleLater(widget);
//            if (XHandler::opacity() < 1.0f)
//            {
//                widget->setAttribute(Qt::WA_TranslucentBackground);
//                UNO::Handler::fixTitleLater(widget);
//                unsigned int d(0);
//                XHandler::setXProperty<unsigned int>(widget->winId(), XHandler::KwinBlur, XHandler::Long, &d);
//            }
        }
        installFilter(widget);
        bool needShadows(false);
        if (widget->windowType() == Qt::ToolTip && widget->inherits("QTipLabel"))
        {
            widget->setAttribute(Qt::WA_TranslucentBackground);
            needShadows = true;
        }
        if (Settings::conf.hackDialogs && qobject_cast<QDialog *>(widget))
        {
            WinHandler::manage(widget);
            needShadows = true;
        }
        if (qobject_cast<QMenu *>(widget)
                || qobject_cast<QDockWidget *>(widget)
                || qobject_cast<QToolBar *>(widget)
                || (qobject_cast<QMainWindow *>(widget) && Settings::conf.removeTitleBars))
            needShadows = true;
        if (needShadows)
            ShadowHandler::manage(widget);
    }
    if (castObj(QDockWidget *,dock, widget))
        dock->setAutoFillBackground(false);
    if (castObj(QScrollBar *, sb, widget))
        sb->setAttribute(Qt::WA_Hover);

    if (widget->inherits("KTitleWidget"))
    {
        installFilter(widget);
        QList<QFrame *> children = widget->findChildren<QFrame *>();
        for (int i = 0; i < children.size(); ++i)
        {
            QFrame *f(children.at(i));
            if (f->autoFillBackground())
                f->setFrameStyle(QFrame::Sunken|QFrame::StyledPanel);
            if (QLabel *l = qobject_cast<QLabel *>(f))
            {
                l->setAlignment(Qt::AlignCenter);
                l->setProperty("DSP_KTitleLabel", true);
                installFilter(l);
            }
        }
    }
    if (widget->inherits("KUrlNavigator"))
        if (widget->parentWidget() && widget->parentWidget()->size() == widget->size())
            widget->parentWidget()->setAutoFillBackground(false); //gwenview kurlnavigator parent seems to be an idiot... oh well
    if (Settings::conf.uno.enabled)
    if (castObj(QFrame *, frame, widget))
    {
        if (frame->frameShadow() == QFrame::Sunken
                && qobject_cast<QMainWindow *>(frame->window()))
            OverLay::manage(frame, Settings::conf.shadows.opacity*255.0f);
    }
    if (castObj(QAbstractItemView *, view, widget))
    {
        view->viewport()->setAttribute(Qt::WA_Hover);
        view->setAttribute(Qt::WA_MouseTracking);
        installFilter(view);
    }
    if (castObj(QComboBox *, box, widget))
        if (QLineEdit *l = box->findChild<QLineEdit *>())
            connect(l, SIGNAL(textChanged(QString)), box, SLOT(update()));
    if (qobject_cast<QMenu *>(widget)||qobject_cast<QMenuBar *>(widget))
    {     
        widget->setMouseTracking(true);
        widget->setAttribute(Qt::WA_Hover);
        if (qobject_cast<QMenu *>(widget))
        {
            widget->setAttribute(Qt::WA_TranslucentBackground);
            widget->setForegroundRole(QPalette::Text);
            widget->setBackgroundRole(QPalette::Base);
        }
        else
        {
            widget->setForegroundRole(QPalette::WindowText);
            widget->setBackgroundRole(QPalette::Window);
        }
        /**
         * Apparently some menues fight back and dont
         * want mousetracking, we filter and set mousetracking
         * when the menu is shown.
         */
        installFilter(widget);
    }
    if (castObj(QToolBox *, tb, widget))
    {
        tb->setBackgroundRole(QPalette::Window);
        tb->setForegroundRole(QPalette::WindowText);
        if (QLayout *l = tb->layout())
        {
            l->setMargin(0);
            l->setSpacing(0);
        }
    }
    if (widget->inherits("KTabWidget"))
        installFilter(widget);
    if (castObj(QTabBar *, tabBar, widget))
    {
//        tabBar->setTabsClosable(true);
//        QFont f(tabBar->font());
//        f.setBold(true);
//        tabBar->setFont(f);
//        installFilter(tabBar);
//        if (tabBar->documentMode())
//            tabBar->setExpanding(true);
        const bool safari(Ops::isSafariTabBar(tabBar)); //hmmm
        if (safari || tabBar->documentMode())
        {
            tabBar->setBackgroundRole(QPalette::Window);
            tabBar->setForegroundRole(QPalette::WindowText);
        }
        else
        {
            tabBar->setBackgroundRole(QPalette::Button);
            tabBar->setForegroundRole(QPalette::ButtonText);
        }
        Anim::Tabs::manage(tabBar);
        tabBar->setAttribute(Qt::WA_Hover);
        tabBar->setAttribute(Qt::WA_MouseTracking);
        installFilter(tabBar);

        if (!qApp->applicationName().compare("konsole", Qt::CaseInsensitive))
        {
            if (safari) //hmmm
            {
                QWidget *p(tabBar->parentWidget());
                if (p)
                {
                    p->setObjectName("konsole_tabbar_parent");
                    installFilter(p);
                }
            }
        }
    }

#if !defined(QT_NO_DBUS)
    if (castObj(QMenuBar *, menuBar, widget))
        Bespin::MacMenu::manage(menuBar);
#endif
    QCommonStyle::polish(widget);
}

void
StyleProject::unpolish(QWidget *widget)
{
    if (!widget)
        return;
    widget->disconnect(this);
    widget->removeEventFilter(this);
    ShadowHandler::release(widget);
    Anim::Basic::release(widget);
    UNO::Handler::release(widget);
    WinHandler::release(widget);
    if (castObj(QProgressBar *, pb, widget))
        ProgressHandler::release(pb);
    if (castObj(QTabBar *, tb, widget))
        Anim::Tabs::release(tb);
    if (castObj(QFrame *, f, widget))
        OverLay::release(f);
    if (castObj(QToolButton *, tb, widget))
        Anim::ToolBtns::release(tb);

#if !defined(QT_NO_DBUS)
    if (QMenuBar *menuBar = qobject_cast<QMenuBar *>(widget))
        Bespin::MacMenu::release(menuBar);
#endif
    QCommonStyle::unpolish(widget);
}
