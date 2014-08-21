#ifndef ANIMHANDLER_H
#define ANIMHANDLER_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QTabBar>
#include <QStyle>
#include <QToolButton>

namespace Anim
{

#define STEPS 16

Q_DECL_EXPORT void setStyle(QStyle *style);

class Q_DECL_EXPORT Basic : public QObject
{
    Q_OBJECT
public:
    explicit Basic(QObject *parent = 0);
    static Basic *instance();
    static void manage(QWidget *w);
    static void release(QWidget *w);
    static void deleteInstance();
    static int level(const QWidget *widget) { return instance()->hoverLevel(widget); }

protected:
    bool eventFilter(QObject *, QEvent *);
    int hoverLevel(const QWidget *widget);
    void remove(QWidget *w);

protected slots:
    void removeSender();
    void animate();

private:
    static Basic *s_instance;
    QMap<QWidget *, int> m_vals;
    QTimer *m_timer;
};

class Q_DECL_EXPORT Tabs : public QObject
{
    Q_OBJECT
public:
    typedef int Tab, Level; //for readability only...
    explicit Tabs(QObject *parent = 0);
    static Tabs *instance();
    static void manage(QTabBar *tb);
    static void release(QTabBar *tb);
    static void deleteInstance();
    static int level(const QTabBar *tb, Tab tab) { return instance()->hoverLevel(tb, tab); }

protected:
    bool eventFilter(QObject *, QEvent *);
    int hoverLevel(const QTabBar *tb, Tab tab);
    void remove(QTabBar *tb);

protected slots:
    void removeSender();
    void animate();

private:
    static Tabs *s_instance;
    QMap<QTabBar *, QMap<Tab, Level> > m_vals;
    QTimer *m_timer;
};

class Q_DECL_EXPORT ToolBtns : public QObject
{
    Q_OBJECT
public:
    typedef int Level, ArrowLevel; //for readability only...
    explicit ToolBtns(QObject *parent = 0);
    static ToolBtns *instance();
    static void manage(QToolButton *tb);
    static void release(QToolButton *tb);
    static void deleteInstance();
    static int level(const QToolButton *tb, bool arrow) { return instance()->hoverLevel(tb, arrow); }

protected:
    bool eventFilter(QObject *, QEvent *);
    int hoverLevel(const QToolButton *tb, bool arrow);
    void remove(QToolButton *tb);

protected slots:
    void removeSender();
    void animate();

private:
    static ToolBtns *s_instance;
    QMap<QToolButton *, QPair<Level, ArrowLevel> > m_vals;
    QTimer *m_timer;
};

}

#endif //ANIMHANDLER_H
