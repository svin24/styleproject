#include <QDebug>
#include <QTabBar>
#include <QTabWidget>
#include <QStyleOption>
#include <QStyleOptionTab>
#include <QStyleOptionTabBarBase>
#include <QStyleOptionTabBarBaseV2>
#include <QStyleOptionTabV2>
#include <QStyleOptionTabV3>
#include <QStyleOptionTabWidgetFrame>
#include <QStyleOptionTabWidgetFrameV2>
#include <QMainWindow>
#include <QToolBar>
#include <QPaintDevice>
#include <QPainter>
#include <QApplication>
#include <QStyleOptionToolBoxV2>
#include <QToolBox>

#include "styleproject.h"
#include "stylelib/ops.h"
#include "stylelib/render.h"
#include "stylelib/color.h"
#include "stylelib/animhandler.h"
#include "stylelib/xhandler.h"
#include "config/settings.h"
#include "stylelib/handlers.h"

bool
StyleProject::drawTab(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
//    return false;
    drawTabShape(option, painter, widget);
    drawTabLabel(option, painter, widget);
    return true;
}

bool
StyleProject::drawSafariTab(const QStyleOptionTab *opt, QPainter *painter, const QTabBar *bar) const
{
    const bool isFirst(opt->position == QStyleOptionTab::Beginning),
            isOnly(opt->position == QStyleOptionTab::OnlyOneTab),
            isLast(opt->position == QStyleOptionTab::End),
            isSelected(opt->state & State_Selected),
            isLeftOf(bar->tabAt(opt->rect.center()) < bar->currentIndex());
    int o(pixelMetric(PM_TabBarTabOverlap, opt, bar));
    QRect r(opt->rect.adjusted(0, 0, 0, !isSelected));
    int leftMargin(o), rightMargin(o);
    if (isFirst || isOnly)
        leftMargin /= 2;
    if ((isLast || isOnly) && bar->expanding())
        rightMargin /= 2;
    if (!bar->expanding())
    {
        --leftMargin;
        ++rightMargin;
    }
    r.setLeft(r.left()-leftMargin);
    r.setRight(r.right()+rightMargin);

    QPainterPath p;
    Render::renderTab(r, painter, isLeftOf ? Render::BeforeSelected : isSelected ? Render::Selected : Render::AfterSelected, &p, dConf.shadows.opacity);
    if (isSelected)
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        QPixmap pix(r.size());
        pix.fill(Qt::transparent);
        QPainter pt(&pix);
        Handlers::Window::drawUnoPart(&pt, pix.rect(), bar, bar->mapTo(bar->window(), r.topLeft()), XHandler::opacity());
        pt.end();
        if (XHandler::opacity() < 1.0f)
        {
            painter->setCompositionMode(QPainter::CompositionMode_DestinationOut);
            painter->setBrush(Qt::black);
            painter->drawPath(p);
            painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        }
        painter->setBrushOrigin(r.topLeft());
        painter->setBrush(pix);
        painter->drawPath(p);
        painter->restore();
    }
    return true;
}

