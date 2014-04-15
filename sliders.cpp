#include <QScrollBar>
#include <QStyleOptionComplex>
#include <QStyleOptionSlider>
#include <QDebug>
#include <QPainter>
#include <QSlider>

#include "styleproject.h"
#include "render.h"
#include "stylelib/ops.h"

bool
StyleProject::drawScrollBar(const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
//    const QStyleOptionSlider *opt = qstyleoption_cast<const QStyleOptionSlider *>(option);
    if (!painter->isActive())
        return false;
    QPalette::ColorRole fg(QPalette::WindowText), bg(QPalette::Window);
    if (widget)
    {
        fg = widget->foregroundRole();
        bg = widget->backgroundRole();
    }
    QRect up(subControlRect(CC_ScrollBar, option, SC_ScrollBarSubLine, widget));
    QRect down(subControlRect(CC_ScrollBar, option, SC_ScrollBarAddLine, widget));
    QRect slider(subControlRect(CC_ScrollBar, option, SC_ScrollBarSlider, widget));
    QRect groove(subControlRect(CC_ScrollBar, option, SC_ScrollBarGroove, widget));
    const QColor mid(Ops::mid(option->palette.color(bg), option->palette.color(fg)));
    painter->fillRect(up, mid);
    painter->fillRect(down, mid);
    painter->fillRect(groove, option->palette.color(bg));
    painter->fillRect(slider, option->palette.color(fg));
    return true;
}

bool
StyleProject::drawSlider(const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    if (!painter->isActive())
        return false;
    QPalette::ColorRole fg(QPalette::WindowText), bg(QPalette::Window);
    if (widget)
    {
        fg = widget->foregroundRole();
        bg = widget->backgroundRole();
    }
    QRect slider(subControlRect(CC_Slider, option, SC_SliderHandle, widget));
    QRect groove(subControlRect(CC_Slider, option, SC_SliderGroove, widget));
    painter->fillRect(groove, Ops::mid(option->palette.color(bg), option->palette.color(fg)));
    painter->fillRect(slider, option->palette.color(fg));
    return true;
}
