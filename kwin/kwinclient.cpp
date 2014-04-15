
#include <X11/Xatom.h>
#include "../stylelib/xhandler.h"

#include <kwindowinfo.h>
#include <QPainter>
#include <QHBoxLayout>

#include "kwinclient.h"
#include "button.h"
#include "../stylelib/ops.h"
#include "../stylelib/shadowhandler.h"

KwinClient::KwinClient(KDecorationBridge *bridge, Factory *factory)
    : KDecoration(bridge, factory)
    , m_titleLayout(0)
    , m_mainLayout(0)
    , m_isUno(false)
{
    setParent(factory);
}

void
KwinClient::init()
{
    createMainWidget();
    widget()->installEventFilter(this);
//    KWindowInfo info(windowId(), NET::WMWindowType, NET::WM2WindowClass);
    m_mainLayout = new QVBoxLayout(widget());
    m_mainLayout->setSpacing(0);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_titleLayout = new QHBoxLayout();
    m_mainLayout->addLayout(m_titleLayout);
    m_mainLayout->addStretch();
    widget()->setLayout(m_mainLayout);
    reset(63);
    ShadowHandler::installShadows(windowId());
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
        case '_': m_titleLayout->addSpacing(4); supported = false; break;
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
}

void
KwinClient::borders(int &left, int &right, int &top, int &bottom) const
{
    left = 0;
    right = 0;
    top = 16;
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
        p.setPen(options()->color(ColorFont, isActive()));
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
    default: break;
    }
    return KDecoration::eventFilter(o, e);
}

void
KwinClient::paint(QPainter &p)
{
    QRect r(0, 0, width(), 16);
    if (m_isUno)
        p.fillRect(r, m_unoGradient);
    QFont f(p.font());
    f.setBold(isActive());
    p.setFont(f);
    p.drawText(r, Qt::AlignCenter, caption());
}

QSize
KwinClient::minimumSize() const
{
    return QSize(256, 128);
}

void
KwinClient::activeChange()
{
    if (unsigned int *uno = XHandler::getXProperty<unsigned int>(windowId(), XHandler::MainWindow))
        m_isUno = *uno;
    if (unsigned int *bg = XHandler::getXProperty<unsigned int>(windowId(), XHandler::HeadColor))
    {
        m_unoColor = QColor(*bg);
        QRect r(0, 0, width(), 16);
        m_unoGradient = QLinearGradient(r.topLeft(), r.bottomLeft());
        m_unoGradient.setColorAt(0.0f, Ops::mid(m_unoColor, Qt::white));
        m_unoGradient.setColorAt(1.0f, m_unoColor);
    }
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
}