bool
StyleProject::drawSelector(const QStyleOptionTab *opt, QPainter *painter, const QTabBar *bar) const
{
    const bool isFirst(opt->position == QStyleOptionTab::Beginning),
            isOnly(opt->position == QStyleOptionTab::OnlyOneTab),
            isLast(opt->position == QStyleOptionTab::End),
            isRtl(opt->direction == Qt::RightToLeft),
            isSelected(opt->state & State_Selected);

    Render::Sides sides(Render::All);
    QPalette::ColorRole fg(Ops::fgRole(bar, QPalette::ButtonText)),
            bg(Ops::bgRole(bar, QPalette::Button));

    if (isSelected)
        Ops::swap(fg, bg);

    QColor bgc(opt->palette.color(bg));
    QColor sc = Color::mid(bgc, opt->palette.color(QPalette::Highlight), 2, 1);
    if (opt->ENABLED && !(opt->state & State_On) && bar)
    {
        int hl(Anim::Tabs::level(bar, bar->tabAt(opt->rect.center())));
        bgc = Color::mid(bgc, sc, STEPS-hl, hl);
    }
    QLinearGradient lg;
    QRect r(opt->rect);
    bool vert(false);
    switch (opt->shape)
    {
    case QTabBar::RoundedNorth:
    case QTabBar::TriangularNorth:
    case QTabBar::RoundedSouth:
    case QTabBar::TriangularSouth:
        lg = QLinearGradient(0, 0, 0, Render::maskHeight(dConf.tabs.shadow, r.height()));
        if (isOnly)
            break;
        if (isRtl)
            sides &= ~((!isFirst*Render::Right)|(!isLast*Render::Left));
        else
            sides &= ~((!isFirst*Render::Left)|(!isLast*Render::Right));
        break;
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
        lg = QLinearGradient(0, 0, Render::maskWidth(dConf.tabs.shadow, r.width()), 0);
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        if (opt->shape != QTabBar::RoundedWest && opt->shape != QTabBar::TriangularWest)
        {
            lg = QLinearGradient(Render::maskWidth(dConf.tabs.shadow, r.width()), 0, 0, 0);
        }
        vert = true;
        if (isOnly)
            break;
        sides &= ~((!isLast*Render::Bottom)|(!isFirst*Render::Top));
        break;
    default: break;
    }
    lg.setStops(Settings::gradientStops(dConf.tabs.gradient, bgc));
    QBrush b(lg);
    Render::drawClickable(dConf.tabs.shadow, r, painter, dConf.tabs.rnd, dConf.shadows.opacity, bar, opt, &b, 0, sides);
    const QRect mask(Render::maskRect(dConf.tabs.shadow, r, sides));
    if (isSelected && !isOnly)
    {
        const QPen pen(painter->pen());
        painter->setPen(QColor(0, 0, 0, 32)/*Color::mid(bgc, fgc)*/);
        if (!isFirst)
            painter->drawLine(vert?mask.topLeft():isRtl?mask.topRight():mask.topLeft(), vert?mask.topRight():isRtl?mask.bottomRight():mask.bottomLeft());
        if (!isLast)
            painter->drawLine(vert?mask.bottomLeft():isRtl?mask.topLeft():mask.topRight(), vert?mask.bottomRight():isRtl?mask.bottomLeft():mask.bottomRight());
        painter->setPen(pen);
    }
    else if (opt->selectedPosition != QStyleOptionTab::NextIsSelected && opt->position != QStyleOptionTab::End && !isOnly)
    {
        const QPen pen(painter->pen());
        painter->setPen(QColor(0, 0, 0, 32)/*Color::mid(bgc, fgc)*/);
        painter->drawLine(vert?mask.bottomLeft():isRtl?mask.topLeft():mask.topRight(), vert?mask.bottomRight():isRtl?mask.bottomLeft():mask.bottomRight());
        painter->setPen(pen);
    }
    return true;
}

bool
StyleProject::drawTabShape(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionTab *opt = qstyleoption_cast<const QStyleOptionTab *>(option);
    if (!opt)
        return true;
    if (opt->position == QStyleOptionTab::OnlyOneTab) //if theres only one tab then thats the selected one right? right? am I missing something?
        const_cast<QStyleOption *>(option)->state |= State_Selected;
    const QTabBar *bar = qobject_cast<const QTabBar *>(widget);
    if (Ops::isSafariTabBar(bar))
        return drawSafariTab(opt, painter, bar);
    if (styleHint(SH_TabBar_Alignment, option, widget) == Qt::AlignCenter)
        return drawSelector(opt, painter, bar);

    const bool isFirst(opt->position == QStyleOptionTab::Beginning),
            isOnly(opt->position == QStyleOptionTab::OnlyOneTab),
            isLast(opt->position == QStyleOptionTab::End),
            isRtl(opt->direction == Qt::RightToLeft),
            isSelected(opt->state & State_Selected);
    const int topMargin(!isSelected*2);
    const int level(isSelected?STEPS:bar?Anim::Tabs::level(bar, bar->tabAt(opt->rect.topLeft())):0);
    Render::renderShadow(Render::Raised, opt->rect.adjusted(0, topMargin, 0, -4), painter, 4, Render::All & ~Render::Bottom, dConf.shadows.opacity);
    const QRect maskRect(opt->rect.adjusted(2, 1+topMargin, -2, -4));
    QLinearGradient lg(0, 0, 0, isSelected?opt->rect.height():maskRect.height());
    QColor c(opt->palette.color(QPalette::Window));
    QColor c2 = Color::mid(c, opt->palette.color(QPalette::Highlight), 2, 1);
    c = Color::mid(c, c2, STEPS-level, level);
    lg.setStops(Settings::gradientStops(dConf.tabs.gradient, c));
    Render::renderMask(maskRect, painter, lg, 2, Render::All & ~Render::Bottom);
    if (!isSelected)
        Render::renderShadow(Render::Raised, opt->rect.adjusted(2, opt->rect.bottom()-6, -2, 0), painter, 4, Render::Top, dConf.shadows.opacity);
    return true;
}

