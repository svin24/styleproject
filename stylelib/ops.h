#ifndef OPS_H
#define OPS_H

#include <QColor>
#include <QWidget>

/**
 * This class is a mess atm, I just add stuff here
 * that I need outta the way... most of this stuff
 * need to be properly categorised in some classes
 * that actually makes sense, for now, deal w/ it.
 */

class ToolButtonData
{
public:
    bool prevSelected, nextSelected, isInTopToolBar;
    unsigned int sides;
};

class QTabBar;
class QToolButton;
class QStyle;
class QStyleOption;
class Q_DECL_EXPORT Ops : public QObject
{
    Q_OBJECT
public:
    enum Dir{ Left, Up, Right, Down };
    typedef unsigned int Direction;
    static Ops *instance();

    static QWidget *window(QWidget *w);
    static bool isSafariTabBar(const QTabBar *tabBar);
    static void updateWindow(WId window);
    static void drawCheckMark(QPainter *p, const QColor &c, const QRect &r, const bool tristate = false);
    static void drawArrow(QPainter *p, const QColor &c, const QRect &r, const Direction d, const Qt::Alignment align = Qt::AlignCenter, int size = 0);
    static QPalette::ColorRole opposingRole(const QPalette::ColorRole &role);
    static void fixWindowTitleBar(QWidget *win);
    static QPalette::ColorRole bgRole(const QWidget *w, const QPalette::ColorRole fallBack = QPalette::Window);
    static QPalette::ColorRole fgRole(const QWidget *w, const QPalette::ColorRole fallBack = QPalette::WindowText);
    static ToolButtonData toolButtonData(const QToolButton *tbtn, const QStyle *s, bool &ok, const QStyleOption *opt = 0);

    template<typename T> static inline bool isOrInsideA(QWidget *widget)
    {
        if (!widget)
            return false;
        QWidget *w = widget;
        while (w->parentWidget())
        {
            if (qobject_cast<T>(w))
                return true;
            w = w->parentWidget();
        }
        return false;
    }
public slots:
    void updateGeoFromSender();

private:
    static Ops *s_instance;
};

#endif // OPS_H
