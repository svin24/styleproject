
#include <X11/Xatom.h>
#include "../stylelib/xhandler.h"

#include <kwindowinfo.h>
#include <QPainter>
#include <QHBoxLayout>
#include <QTimer>
#include <QPixmap>

#include "kwinclient.h"
#include "button.h"
#include "../stylelib/ops.h"
#include "../stylelib/shadowhandler.h"

#define TITLEHEIGHT 22

TitleBar::TitleBar(KwinClient *client, QWidget *parent)
    : QWidget(parent)
    , m_brush(QBrush())
    , m_client(client)
{
    setFixedHeight(TITLEHEIGHT);
    setAttribute(Qt::WA_NoSystemBackground);
//    setAttribute(Qt::WA_TranslucentBackground);
}

void
TitleBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), m_brush);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    const int rnd(8);
    p.drawRoundedRect(rect().adjusted(0, 0, 0, rnd), rnd, rnd);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QFont f(p.font());
    f.setBold(m_client->isActive());
    p.setFont(f);
    if (m_client->isActive())
    {
        p.setPen(QColor(255, 255, 255, 127));
        p.drawText(rect().translated(0, 1), Qt::AlignCenter, m_client->caption());
    }
    p.setPen(m_client->options()->color(KwinClient::ColorFont, m_client->isActive()));
    p.drawText(rect(), Qt::AlignCenter, m_client->caption());
    p.end();
}

void
TitleBar::setBrush(const QBrush &brush)
{
    m_brush = brush;
}

///-------------------------------------------------------------------

KwinClient::KwinClient(KDecorationBridge *bridge, Factory *factory)
    : KDecoration(bridge, factory)
    , m_titleLayout(0)
    , m_titleBar(0)
    , m_isUno(false)
{
    setParent(factory);
}

void
KwinClient::init()
{
    createMainWidget();
    widget()->installEventFilter(this);
    widget()->setAttribute(Qt::WA_NoSystemBackground);
    widget()->setAutoFillBackground(false);
    m_titleBar = new TitleBar(this);
    m_titleLayout = new QHBoxLayout();
    m_titleLayout->setSpacing(0);
    m_titleLayout->setContentsMargins(0, 0, 0, 0);
    m_titleBar->setLayout(m_titleLayout);
    QVBoxLayout *l = new QVBoxLayout(widget());
    l->setSpacing(0);
    l->setContentsMargins(0, 0, 0, 0);
//    l->addLayout(m_titleLayout);
    l->addWidget(m_titleBar);
    l->addStretch();
    widget()->setLayout(l);

    ShadowHandler::installShadows(windowId());
    QTimer::singleShot(1, this, SLOT(postInit()));
    setAlphaEnabled(true);
}

void
KwinClient::postInit()
{
    reset(63);
}

void
KwinClient::populate(const QString &buttons)
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        Button::Type t;
        bool supported(true);
        switch (buttons.at(i).toAscii())
        {
        /*
        * @li 'N' application menu button
        * @li 'M' window menu button
        * @li 'S' on_all_desktops button
        * @li 'H' quickhelp button
        * @li 'I' minimize ( iconify ) button
        * @li 'A' maximize button
        * @li 'X' close button
        * @li 'F' keep_above_others button
        * @li 'B' keep_below_others button
        * @li 'L' shade button
        * @li 'R' resize button
        * @li '_' spacer
        */
        case 'X': t = Button::Close; break;
        case 'I': t = Button::Min; break;
        case 'A': t = Button::Max; break;
        case '_': m_titleLayout->addSpacing(2); supported = false; break;
        default: supported = false; break;
        }
        if (supported)
            m_titleLayout->addWidget(new Button(t, this));
    }
}

void
KwinClient::resize(const QSize &s)
{
    widget()->resize(s);
//    m_stretch->setFixedHeight(widget()->height()-TITLEHEIGHT);
}