bool
StyleProject::isVertical(const QStyleOptionTabV3 *tab, const QTabBar *bar)
{
    if (!tab)
        return false;
    QTabBar::Shape tabShape(bar ? bar->shape() : tab ? tab->shape : QTabBar::RoundedNorth);
    switch (tabShape)
    {
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        return true;
    default:
        return false;
    }
}

bool
StyleProject::drawTabLabel(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionTabV3 *opt = qstyleoption_cast<const QStyleOptionTabV3 *>(option);
    if (!opt)
        return true;
    const QTabBar *bar = qobject_cast<const QTabBar *>(widget);
    painter->save();
    const bool isSelected(opt->state & State_Selected),
            leftClose(styleHint(SH_TabBar_CloseButtonPosition, opt, widget) == QTabBar::LeftSide && bar && bar->tabsClosable());
    QRect ir(subElementRect(leftClose?SE_TabBarTabRightButton:SE_TabBarTabLeftButton, opt, widget));
    QRect tr(subElementRect(SE_TabBarTabText, opt, widget));
    int trans(0);
    bool west(false), east(false);
    switch (opt->shape)
    {
    case QTabBar::RoundedWest:
    case QTabBar::TriangularWest:
        west = true;
        trans = -90;
        break;
    case QTabBar::RoundedEast:
    case QTabBar::TriangularEast:
        east = true;
        trans = 90;
        break;
    default: break;
    }
    if (qAbs(trans))
    {
        painter->translate(tr.center());
        painter->rotate(trans);
        painter->translate(-tr.center());
        QSize sz(tr.size());
        const QPoint center(tr.center());
        sz.transpose();
        tr.setSize(sz);
        tr.moveCenter(center);
    }
    QFont f(painter->font());
    if (isSelected)
    {
        f.setBold(true);
        painter->setFont(f);
    }
    QPalette::ColorRole fg(Ops::fgRole(widget, QPalette::ButtonText));
    const bool safari(Ops::isSafariTabBar(bar));
    bool needRestore(false);
    if (safari || styleHint(SH_TabBar_Alignment, opt, widget) == Qt::AlignLeft)
        fg = QPalette::WindowText;
    else if (isSelected)
        fg = Ops::opposingRole(fg);

    const QFontMetrics fm(painter->fontMetrics());
    Qt::TextElideMode elide((Qt::TextElideMode)styleHint(SH_TabBar_ElideMode, opt, widget));
    if (isVertical(opt, bar))
        elide = Qt::ElideRight;
    const QString text(fm.elidedText(opt->text, elide, tr.width()));
    drawItemText(painter, tr, Qt::AlignCenter, option->palette, option->ENABLED, text, fg);
    if (!opt->icon.isNull())
        drawItemPixmap(painter, ir, Qt::AlignCenter, opt->icon.pixmap(opt->iconSize));
    painter->restore();
    return true;
}

