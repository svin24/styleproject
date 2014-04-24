#include <QStylePlugin>
#include <QWidget>
#include <QStyleOptionComplex>
#include <QStyleOptionSlider>
#include <QStyleOptionComboBox>
#include <QDebug>
#include <qmath.h>
#include <QEvent>
#include <QToolBar>
#include <QAbstractItemView>
#include <QMainWindow>
#include <QTimer>
#include <QApplication>
#include <QLayout>

#include "styleproject.h"
#include "stylelib/render.h"
#include "stylelib/ops.h"
#include "stylelib/xhandler.h"
#include "stylelib/color.h"

class ProjectStylePlugin : public QStylePlugin
{
public:
    QStringList keys() const { return QStringList() << "StyleProject"; }
    QStyle *create(const QString &key)
    {
        if (key == "styleproject")
            return new StyleProject;
        return 0;
    }
};

Q_EXPORT_PLUGIN2(StyleProject, ProjectStylePlugin)

StyleProject::StyleProject()
    : QCommonStyle()
{
    init();
    assignMethods();
    Render::generateData();
    QPalette p(QApplication::palette());
    QColor c = Color::mid(p.color(QPalette::Window), p.color(QPalette::WindowText), 4, 1);
    Color::titleBarColors[0] = Color::mid(c, Qt::white, 1, 2);
    Color::titleBarColors[1] = Color::mid(c, Qt::black, 11, 1);
}

void
StyleProject::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!(m_ce[element] && (this->*m_ce[element])(option, painter, widget)))
        QCommonStyle::drawControl(element, option, painter, widget);
}

void
StyleProject::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    if (!(m_cc[control] && (this->*m_cc[control])(option, painter, widget)))
        QCommonStyle::drawComplexControl(control, option, painter, widget);
}

void
StyleProject::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (!(m_pe[element] && (this->*m_pe[element])(option, painter, widget)))
        QCommonStyle::drawPrimitive(element, option, painter, widget);
}

void
StyleProject::drawItemText(QPainter *painter, const QRect &rect, int flags, const QPalette &pal, bool enabled, const QString &text, QPalette::ColorRole textRole) const
{
    if (text.isEmpty())
        return;
    painter->save();
    // we need to add either hide/show mnemonic, otherwise
    // we are rendering text w/ '&' characters.
    flags |= Qt::TextShowMnemonic; // Qt::TextHideMnemonic
    if (textRole != QPalette::NoRole)
    {
        const QPalette::ColorRole bgRole(Ops::opposingRole(textRole));
        if (bgRole != QPalette::NoRole && enabled)
        {
            const bool isDark(Color::luminosity(pal.color(textRole)) > Color::luminosity(pal.color(bgRole)));
            const int rgb(isDark?0:255);
            const QColor bevel(rgb, rgb, rgb, 127);
            painter->setPen(bevel);
            painter->drawText(rect.translated(0, 1), flags, text);
        }
        painter->setPen(pal.color(enabled ? QPalette::Active : QPalette::Disabled, textRole));
    }

    painter->drawText(rect, flags, text);
//    QCommonStyle::drawItemText(painter, rect, flags, pal, enabled, text, textRole);
    painter->restore();
}

void
StyleProject::drawItemPixmap(QPainter *painter, const QRect &rect, int alignment, const QPixmap &pixmap) const
{
    QCommonStyle::drawItemPixmap(painter, rect, alignment, pixmap);
}

QPixmap
StyleProject::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap, const QStyleOption *opt) const
{
    return QCommonStyle::generatedIconPixmap(iconMode, pixmap, opt);
}

QRect
StyleProject::itemPixmapRect(const QRect &r, int flags, const QPixmap &pixmap) const
{
    if (flags == Qt::AlignCenter)
        flags = Qt::AlignLeft|Qt::AlignVCenter;
    QRect ret(pixmap.rect());
    if (flags & (Qt::AlignHCenter|Qt::AlignVCenter))
        ret.moveCenter(r.center());
    if (flags & Qt::AlignLeft)
        ret.moveLeft(r.left());
    if (flags & Qt::AlignRight)
        ret.moveRight(r.right());
    if (flags & Qt::AlignTop)
        ret.moveTop(r.top());
    if (flags & Qt::AlignBottom)
        ret.moveBottom(r.bottom());
    return ret;
}

void
StyleProject::fixTitleLater(QWidget *win)
{
    QTimer *t = new QTimer(win);
    connect(t, SIGNAL(timeout()), this, SLOT(fixTitle()));
    t->start(1000);
}

void
StyleProject::fixTitle()
{
    if (castObj(QTimer *, t, sender()))
    {
        if (castObj(QWidget *, w, t->parent()))
            Ops::fixWindowTitleBar(w);
        t->stop();
        t->deleteLater();
    }
}

void
StyleProject::updateToolBar(QToolBar *toolBar)
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
            toolBar->setMovable(true);
            toolBar->layout()->setContentsMargins(0, 0, 0, 0);
            toolBar->setContentsMargins(0, 0, pixelMetric(PM_ToolBarHandleExtent), 6);
        }
        else
            toolBar->layout()->setContentsMargins(2, 2, 2, 2);
    }
}

void
StyleProject::fixMainWindowToolbar()
{
    updateToolBar(static_cast<QToolBar *>(sender()));
}