void
KwinClient::borders(int &left, int &right, int &top, int &bottom) const
{
    left = 0;
    right = 0;
    top = TITLEHEIGHT;
    bottom = 0;
}

void
KwinClient::captionChange()
{
    widget()->update();
}

bool
KwinClient::eventFilter(QObject *o, QEvent *e)
{
    if (o != widget())
        return false;
    switch (e->type())
    {
    case QEvent::Paint:
    {
        QPainter p(widget());
        p.setRenderHint(QPainter::Antialiasing);
        paint(p);
        p.end();
        return true;
    }
    case QEvent::MouseButtonDblClick:
        titlebarDblClickOperation();
        return true;
    case QEvent::MouseButtonPress:
        processMousePressEvent(static_cast<QMouseEvent *>(e));
        return true;
    case QEvent::Wheel:
        titlebarMouseWheelOperation(static_cast<QWheelEvent *>(e)->delta());
        return true;
//    case QEvent::Show:
//    case QEvent::Resize:
//        m_titleLayout->setGeometry(QRect(0, 0, width(), TITLEHEIGHT));
//        m_stretch->setFixedHeight(widget()->height()-TITLEHEIGHT);
    default: break;
    }
    return KDecoration::eventFilter(o, e);
}

void
KwinClient::paint(QPainter &p)
{
//    p.fillRect(m_titleLayout->geometry(), Qt::white);
//    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
//    p.setPen(Qt::NoPen);
//    p.setBrush(Qt::black);
//    p.drawRoundedRect(m_titleLayout->geometry(), 8, 8);
//    QFont f(p.font());
//    f.setBold(isActive());
//    p.setFont(f);
//    p.setPen(options()->color(ColorFont, isActive()));
//    p.drawText(m_titleBar->geometry(), Qt::AlignCenter, caption());
}

QSize
KwinClient::minimumSize() const
{
    return QSize(256, 128);
}

void
KwinClient::activeChange()
{
    ShadowHandler::installShadows(windowId());
    widget()->update();
}

/**
 * These flags specify which settings changed when rereading settings.
 * Each setting in class KDecorationOptions specifies its matching flag.
 */
/**
    SettingDecoration  = 1 << 0, ///< The decoration was changed
    SettingColors      = 1 << 1, ///< The color palette was changed
    SettingFont        = 1 << 2, ///< The titlebar font was changed
    SettingButtons     = 1 << 3, ///< The button layout was changed
    SettingTooltips    = 1 << 4, ///< The tooltip setting was changed
    SettingBorder      = 1 << 5, ///< The border size setting was changed
    SettingCompositing = 1 << 6  ///< Compositing settings was changed
};*/

void
KwinClient::reset(unsigned long changed)
{
    if (changed & SettingButtons)
    {
        while (QLayoutItem *item = m_titleLayout->takeAt(0))
        {
            if (item->widget())
                delete item->widget();
            else if (item->spacerItem())
                delete item->spacerItem();
            else
                delete item;
        }
        populate(options()->titleButtonsLeft());
        m_titleLayout->addStretch();
        populate(options()->titleButtonsRight());
    }

    if (unsigned int *uno = XHandler::getXProperty<unsigned int>(windowId(), XHandler::MainWindow))
        m_isUno = *uno;
    if (unsigned int *bg = XHandler::getXProperty<unsigned int>(windowId(), XHandler::HeadColor))
    {
        m_unoColor = QColor(*bg);
        QRect r(m_titleBar->rect());
        m_unoGradient = QLinearGradient(r.topLeft(), r.bottomLeft());
        m_unoGradient.setColorAt(0.0f, Ops::mid(m_unoColor, Qt::white));
        m_unoGradient.setColorAt(1.0f, m_unoColor);
        m_titleBar->setBrush(m_unoGradient);
        m_titleBar->update();
    }
    ShadowHandler::installShadows(windowId());
    widget()->update();
}