static void drawDocTabBar(QPainter *p, const QTabBar *bar, QRect rect = QRect())
{
    QRect r(rect.isValid()?rect:bar->rect());
    if (Ops::isSafariTabBar(bar))
    {
        if (XHandler::opacity() < 1.0f)
        {
            p->setCompositionMode(QPainter::CompositionMode_DestinationOut);
            p->fillRect(r, Qt::black);
            p->setCompositionMode(QPainter::CompositionMode_SourceOver);
        }
        const float o(p->opacity());
        Handlers::Window::drawUnoPart(p, r, bar, bar->mapTo(bar->window(), bar->rect().topLeft()), XHandler::opacity());
        p->setPen(Qt::black);
        p->setOpacity(dConf.shadows.opacity/2);
        p->drawLine(r.topLeft(), r.topRight());
        p->setOpacity(dConf.shadows.opacity);
        p->drawLine(r.bottomLeft(), r.bottomRight());
        p->setOpacity(o);
        Render::renderShadow(Render::Sunken, r, p, 32, Render::Top, dConf.shadows.opacity/2);
    }
    else if (bar->documentMode())
    {
//        p->save();
//        QLinearGradient lg(r.topLeft(), r.bottomLeft());
//        lg.setStops(Settings::gradientStops(dConf.tabs.gradient, bar->palette().color(QPalette::Window)));
//        p->fillRect(r, lg);
//        p->setPen(QColor(0, 0, 0, 255.0f*dConf.shadows.opacity));
//        p->drawLine(r.topLeft(), r.topRight());
//        p->drawLine(r.bottomLeft(), r.bottomRight());
//        p->setPen(QColor(255, 255, 255, 127.0f*dConf.shadows.opacity));
//        r.setTop(r.top()+1);
//        p->drawLine(r.topLeft(), r.topRight());
//        p->restore();

        QRect rt(r.adjusted(0, r.height()-6, 0, 0));
        Render::renderShadow(Render::Raised, rt, p, 2, Render::All & ~Render::Bottom, dConf.shadows.opacity);
        rt.adjust(0, 2, 0, 0);
        QLinearGradient lg(0, 0, 0, bar->height());
        QColor c(Color::mid(bar->palette().color(QPalette::Window), bar->palette().color(QPalette::Highlight), 2, 1));
        lg.setStops(Settings::gradientStops(dConf.pushbtn.gradient, c));
        Render::renderMask(rt, p, lg, 0, Render::All & ~Render::Bottom, -QPoint(0, bar->height()-4));
    }
}

bool
StyleProject::drawTabBar(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (widget && widget->parentWidget() && widget->parentWidget()->inherits("KTabWidget"))
        return true;

    const QStyleOptionTabBarBaseV2 *opt = qstyleoption_cast<const QStyleOptionTabBarBaseV2 *>(option);
    if (!opt)
        return true;

    if (const QTabBar *tabBar = qobject_cast<const QTabBar *>(widget))
    {
        if (tabBar->documentMode() || styleHint(SH_TabBar_Alignment, opt, widget) == Qt::AlignLeft)
        {
            QRect r(tabBar->rect());
            if (opt->rect.width() > r.width())
                r = opt->rect;

            drawDocTabBar(painter, tabBar, r);
            return true;
        }
    }
    return true;
}

bool
StyleProject::drawTabWidget(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    Render::Sides sides = Render::checkedForWindowEdges(widget);
    const QTabWidget *tabWidget = qobject_cast<const QTabWidget *>(widget);
    const QStyleOptionTabWidgetFrame *opt = qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option);

    if (!tabWidget || !opt)
        return true;
    const int m(2);
    QRect r(opt->rect.adjusted(m, m, -m, -m));
    QRect rect(opt->rect);
    if (const QTabBar *tabBar = tabWidget->findChild<const QTabBar *>()) //tabBar() method of tabwidget is protected...
    {
        QRect barRect(tabBar->geometry());
        const int h(barRect.height()/2), wh(barRect.width()/2);
        switch (opt->shape)
        {
        case QTabBar::RoundedNorth:
        case QTabBar::TriangularNorth:
        {
            barRect.setLeft(tabWidget->rect().left());
            barRect.setRight(tabWidget->rect().right());
            const int b(barRect.bottom()+1);
            r.setTop(b);
            rect.setTop(b-h);
            break;
        }
        case QTabBar::RoundedSouth:
        case QTabBar::TriangularSouth:
        {
            const int t(barRect.top()-1);
            r.setBottom(t);
            rect.setBottom(t+h);
            break;
        }
        case QTabBar::RoundedWest:
        case QTabBar::TriangularWest:
        {
            const int right(barRect.right()+1);
            r.setLeft(right);
            rect.setLeft(right-wh);
            break;
        }
        case QTabBar::RoundedEast:
        case QTabBar::TriangularEast:
        {
            const int l(barRect.left()-1);
            r.setRight(l);
            rect.setRight(l+wh);
            break;
        }
        default: break;
        }
        if (tabBar->documentMode())
        {
            drawDocTabBar(painter, tabBar, barRect);
            return true;
        }
    }
    if (!tabWidget->documentMode())
        Render::renderShadow(Render::Sunken, rect, painter, 7, sides, dConf.shadows.opacity);
    return true;
}

