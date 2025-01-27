#ifndef PROGRESSHANDLER_H
#define PROGRESSHANDLER_H

#include <QObject>
#include <QMap>

class QProgressBar;
class QTimer;
namespace DSP
{
struct TimerData
{
    bool goingBack;
    int busyValue, timerId;
};

class Q_DECL_EXPORT ProgressHandler : public QObject
{
    Q_OBJECT
public:
    explicit ProgressHandler(QObject *parent = 0);
    static ProgressHandler *instance();
    static void manage(QProgressBar *bar);
    static void release(QProgressBar *bar);

protected slots:
    void valueChanged();

protected:
    void checkBar(QProgressBar *bar);
    void initBar(QProgressBar *bar);
    bool eventFilter(QObject *, QEvent *);

private:
    static ProgressHandler *s_instance;
    QList<QProgressBar *> m_bars;
    QMap<QProgressBar *, TimerData *> m_data;
};
} //namespace
#endif // PROGRESSHANDLER_H
