#include <QPainter>
#include <QStyleOption>
#include <QDebug>
#include <QToolBar>
#include <QMainWindow>
#include <QStyleOptionGroupBox>
#include <QStyleOptionDockWidget>
#include <QStyleOptionDockWidgetV2>
#include <QCheckBox>
#include <QMenuBar>
#include <QMap>

#include "styleproject.h"
#include "stylelib/render.h"
#include "stylelib/ops.h"
#include "stylelib/color.h"
#include "stylelib/unohandler.h"
#include "overlay.h"

bool
StyleProject::drawStatusBar(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!widget || !widget->window() || !painter->isActive() || widget->palette().color(widget->backgroundRole()) != widget->window()->palette().color(QPalette::Window))
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
    static QMap<int, QPixmap> s_pix;
    if (!s_pix.contains(rect.height()))
    {
        QLinearGradient lg(rect.topLeft(), rect.bottomLeft());
        lg.setColorAt(0.0f, Color::titleBarColors[0]);
        lg.setColorAt(1.0f, Color::titleBarColors[1]);
        QPixmap p(Render::noise().width(), rect.height());
        p.fill(Qt::transparent);
        QPainter pt(&p);
        pt.fillRect(p.rect(), lg);
        pt.end();
        s_pix.insert(rect.height(), Render::mid(p, Render::noise(), 40, 1));
    }

    Render::renderMask(rect.sAdjusted(1, 1, -1, -1), painter, s_pix.value(rect.height()), 3, sides);
    Render::renderShadow(Render::Etched, rect, painter, 4, sides, m_s.shadows.opacity);
    return true;
}

bool
StyleProject::drawSplitter(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (widget && !qobject_cast<const QToolBar *>(widget->parentWidget()))
    if (option->rect.width() == 1 || option->rect.height() == 1)
        painter->fillRect(option->rect, QColor(0, 0, 0, m_s.shadows.opacity*255.0f));
    return true;
}

bool
StyleProject::drawToolBar(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (widget)
    if (castObj(const QMainWindow *, win, widget->parentWidget()))
    {
        int th(win->property("titleHeight").toInt());
        UNOHandler::drawUnoPart(painter, option->rect, win, th+widget->geometry().top());
    }
    return true;
}

bool
StyleProject::drawMenuBar(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (widget)
    if (castObj(const QMainWindow *, win, widget->parentWidget()))
    {
        int th(win->property("titleHeight").toInt());
        UNOHandler::drawUnoPart(painter, option->rect, win, th+widget->geometry().top());
    }
    return true;
}

bool
StyleProject::drawWindow(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!(widget && widget->isWindow()))
        return true;

    Render::renderShadow(Render::Sunken, option->rect, painter, 32, Render::Top);
    return true;
}

bool
StyleProject::drawMenu(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    QPalette::ColorRole /*fg(Ops::fgRole(widget, QPalette::Text)),*/ bg(Ops::bgRole(widget, QPalette::Base));
    Render::Sides sides = Render::All;
//    if (widget && qobject_cast<QMenuBar *>(widget->parentWidget()))
//        sides &= ~Render::Top;
    Render::renderMask(option->rect, painter, option->palette.color(bg), 4, sides);
    return true;
}

bool
StyleProject::drawGroupBox(const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    castOpt(GroupBox, opt, option);
    if (!opt)
        return true;

//    QRect frame(subControlRect(CC_GroupBox, opt, SC_GroupBoxFrame, widget)); //no need?
    QRect label(subControlRect(CC_GroupBox, opt, SC_GroupBoxLabel, widget));
    QRect check(subControlRect(CC_GroupBox, opt, SC_GroupBoxCheckBox, widget));
    QRect cont(subControlRect(CC_GroupBox, opt, SC_GroupBoxContents, widget));

    Render::renderShadow(Render::Sunken, cont, painter, 8, Render::All, 0.2f);
    if (opt->subControls & SC_GroupBoxCheckBox)
    {
        QStyleOptionButton btn;
        btn.QStyleOption::operator =(*option);
        btn.rect = check;
        drawCheckBox(&btn, painter, 0);
    }
    if (!opt->text.isEmpty())
    {
        painter->save();
        QFont f(painter->font());
        f.setBold(true);
        painter->setFont(f);
        drawItemText(painter, label, opt->textAlignment, opt->palette, opt->ENABLED, opt->text, widget?widget->foregroundRole():QPalette::WindowText);
        painter->restore();
    }
    return true;
}

bool
StyleProject::drawToolTip(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    QLinearGradient lg(option->rect.topLeft(), option->rect.bottomLeft());
    lg.setColorAt(0.0f, Color::mid(option->palette.color(QPalette::ToolTipBase), Qt::white, 5, 1));
    lg.setColorAt(1.0f, Color::mid(option->palette.color(QPalette::ToolTipBase), Qt::black, 5, 1));
    Render::renderMask(option->rect, painter, lg, 4);
    return true;
}

bool
StyleProject::drawDockTitle(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    castOpt(DockWidget, opt, option);
    if (!opt)
        return true;

    const QRect tr(subElementRect(SE_DockWidgetTitleBarText, opt, widget));
//    const QRect cr(subElementRect(SE_DockWidgetCloseButton, opt, widget));
//    const QRect fr(subElementRect(SE_DockWidgetFloatButton, opt, widget));
//    const QRect ir(subElementRect(SE_DockWidgetIcon, opt, widget));

    painter->save();
    painter->setOpacity(m_s.shadows.opacity);
    painter->setPen(opt->palette.color(Ops::fgRole(widget, QPalette::WindowText)));
    QRect l(tr);
    l.setLeft(0);
    l.setBottom(l.bottom()+1);
    painter->drawLine(l.bottomLeft(), l.bottomRight());
    painter->restore();

    const QFont f(painter->font());
    QFont bold(f);
    bold.setBold(true);
    const QString title(QFontMetrics(bold).elidedText(opt->title, Qt::ElideRight, tr.width()));
    painter->setFont(bold);
    drawItemText(painter, tr, Qt::AlignCenter, opt->palette, opt->ENABLED, title, widget?widget->foregroundRole():QPalette::WindowText);
    painter->setFont(f);
    return true;
}

bool
StyleProject::drawFrame(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!widget)
        return true;

    castOpt(FrameV3, opt, option);
    if (!opt)
        return true;

    castObj(const QFrame *, frame, widget);
    if (OverLay::hasOverLay(frame))
        return true;

    int roundNess(2);
    QRect r(option->rect);

    if ((frame && frame->frameShadow() == QFrame::Sunken) || (opt->state & State_Sunken))
        Render::renderShadow(Render::Sunken, r.adjusted(1, 1, -1, 0), painter, roundNess, Render::All, m_s.shadows.opacity);

    if (opt->state & State_Raised)
    {
        QPixmap pix(frame->rect().size());
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        Render::renderShadow(Render::Raised, pix.rect(), &p, 8);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        Render::renderMask(pix.rect().adjusted(2, 2, -2, -2), &p, Qt::black, 6);
        p.end();
        painter->drawTiledPixmap(frame->rect(), pix);
    }
    if (frame && frame->frameShadow() == QFrame::Plain)
        Render::renderShadow(Render::Etched, r, painter, 6, Render::All, 0.25f);
    return true;
}