bool
StyleProject::drawTabCloser(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const int size(qMin(option->rect.width(), option->rect.height())), _2_(size/2),_4_(size/4), _8_(size/8), _16_(size/16);
    const QRect line(_2_-_16_, _4_, _8_, size-(_4_*2));

//    const bool isTabBar(widget&&qobject_cast<const QTabBar *>(widget->parentWidget()));
    bool safTabs(false);
    const QTabBar *bar(0);
    if (widget && widget->parentWidget())
    {
        bar = qobject_cast<const QTabBar *>(widget->parentWidget());
        if (bar)
            safTabs = Ops::isSafariTabBar(qobject_cast<const QTabBar *>(bar));
    }

    const bool hover(option->HOVER);

    bool isSelected(option->state & State_Selected);
    if (bar && bar->count() == 1)
        isSelected = true;

    QPixmap pix = QPixmap(option->rect.size());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.setRenderHint(QPainter::Antialiasing);
    QRect l(pix.rect().adjusted(_16_, _16_, -_16_, -_16_));
    p.drawEllipse(l);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    const int r[2] = { 45, 90 };
    for (int i = 0; i < 2; ++i)
    {
        p.translate(_2_, _2_);
        p.rotate(r[i]);
        p.translate(-_2_, -_2_);
        p.drawRect(line);
    }
    p.end();

    bool isDark(false);
    if (bar)
    {
        if (safTabs || bar->documentMode())
        {
            QWidget *d(widget->window());
            const QPalette pal(d->palette());
            isDark = Color::luminosity(pal.color(d->foregroundRole())) > Color::luminosity(pal.color(d->backgroundRole()));
        }
        else
        {
            isDark = Color::luminosity(option->palette.color(bar->foregroundRole())) > Color::luminosity(option->palette.color(bar->backgroundRole()));
            if (isSelected)
                isDark = !isDark;
        }
    }
    int cc(isDark?0:255);
    QPixmap tmp(pix);
    Render::colorizePixmap(tmp, QColor(cc, cc, cc, 127));
    QPixmap tmp2(pix);
    QPalette::ColorRole role(Ops::fgRole(bar, QPalette::WindowText));
    if (safTabs || (bar && bar->documentMode()))
        role = QPalette::WindowText;
    else if (hover)
        role = QPalette::Highlight;
    else if (isSelected)
        role = Ops::opposingRole(role);
    Render::colorizePixmap(tmp2, option->palette.color(role));

    QPixmap closer = QPixmap(option->rect.size());
    closer.fill(Qt::transparent);
    p.begin(&closer);
    p.drawTiledPixmap(closer.rect().translated(0, 1), tmp);
    p.drawTiledPixmap(closer.rect(), tmp2);
    p.end();

    if (!isSelected)
        painter->setOpacity(0.75f);
    painter->drawTiledPixmap(option->rect, closer);
    return true;
}

bool
StyleProject::drawToolBoxTab(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (option && widget && widget->parentWidget())
        const_cast<QStyleOption *>(option)->palette = widget->parentWidget()->palette();
    drawToolBoxTabShape(option, painter, widget);
    drawToolBoxTabLabel(option, painter, widget);
    return true;
}

