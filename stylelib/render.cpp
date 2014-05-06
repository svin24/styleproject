#include <QDebug>
#include <qmath.h>
#include <QWidget>

#include "render.h"

#define OPACITY 0.75f

Render *Render::m_instance = 0;

/* blurring function below from:
 * http://stackoverflow.com/questions/3903223/qt4-how-to-blur-qpixmap-image
 * unclear to me who wrote it.
 */
QImage
Render::blurred(const QImage& image, const QRect& rect, int radius, bool alphaOnly)
{
    if (image.isNull())
        return QImage();
   int tab[] = { 14, 10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2 };
   int alpha = (radius < 1)  ? 16 : (radius > 17) ? 1 : tab[radius-1];

   QImage result = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
   int r1 = rect.top();
   int r2 = rect.bottom();
   int c1 = rect.left();
   int c2 = rect.right();

   int bpl = result.bytesPerLine();
   int rgba[4];
   unsigned char* p;

   int i1 = 0;
   int i2 = 3;

   if (alphaOnly)
       i1 = i2 = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

   for (int col = c1; col <= c2; col++) {
       p = result.scanLine(r1) + col * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p += bpl;
       for (int j = r1; j < r2; j++, p += bpl)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   for (int row = r1; row <= r2; row++) {
       p = result.scanLine(row) + c1 * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p += 4;
       for (int j = c1; j < c2; j++, p += 4)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   for (int col = c1; col <= c2; col++) {
       p = result.scanLine(r2) + col * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p -= bpl;
       for (int j = r1; j < r2; j++, p -= bpl)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   for (int row = r1; row <= r2; row++) {
       p = result.scanLine(row) + c2 * 4;
       for (int i = i1; i <= i2; i++)
           rgba[i] = p[i] << 4;

       p -= 4;
       for (int j = c1; j < c2; j++, p -= 4)
           for (int i = i1; i <= i2; i++)
               p[i] = (rgba[i] += ((p[i] << 4) - rgba[i]) * alpha / 16) >> 4;
   }

   return result;
}

Render
*Render::instance()
{
    if (!m_instance)
        m_instance = new Render();
    return m_instance;
}

Render::Render()
{
}

void
Render::_generateData()
{
    initMaskParts();
    initShadowParts();
    initTabs();
}

void
Render::initMaskParts()
{
    for (int i = 0; i < MAXRND+1; ++i)
    {
        const int size = i*2+1;
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(pix.rect(), Qt::black);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
        p.setBrush(Qt::black);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(pix.rect(), i, i);
        p.end();

        m_mask[i][TopLeftPart] = pix.copy(0, 0, i, i);
        //m_mask[TopMidPart] = pix.copy(RND, 0, SIZE-RND*2, RND);
        m_mask[i][TopRightPart] = pix.copy(size-i, 0, i, i);
        //m_mask[Left] = pix.copy(0, RND, RND, SIZE-RND*2);
        //m_mask[CenterPart] = pix.copy(RND, RND, SIZE-RND*2, SIZE-RND*2);
        //m_mask[Right] = pix.copy(SIZE-RND, RND, RND, SIZE-RND*2);
        m_mask[i][BottomLeftPart] = pix.copy(0, size-i, i, i);
        //m_mask[BottomMidPart] = pix.copy(RND, SIZE-RND, SIZE-RND*2, RND);
        m_mask[i][BottomRightPart] = pix.copy(size-i, size-i, i, i);
    }
}

void
Render::initShadowParts()
{
    for (int s = 0; s < ShadowCount; ++s)
    for (int r = 0; r < MAXRND+1; ++r)
    {
        if (!r)
            continue;
        const int size = r*2+1;
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
//        p.setOpacity(OPACITY);
        p.setRenderHint(QPainter::Antialiasing);
        float add(r>4?(float)r/100.0f:0);
        switch (s)
        {
        case Sunken:
        {
            QRect rect(pix.rect());
            p.setPen(Qt::NoPen);
            const int rnd(bool(r>2)?r:1);
            if (r > 1)
            {
                p.setBrush(QColor(255, 255, 255, 255));
                p.drawRoundedRect(rect, rnd, rnd);
            }
            p.setBrush(Qt::black);
            rect.setBottom(rect.bottom()-bool(r>1)*1);
            p.drawRoundedRect(rect, rnd, rnd);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            QRadialGradient rg(pix.rect().center()+QPointF(0, qMin(0.25f, add)), qFloor(size/2));
            rg.setColorAt(r>3?qMin(0.8f, 0.6f+add):0, Qt::black);
            rg.setColorAt(1.0f, Qt::transparent);
            p.translate(0.5f, 0.5f); //...and this is needed.... whyyy?
            p.fillRect(rect, rg);
            break;
        }
        case Raised:
        {
//            QRadialGradient rg(pix.rect().center()+QPointF(0, add), size/2.0f);
//            rg.setColorAt(0.6f, Qt::black);
//            rg.setColorAt(0.9f, Qt::transparent);
//            rg.setColorAt(1.0f, Qt::transparent);
//            p.translate(0.5f, 0.5f); //...and this is needed.... whyyy?
//            p.fillRect(pix.rect(), rg);
            p.setPen(Qt::NoPen);
            int n(6);
            int a(255/n);
            for (int i = 1; i <= n; ++i)
            {
                p.setBrush(QColor(0, 0, 0, a*i));
                int a(qMax(0, i-1));
                int rnd(r-a);
                p.drawRoundedRect(pix.rect().adjusted(a, a, -a, -a), rnd, rnd);
            }
            break;
        }
        case Etched:
        {
            QRect rect(pix.rect());
            p.setBrush(Qt::white);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(rect, r, r);
            p.setBrush(Qt::black);
            rect.setBottom(rect.bottom()-1);
            p.drawRoundedRect(rect, r, r);
            p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            rect.adjust(1, 1, -1, -1);
            p.drawRoundedRect(rect, r-1, r-1);
            break;
        }
        default: break;
        }
        p.end();
        splitShadowParts((Shadow)s, r, size, pix);
    }
}

void
Render::splitShadowParts(const Shadow shadow, int roundNess, const int size, const QPixmap &source)
{
    m_shadow[shadow][roundNess][TopLeftPart] = source.copy(0, 0, roundNess, roundNess);
    m_shadow[shadow][roundNess][TopMidPart] = source.copy(roundNess, 0, size-roundNess*2, roundNess);
    m_shadow[shadow][roundNess][TopRightPart] = source.copy(size-roundNess, 0, roundNess, roundNess);
    m_shadow[shadow][roundNess][LeftPart] = source.copy(0, roundNess, roundNess, size-roundNess*2);
    m_shadow[shadow][roundNess][CenterPart] = source.copy(roundNess, roundNess, size-roundNess*2, size-roundNess*2);
    m_shadow[shadow][roundNess][RightPart] = source.copy(size-roundNess, roundNess, roundNess, size-roundNess*2);
    m_shadow[shadow][roundNess][BottomLeftPart] = source.copy(0, size-roundNess, roundNess, roundNess);
    m_shadow[shadow][roundNess][BottomMidPart] = source.copy(roundNess, size-roundNess, size-roundNess*2, roundNess);
    m_shadow[shadow][roundNess][BottomRightPart] = source.copy(size-roundNess, size-roundNess, roundNess, roundNess);
}

static int tr(4); //tab roundness
static int ts(4); //tab shadow size

static QPainterPath tab(const QRect &r, int d)
{
    int x1, y1, x2, y2;
    r.getCoords(&x1, &y1, &x2, &y2);
    QPainterPath path;
    path.moveTo(x2, y1);
    path.quadTo(x2-d, y1, x2-d, y1+d);
    path.lineTo(x2-d, y2-d);
    path.quadTo(x2-d, y2, x2-d*2, y2);
    path.lineTo(x1+d*2, y2);
    path.quadTo(x1+d, y2, x1+d, y2-d);
    path.lineTo(x1+d, y1+d);
    path.quadTo(x1+d, y1, x1, y1);
    path.lineTo(x2, y1);
    path.closeSubpath();
    return path;
}

void
Render::initTabs()
{
    for (int i = 0; i < AfterSelected+1; ++i)
    {
        int hsz = (tr*4)+(ts*4);
        int vsz = hsz/2;
        ++hsz;
        ++vsz;
        QImage img(hsz, vsz, QImage::Format_ARGB32);
        img.fill(Qt::transparent);
        const QRect r(img.rect());
        QPainterPath path(tab(r.adjusted(ts, 0, -ts, 0), tr));
//        switch(i)
//        {
//        case 0: //left of selected
//        {
//            path.moveTo(x1, y1);
//            path.quadTo(x1+d, y1, x1+d, y1+d);
//            path.lineTo(x1+d, y2-d);
//            path.quadTo(x1+d, y2, x1+d*2, y2);
//            path.lineTo(x2, y2);
//        }
//        case 1:  //selected
//        {

//        }
//        case 2:  //right of selected
//        {
//            path.moveTo(x2, y1);
//            path.quadTo(x2-d, y1, x2-d, y1+d);
//            path.lineTo(x2-d, y2-d);
//            path.quadTo(x2-d, y2, x2-d*2, y2);
//            path.lineTo(x1, y2);
//        }
//        }
        QPainter p(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.drawPath(path);
        p.end();
        img = blurred(img, img.rect(), ts);
        p.begin(&img);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.setCompositionMode(QPainter::CompositionMode_DestinationOut); //erase the tabbar shadow... otherwise double.
        p.drawPath(path);
        renderShadow(Sunken, img.rect(), &p, 16, Top|Bottom);
        p.end();
        --hsz-1;
        hsz/=2;
        --vsz;
        vsz/=2;
        m_tab[i][TopLeftPart] = QPixmap::fromImage(img.copy(0, 0, hsz, vsz));
        m_tab[i][TopMidPart] = QPixmap::fromImage(img.copy(hsz, 0, 1, vsz));
        m_tab[i][TopRightPart] = QPixmap::fromImage(img.copy(hsz+1, 0, hsz, vsz));
        m_tab[i][LeftPart] = QPixmap::fromImage(img.copy(0, vsz, hsz, 1));
        m_tab[i][CenterPart] = QPixmap::fromImage(img.copy(hsz, vsz, 1, 1));
        m_tab[i][RightPart] = QPixmap::fromImage(img.copy(hsz+1, vsz, hsz, 1));
        m_tab[i][BottomLeftPart] = QPixmap::fromImage(img.copy(0, vsz+1, hsz, vsz));
        m_tab[i][BottomMidPart] = QPixmap::fromImage(img.copy(hsz, vsz+1, 1, vsz));
        m_tab[i][BottomRightPart] = QPixmap::fromImage(img.copy(hsz+1, vsz+1, hsz, vsz));
    }
}

void
Render::_renderTab(const QRect &r, QPainter *p, const Tab t, QPainterPath *path)
{
    const QSize sz(m_tab[t][TopLeftPart].size());
    if (r.width()*2+1 < sz.width()*2+1)
        return;
    int x1, y1, x2, y2;
    r.getCoords(&x1, &y1, &x2, &y2);
//    x1-=tr;
//    x2+=tr;
    int halfH(r.width()-(sz.width()*2)), halfV(r.height()-(sz.height()*2));

    if (t < AfterSelected)
    {
        p->drawTiledPixmap(QRect(x1, y1, sz.width(), sz.height()), m_tab[t][TopLeftPart]);
        p->drawTiledPixmap(QRect(x1, y1+sz.height(), sz.width(), halfV), m_tab[t][LeftPart]);
        p->drawTiledPixmap(QRect(x1, y1+sz.height()+halfV, sz.width(), sz.height()), m_tab[t][BottomLeftPart]);
    }

    p->drawTiledPixmap(QRect(x1+sz.width(), y1, halfH, sz.height()), m_tab[t][TopMidPart]);
    p->drawTiledPixmap(QRect(x1+sz.width(), y1+sz.height(), halfH, halfV), m_tab[t][CenterPart]);
    p->drawTiledPixmap(QRect(x1+sz.width(), y1+sz.height()+halfV, halfH, sz.height()), m_tab[t][BottomMidPart]);

    if (t > BeforeSelected)
    {
        p->drawTiledPixmap(QRect(x1+sz.width()+halfH, y1, sz.width(), sz.height()), m_tab[t][TopRightPart]);
        p->drawTiledPixmap(QRect(x1+sz.width()+halfH, y1+sz.height(), sz.width(), halfV), m_tab[t][RightPart]);
        p->drawTiledPixmap(QRect(x1+sz.width()+halfH, y1+sz.height()+halfV, sz.width(), sz.height()), m_tab[t][BottomRightPart]);
    }
    if (path)
        *path = tab(r.adjusted(tr, 0, -tr, 0), tr);
}

QRect
Render::partRect(const QRect &rect, const Part part, const int roundNess, const Sides sides) const
{
    int x, y, w, h;
    rect.getRect(&x, &y, &w, &h);

    int midWidth = w;
    if (sides&Right)
        midWidth-=roundNess;
    if (sides&Left)
        midWidth-=roundNess;

    int midHeight = h;
    if (sides&Top)
        midHeight-=roundNess;
    if (sides&Bottom)
        midHeight-=roundNess;

    int left(bool(sides&Left)*roundNess)
            , top(bool(sides&Top)*roundNess)
            , right(bool(sides&Right)*roundNess)
            , bottom(bool(sides&Bottom)*roundNess);

    switch (part)
    {
    case TopLeftPart: return QRect(0, 0, roundNess, roundNess);
    case TopMidPart: return QRect(left, 0, midWidth, roundNess);
    case TopRightPart: return QRect(w-right, 0, roundNess, roundNess);
    case LeftPart: return QRect(0, top, roundNess, midHeight);
    case CenterPart: return QRect(left, top, midWidth, midHeight);
    case RightPart: return QRect(w-right, top, roundNess, midHeight);
    case BottomLeftPart: return QRect(0, h-bottom, roundNess, roundNess);
    case BottomMidPart: return QRect(left, h-bottom, midWidth, roundNess);
    case BottomRightPart: return QRect(w-right, h-bottom, roundNess, roundNess);
    default: return QRect();
    }
}

QRect
Render::rect(const QRect &rect, const Part part, const int roundNess) const
{
    int x, y, w, h;
    rect.getRect(&x, &y, &w, &h);
    int midWidth = w-(roundNess*2);
    int midHeight = h-(roundNess*2);
    switch (part)
    {
    case TopLeftPart: return QRect(0, 0, roundNess, roundNess);
    case TopMidPart: return QRect(roundNess, 0, midWidth, roundNess);
    case TopRightPart: return QRect(w-roundNess, 0, roundNess, roundNess);
    case LeftPart: return QRect(0, roundNess, roundNess, midHeight);
    case CenterPart: return QRect(roundNess, roundNess, midWidth, midHeight);
    case RightPart: return QRect(w-roundNess, roundNess, roundNess, midHeight);
    case BottomLeftPart: return QRect(0, h-roundNess, roundNess, roundNess);
    case BottomMidPart: return QRect(roundNess, h-roundNess, midWidth, roundNess);
    case BottomRightPart: return QRect(w-roundNess, h-roundNess, roundNess, roundNess);
    default: return QRect();
    }
}

bool
Render::isCornerPart(const Part part) const
{
    return part==TopLeftPart||part==TopRightPart||part==BottomLeftPart||part==BottomRightPart;
}

QPixmap
Render::genPart(const Part part, const QPixmap &source, const int roundNess, const Sides sides) const
{
    QPixmap rt = source.copy(partRect(source.rect(), part, roundNess, sides));
    if (!isCornerPart(part))
        return rt;
    QPainter p(&rt);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.drawTiledPixmap(rt.rect(), m_mask[roundNess][part]);
    p.end();
    return rt;
}

bool
Render::needPart(const Part part, const Sides sides) const
{
    switch (part)
    {
    case TopLeftPart: return bool((sides&Top)&&(sides&Left));
    case TopMidPart: return bool(sides&Top);
    case TopRightPart: return bool((sides&Top)&&(sides&Right));
    case LeftPart: return bool(sides&Left);
    case CenterPart: return true;
    case RightPart: return bool(sides&Right);
    case BottomLeftPart: return bool((sides&Bottom)&&(sides&Left));
    case BottomMidPart: return bool(sides&Bottom);
    case BottomRightPart: return bool((sides&Bottom)&&(sides&Right));
    default: return false;
    }
}

void
Render::_renderMask(const QRect &rect, QPainter *painter, const QBrush &brush, int roundNess, const Sides sides)
{
    if (!rect.isValid())
        return;
    roundNess = qMin(qMin(MAXRND, roundNess), qMin(rect.height(), rect.width())/2);
    if (!roundNess)
        roundNess = 1;
    QPixmap pix(rect.size());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    if (!p.isActive())
        return;
    p.fillRect(pix.rect(), brush);
    p.end();

    for (int i = 0; i < PartCount; ++i)
        if (needPart((Part)i, sides))
        {
            if (i != CenterPart && !roundNess)
                continue;
            const QPixmap &partPix = genPart((Part)i, pix, roundNess, sides);
            painter->drawPixmap(partRect(QRect(QPoint(0, 0), rect.size()), (Part)i, roundNess, sides).translated(rect.x(), rect.y()), partPix);
        }
}

void
Render::_renderShadow(const Shadow shadow, const QRect &rect, QPainter *painter, int roundNess, const float opacity, const Sides sides)
{
    if (!rect.isValid())
        return;
    roundNess = qMin(qMin(MAXRND, roundNess), qMin(rect.height(), rect.width())/2);
    if (!roundNess)
        roundNess = 1;
    const float o(painter->opacity());
    painter->setOpacity(opacity);
    for (int i = 0; i < PartCount; ++i)
        if (i == CenterPart || roundNess)
            if (needPart((Part)i, sides))
                painter->drawTiledPixmap(partRect(QRect(QPoint(0, 0), rect.size()), (Part)i, roundNess, sides).translated(rect.x(), rect.y()), m_shadow[shadow][roundNess][i]);
    painter->setOpacity(o);
}

Render::Sides
Render::checkedForWindowEdges(const QWidget *w, Sides from)
{
    if (!w)
        return from;
    QPoint topLeft = w->mapTo(w->window(), w->rect().topLeft());
    QRect winRect = w->window()->rect();
    QRect widgetRect = QRect(topLeft, w->size());

    if (widgetRect.left() <= winRect.left())
        from &= ~Render::Left;
    if (widgetRect.right() >= winRect.right())
        from &= ~Render::Right;
    if (widgetRect.bottom() >= winRect.bottom())
        from &= ~Render::Bottom;
    if (widgetRect.top() <= winRect.top())
        from &= ~Render::Top;
    return from;
}

void
Render::colorizePixmap(QPixmap &pix, const QBrush &b)
{
    QImage tmp(pix.size(), QImage::Format_ARGB32);
    tmp.fill(Qt::transparent);
    QPainter p(&tmp);
    p.fillRect(tmp.rect(), b);
    p.end();

    QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
    QRgb *tmpColors = reinterpret_cast<QRgb *>(tmp.bits());
    QRgb *pixColors = reinterpret_cast<QRgb *>(img.bits());
    const int s(pix.height()*pix.width());
    for (int i = 0; i < s; ++i)
    {
        QColor c(tmpColors[i]);
        c.setAlpha(qMin(qAlpha(pixColors[i]), qAlpha(tmpColors[i])));
        pixColors[i] = c.rgba();
    }
    pix = QPixmap::fromImage(img);
}

QPixmap
Render::colorized(QPixmap pix, const QBrush &b)
{
    colorizePixmap(pix, b);
    return pix;
}
