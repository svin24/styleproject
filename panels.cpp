#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QToolBar>
#include <QMainWindow>

#include "styleproject.h"
#include "stylelib/render.h"
#include "stylelib/ops.h"
#include "stylelib/color.h"

bool
StyleProject::drawStatusBar(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!widget || !widget->window() || !painter->isActive())
        return true;

    Render::Sides sides = Render::All;
    QPoint topLeft = widget->mapTo(widget->window(), widget->rect().topLeft());
    QRect winRect = widget->window()->rect();
    QRect widgetRect = QRect(topLeft, widget->size());

    if (widgetRect.left() <= winRect.left())
        sides &= ~Render::Left;
    if (widgetRect.right() >= winRect.right())
        sides &= ~Render::Right;
    if (widgetRect.bottom() >= winRect.bottom())
        sides &= ~Render::Bottom;
    //needless to check for top I think ;-)

    const QRect &rect(widget->rect());
    QLinearGradient lg(rect.topLeft(), rect.bottomLeft());
    lg.setColorAt(0.0f, Color::titleBarColors[0]);
    lg.setColorAt(1.0f, Color::titleBarColors[1]);

    Render::renderMask(rect.sAdjusted(1, 1, -1, -1), painter, lg, 3, sides);
    const int o(painter->opacity());
    painter->setOpacity(0.5f);
    Render::renderShadow(Render::Etched, rect, painter, 4, sides);
    painter->setOpacity(o);
    return true;
}

bool
StyleProject::drawSplitter(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!painter->isActive())
        return false;
    painter->fillRect(option->rect, QColor(0, 0, 0, 128));
    return true;
}

bool
StyleProject::drawToolBar(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!widget || !widget->parentWidget() || !widget->window() || !painter->isActive())
        return true;

    painter->save();
    uint sides = Render::All;
    QPoint topLeft = widget->mapTo(widget->window(), widget->rect().topLeft());
    QRect winRect = widget->window()->rect();
    QRect widgetRect = QRect(topLeft, widget->size());

    if (widgetRect.left() <= winRect.left())
        sides &= ~Render::Left;
    if (widgetRect.right() >= winRect.right())
        sides &= ~Render::Right;
    if (widgetRect.top() <= winRect.top())
        sides &= ~Render::Top;

    QRect rect(widget->rect());
    if (castObj(const QMainWindow *, win, widget->parentWidget()))
    {
        QBrush b(Color::titleBarColors[1]);
        sides = 0;
        if (widget->geometry().top() <= win->rect().top())
        {
            int th(win->property("titleHeight").toInt());
            rect.setTop(rect.top()-th);
            painter->translate(0, th);
            QLinearGradient lg(rect.topLeft(), rect.bottomLeft());
            lg.setColorAt(0.0f, Color::titleBarColors[0]);
            lg.setColorAt(1.0f, Color::titleBarColors[1]);
            b = lg;
        }
        castObj(const QToolBar *, toolBar, widget);
        if (win->toolBarArea(const_cast<QToolBar *>(toolBar)) == Qt::TopToolBarArea)
            Render::renderMask(rect, painter, b, 3, sides);
    }

    if (sides & Render::Top)
    {
        painter->setOpacity(0.5f);
        Render::renderShadow(Render::Etched, rect, painter, 4, sides);
    }
    painter->restore();
    return true;
}

bool
StyleProject::drawWindow(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!(widget && widget->isWindow()) || !painter->isActive())
        return true;

    Render::renderShadow(Render::Sunken, option->rect, painter, 32, Render::Top);
    return true;
}

bool
StyleProject::drawMenu(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    QPalette::ColorRole fg(QPalette::Text), bg(QPalette::Base);
    if (widget)
    {
        fg = widget->foregroundRole();
        bg = widget->backgroundRole();
    }
    Render::renderMask(option->rect, painter, option->palette.color(bg), 6);
    return true;
}