bool
StyleProject::drawToolBoxTabShape(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionToolBoxV2 *opt = qstyleoption_cast<const QStyleOptionToolBoxV2 *>(option);
    if (!opt)
        return true;
    const QPalette pal(opt->palette);
    QRect geo(opt->rect);
    QWidget *w(0);
    QRect theRect(opt->rect);
    const bool first(opt->position == QStyleOptionToolBoxV2::Beginning);
    const bool last(opt->position == QStyleOptionToolBoxV2::End);
    if (const QToolBox *box = qobject_cast<const QToolBox *>(widget))
    {
        if (box->frameShadow() == QFrame::Sunken)
        {
            painter->fillRect(option->rect, pal.button());
            const QPen saved(painter->pen());
            painter->setPen(QColor(0, 0, 0, 255.0f*dConf.shadows.opacity));
            if (option->SUNKEN || !last)
                painter->drawLine(theRect.bottomLeft(), theRect.bottomRight());
            if (opt->selectedPosition == QStyleOptionToolBoxV2::PreviousIsSelected)
                painter->drawLine(theRect.topLeft(), theRect.topRight());
            painter->setPen(saved);
            return true;
        }
        const QList<QWidget *> tabs(widget->findChildren<QWidget *>("qt_toolbox_toolboxbutton"));
        for (int i = 0; i < tabs.size(); ++i)
        {
            QWidget *tab(tabs.at(i));
            if (tab == painter->device())
            {
                w = tab;
                geo = w->geometry();
            }
            if (tab->geometry().bottom() > theRect.bottom())
                theRect.setBottom(tab->geometry().bottom());
        }
    }

    Render::Sides sides(Render::All);
    if (!first)
        sides &= ~Render::Top;
    if (!last)
        sides &= ~Render::Bottom;

    QLinearGradient lg(0, 0, 0, Render::maskRect(dConf.tabs.shadow, theRect).height());
    lg.setStops(Settings::gradientStops(dConf.tabs.gradient, pal.color(QPalette::Button)));
    QBrush b(lg);

    Render::drawClickable(dConf.tabs.shadow, theRect, painter, qMin<int>(opt->rect.height()/2, dConf.tabs.rnd), dConf.shadows.opacity, w, option, &b, 0, sides, -geo.topLeft());
    return true;
}

bool
StyleProject::drawToolBoxTabLabel(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionToolBoxV2 *opt = qstyleoption_cast<const QStyleOptionToolBoxV2 *>(option);
    if (!opt)
        return true;

    const QPalette pal(widget?widget->palette():opt->palette);
    const bool isSelected(opt->state & State_Selected || opt->position == QStyleOptionTab::OnlyOneTab);
    QRect textRect(opt->rect.adjusted(4, 0, -4, 0));
    if (!opt->icon.isNull())
    {
        const QSize iconSize(16, 16); //no way to get this?
        const QPoint topLeft(textRect.topLeft());
        QRect r(topLeft+QPoint(0, textRect.height()/2-iconSize.height()/2), iconSize);
        opt->icon.paint(painter, r);
        textRect.setLeft(r.right());
    }
    const QFont savedFont(painter->font());
    if (isSelected)
    {
        QFont tmp(savedFont);
        tmp.setBold(true);
        painter->setFont(tmp);
    }
    drawItemText(painter, textRect, Qt::AlignCenter, pal, opt->ENABLED, opt->text, QPalette::ButtonText);
    if (isSelected)
    {
        painter->setClipRegion(QRegion(opt->rect)-QRegion(itemTextRect(painter->fontMetrics(), textRect, Qt::AlignCenter, true, opt->text)));
        int y(textRect.center().y());
        const QPen pen(painter->pen());
        QLinearGradient lg(textRect.topLeft(), textRect.topRight());
        lg.setColorAt(0.0f, Qt::transparent);
        lg.setColorAt(0.5f, pal.color(QPalette::ButtonText));
        lg.setColorAt(1.0f, Qt::transparent);
        painter->setPen(QPen(lg, 1.0f));
        painter->drawLine(textRect.x(), y, textRect.right(), y);
        painter->setPen(pen);
    }
    painter->setFont(savedFont);
    return true;
}
