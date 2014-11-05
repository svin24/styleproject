#include <QDebug>
#include <qmath.h>
#include <QWidget>
#include <QToolBar>
#include <QCheckBox>
#include <QRadioButton>
#include <QBitmap>

#include "render.h"
#include "color.h"
#include "settings.h"
#include "macros.h"

Q_DECL_EXPORT Render Render::m_instance;

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

/*
// Exponential blur, Jani Huhtanen, 2006 ==========================
*  expblur(QImage &img, int radius)
*
*  In-place blur of image 'img' with kernel of approximate radius 'radius'.
*  Blurs with two sided exponential impulse response.
*
*  aprec = precision of alpha parameter in fixed-point format 0.aprec
*  zprec = precision of state parameters zR,zG,zB and zA in fp format 8.zprec
*/

template<int aprec, int zprec>
static inline void blurinner(unsigned char *bptr, int &zR, int &zG, int &zB, int &zA, int alpha)
{
    int R,G,B,A;
    R = *bptr;
    G = *(bptr+1);
    B = *(bptr+2);
    A = *(bptr+3);

    zR += (alpha * ((R<<zprec)-zR))>>aprec;
    zG += (alpha * ((G<<zprec)-zG))>>aprec;
    zB += (alpha * ((B<<zprec)-zB))>>aprec;
    zA += (alpha * ((A<<zprec)-zA))>>aprec;

    *bptr =     zR>>zprec;
    *(bptr+1) = zG>>zprec;
    *(bptr+2) = zB>>zprec;
    *(bptr+3) = zA>>zprec;
}

template<int aprec,int zprec>
static inline void blurrow( QImage & im, int line, int alpha)
{
    int zR,zG,zB,zA;

    QRgb *ptr = (QRgb *)im.scanLine(line);

    zR = *((unsigned char *)ptr    )<<zprec;
    zG = *((unsigned char *)ptr + 1)<<zprec;
    zB = *((unsigned char *)ptr + 2)<<zprec;
    zA = *((unsigned char *)ptr + 3)<<zprec;

    for(int index=1; index<im.width(); index++)
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);

    for(int index=im.width()-2; index>=0; index--)
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);
}

template<int aprec, int zprec>
static inline void blurcol( QImage & im, int col, int alpha)
{
    int zR,zG,zB,zA;

    QRgb *ptr = (QRgb *)im.bits();
    ptr+=col;

    zR = *((unsigned char *)ptr    )<<zprec;
    zG = *((unsigned char *)ptr + 1)<<zprec;
    zB = *((unsigned char *)ptr + 2)<<zprec;
    zA = *((unsigned char *)ptr + 3)<<zprec;

    for(int index=im.width(); index<(im.height()-1)*im.width(); index+=im.width())
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);

    for(int index=(im.height()-2)*im.width(); index>=0; index-=im.width())
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);
}

void
Render::expblur(QImage &img, int radius, Qt::Orientations o)
{
    if(radius<1)
        return;

    static const int aprec = 16; static const int zprec = 7;

    // Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
    int alpha = (int)((1<<aprec)*(1.0f-expf(-2.3f/(radius+1.f))));

    if (o & Qt::Horizontal) {
        for(int row=0;row<img.height();row++)
            blurrow<aprec,zprec>(img,row,alpha);
    }

    if (o & Qt::Vertical) {
        for(int col=0;col<img.width();col++)
            blurcol<aprec,zprec>(img,col,alpha);
    }
}

Render
*Render::instance()
{
    return &m_instance;
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
    makeNoise();
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
                rect.setBottom(rect.bottom()-1);
            }
            p.setBrush(Qt::black);
            p.drawRoundedRect(rect, rnd, rnd);

            int steps(qBound(1, qMax(0, r-1), 3));
            int alpha(300/steps);
            for (int i = 1; i < steps+1; ++i)
            {
                int sides(qMin(i, 2));
                QRect ret(rect.adjusted(sides, i, -sides, -qMin(i, 1)));
                p.setCompositionMode(QPainter::CompositionMode_SourceOver);
                p.setBrush(Qt::black);
                p.drawRoundedRect(ret, rnd, rnd);
                p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
                p.setBrush(QColor(0, 0, 0, qMin(255, alpha*i)));
                p.drawRoundedRect(ret, rnd, rnd);
            }
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
            int n(3);
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
//            QRect rect(pix.rect());
            QPixmap white(pix.size()-QSize(0, 1));
            white.fill(Qt::transparent);
            QPainter pt(&white);
            pt.setRenderHint(QPainter::Antialiasing);
            pt.setBrush(QColor(255, 255, 255, 255));
            pt.setPen(Qt::NoPen);
            pt.drawRoundedRect(white.rect(), r, r);
            pt.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            pt.drawRoundedRect(white.rect().adjusted(1, 1, -1, -1), r-1, r-1);
            pt.end();

            QPixmap black(pix.size()-QSize(0, 1));
            black.fill(Qt::transparent);
            pt.begin(&black);
            pt.setRenderHint(QPainter::Antialiasing);
            pt.setPen(Qt::NoPen);
            pt.setBrush(Qt::black);
            pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
            pt.drawRoundedRect(black.rect(), r, r);
            pt.setCompositionMode(QPainter::CompositionMode_DestinationOut);
            pt.drawRoundedRect(black.rect().adjusted(1, 1, -1, -1), r-1, r-1);
            pt.end();

            p.drawTiledPixmap(white.rect().translated(0, 1), white);
            p.drawTiledPixmap(black.rect(), black);
            break;
        }
        case Simple:
        {
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::black);
            p.drawRoundedRect(pix.rect(), r, r);

            if (r > 1)
            {
                p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
                p.drawRoundedRect(pix.rect().adjusted(1, 1, -1, -1), r-1, r-1);
            }
        }
        case Strenghter:
        {
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::black);
            QRect rt(pix.rect().adjusted(0, 0, 0, -1));
            p.drawRoundedRect(rt, r, r);
            if (r > 1)
            {
                p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
                p.drawRoundedRect(rt.adjusted(1, 1, -1, -1), r-1, r-1);
            }
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

static QPainterPath tab(const QRect &r, int rnd)
{
    int x1, y1, x2, y2;
    r.getCoords(&x1, &y1, &x2, &y2);
    QPainterPath path;
    path.moveTo(x2, y1);
    path.quadTo(x2-rnd, y1, x2-rnd, y1+rnd);
    path.lineTo(x2-rnd, y2-rnd);
    path.quadTo(x2-rnd, y2, x2-rnd*2, y2);
    path.lineTo(x1+rnd*2, y2);
    path.quadTo(x1+rnd, y2, x1+rnd, y2-rnd);
    path.lineTo(x1+rnd, y1+rnd);
    path.quadTo(x1+rnd, y1, x1, y1);
    path.lineTo(x2, y1);
    path.closeSubpath();
    return path;
}

static const int ts(4); //tab shadow...

void
Render::initTabs()
{
    const int tr(Settings::conf.tabs.safrnd);
    int hsz = (tr*4)+(ts*4);
    int vsz = hsz/2;
    ++hsz;
    ++vsz;
    QImage img(hsz, vsz, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    const QRect r(img.rect());
    QPainterPath path(tab(r.adjusted(ts, 0, -ts, 0), tr));
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawPath(path);
    p.end();
    img = blurred(img, img.rect(), ts);
    p.begin(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black, 2.0f));
    p.setBrush(Qt::black);
    p.drawPath(path);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut); //erase the tabbar shadow... otherwise double.
    p.setPen(Qt::NoPen);
    p.drawPath(path);
    p.setPen(Qt::black);
    p.drawLine(img.rect().topLeft(), img.rect().topRight());
    renderShadow(Sunken, img.rect(), &p, ts*4, Top, 0.2f);
    p.end();
    --hsz;
    hsz/=2;
    --vsz;
    vsz/=2;
    m_tab[TopLeftPart] = QPixmap::fromImage(img.copy(0, 0, hsz, vsz));
    m_tab[TopMidPart] = QPixmap::fromImage(img.copy(hsz, 0, 1, vsz));
    m_tab[TopRightPart] = QPixmap::fromImage(img.copy(hsz+1, 0, hsz, vsz));
    m_tab[LeftPart] = QPixmap::fromImage(img.copy(0, vsz, hsz, 1));
    m_tab[CenterPart] = QPixmap::fromImage(img.copy(hsz, vsz, 1, 1));
    m_tab[RightPart] = QPixmap::fromImage(img.copy(hsz+1, vsz, hsz, 1));
    m_tab[BottomLeftPart] = QPixmap::fromImage(img.copy(0, vsz+1, hsz, vsz));
    m_tab[BottomMidPart] = QPixmap::fromImage(img.copy(hsz, vsz+1, 1, vsz));
    m_tab[BottomRightPart] = QPixmap::fromImage(img.copy(hsz+1, vsz+1, hsz, vsz));
}

void
Render::_renderTab(const QRect &r, QPainter *p, const Tab t, QPainterPath *path, const float o)
{
    const QSize sz(m_tab[TopLeftPart].size());
    if (r.width()*2+1 < sz.width()*2+1)
        return;
    int x1, y1, x2, y2;
    r.getCoords(&x1, &y1, &x2, &y2);
    int halfH(r.width()-(sz.width()*2)), halfV(r.height()-(sz.height()*2));
    p->save();
    p->setOpacity(o);

    if (t > BeforeSelected)
    {
        p->drawTiledPixmap(QRect(x1+sz.width()+halfH, y1, sz.width(), sz.height()), m_tab[TopRightPart]);
        p->drawTiledPixmap(QRect(x1+sz.width()+halfH, y1+sz.height(), sz.width(), halfV), m_tab[RightPart]);
        p->drawTiledPixmap(QRect(x1+sz.width()+halfH, y1+sz.height()+halfV, sz.width(), sz.height()), m_tab[BottomRightPart]);
    }
    if (t < AfterSelected)
    {
        p->drawTiledPixmap(QRect(x1, y1, sz.width(), sz.height()), m_tab[TopLeftPart]);
        p->drawTiledPixmap(QRect(x1, y1+sz.height(), sz.width(), halfV), m_tab[LeftPart]);
        p->drawTiledPixmap(QRect(x1, y1+sz.height()+halfV, sz.width(), sz.height()), m_tab[BottomLeftPart]);
    }
    p->drawTiledPixmap(QRect(x1+sz.width(), y1, halfH, sz.height()), m_tab[TopMidPart]);
    p->drawTiledPixmap(QRect(x1+sz.width(), y1+sz.height(), halfH, halfV), m_tab[CenterPart]);
    p->drawTiledPixmap(QRect(x1+sz.width(), y1+sz.height()+halfV, halfH, sz.height()), m_tab[BottomMidPart]);

    if (path)
        *path = tab(r.adjusted(ts, 0, -ts, 0), Settings::conf.tabs.safrnd);
    p->restore();
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
Render::_renderMask(const QRect &rect, QPainter *painter, const QBrush &brush, int roundNess, const Sides sides, const QPoint &offSet)
{
    if (!rect.isValid())
        return;
    roundNess = qMin(qMin(MAXRND, roundNess), qMin(rect.height(), rect.width())/2);
    if (!roundNess)
        roundNess = 1;

    QPixmap pix(rect.size());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    QRect r(pix.rect());
    if (!offSet.isNull())
    {
        const int x(offSet.x()), y(offSet.y());
        if (x < 0)
            r.setRight(r.right()+qAbs(x));
        else
            r.setLeft(r.left()-qAbs(x));
        if (y < 0)
            r.setBottom(r.bottom()+qAbs(y));
        else
            r.setTop(r.top()-qAbs(y));
        p.translate(x, y);
    }
    p.fillRect(r, brush);
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
Render::_renderShadowPrivate(const Shadow shadow, const QRect &rect, QPainter *painter, int roundNess, const float opacity, const Sides sides)
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
            {
                QPixmap pix = m_shadow[shadow][roundNess][i];
                painter->drawTiledPixmap(partRect(QRect(QPoint(0, 0), rect.size()), (Part)i, roundNess, sides).translated(rect.x(), rect.y()), pix);
            }
    painter->setOpacity(o);
}

void
Render::_renderShadow(const Shadow shadow, const QRect &rect, QPainter *painter, int roundNess, const Sides sides, const float opacity, const QBrush *brush)
{
    if (!brush)
    {
        _renderShadowPrivate(shadow, rect, painter, roundNess, opacity, sides);
        return;
    }
    QPixmap pix(rect.size());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    _renderShadowPrivate(shadow, pix.rect(), &p, roundNess, 1.0f, sides);
    p.end();
    colorizePixmap(pix, *brush);
    painter->drawTiledPixmap(rect, pix);
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
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.fillRect(tmp.rect(), pix);
    p.end();
    pix = QPixmap::fromImage(tmp);

    QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
    QRgb *tmpColors = reinterpret_cast<QRgb *>(tmp.bits());
    QRgb *pixColors = reinterpret_cast<QRgb *>(img.bits());
    const int s(pix.height()*pix.width());
    for (int i = 0; i < s; ++i)
    {
        QColor c(tmpColors[i]);
        c.setAlpha(qAlpha(pixColors[i]));
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

static int randInt(int low, int high)
{
    // Random number between low and high
    return qrand() % ((high + 1) - low) + low;
}

void
Render::makeNoise()
{
    static int s(512);
    QImage noise(s, s, QImage::Format_ARGB32);
    noise.fill(Qt::transparent);
    QRgb *rgb = reinterpret_cast<QRgb *>(noise.bits());
    const int size(s*s);
    for (int i = 0; i < size; ++i)
    {
        int v(randInt(0, 255));
        rgb[i] = QColor(v, v, v).rgb();
    }
    m_noise = QPixmap::fromImage(noise);
}

QPixmap
Render::mid(const QPixmap &p1, const QPixmap &p2, const int a1, const int a2, const QSize &sz)
{
    int w(qMin(p1.width(), p2.width()));
    int h(qMin(p1.height(), p2.height()));
    if (sz.isValid())
    {
        w = sz.width();
        h = sz.height();
    }
    QImage i1 = p1.copy(0, 0, w, h).toImage().convertToFormat(QImage::Format_ARGB32);
    QImage i2 = p2.copy(0, 0, w, h).toImage().convertToFormat(QImage::Format_ARGB32);
    const int size(w*h);
    QImage i3(w, h, QImage::Format_ARGB32); //the resulting image
    i3.fill(Qt::transparent);
    QRgb *rgb1 = reinterpret_cast<QRgb *>(i1.bits());
    QRgb *rgb2 = reinterpret_cast<QRgb *>(i2.bits());
    QRgb *rgb3 = reinterpret_cast<QRgb *>(i3.bits());

    for (int i = 0; i < size; ++i)
    {
        const QColor c = Color::mid(QColor::fromRgba(rgb1[i]), QColor::fromRgba(rgb2[i]), a1, a2);
        rgb3[i] = c.rgba();
    }
    return QPixmap::fromImage(i3);
}

QPixmap
Render::mid(const QPixmap &p1, const QBrush &b, const int a1, const int a2)
{
    QPixmap p2(p1.size());
    p2.fill(Qt::transparent);
    QPainter p(&p2);
    p.fillRect(p2.rect(), b);
    p.end();
    return mid(p1, p2, a1, a2);
}

void
Render::drawClickable(const Shadow s, QRect r, QPainter *p, const Sides sides, int rnd, const float opacity, const QWidget *w, QBrush *mask, QBrush *shadow, const bool needStrong, const QPoint &offSet)
{
    rnd = qBound(1, rnd, qMin(r.height(), r.width())/2);
    if (s >= ShadowCount)
        return;
    const int o(opacity*255);
    if (s==Raised || s==Simple)
    {
        if (s==Raised)
            r.sAdjust(1, 1, -1, 0);
        if (s==Simple && !shadow)
        {
            QLinearGradient lg(0, 0, 0, r.height());
            lg.setColorAt(0.0f, QColor(0, 0, 0, o/3));
            lg.setColorAt(0.8f, QColor(0, 0, 0, o/3));
            lg.setColorAt(1.0f, QColor(0, 0, 0, o));
            QBrush sh(lg);
            renderShadow(s, r, p, rnd, sides, opacity, &sh);
        }
        else
            renderShadow(s, r, p, rnd, sides, opacity, shadow);
        const bool inToolBar(w&&qobject_cast<const QToolBar *>(w->parentWidget()));
        const int m(1);
        if (s==Simple)
            r.sAdjust(!inToolBar, !inToolBar, -!inToolBar, -m);
        else
            r.sAdjust(m, m, -m, -(m+1));
        if (!inToolBar || s==Raised)
            rnd = qMax(1, rnd-m);
    }
    else if (s==Carved)
    {
        QLinearGradient lg(0, 0, 0, r.height());
        int high(63), low(63);
        if (w && w->parentWidget())
        {
            QWidget *p(w->parentWidget());
            const bool isDark(Color::luminosity(p->palette().color(p->foregroundRole())) > Color::luminosity(p->palette().color(p->backgroundRole())));
//            const QColor bg(p->palette().color(p->backgroundRole()));
//            high = bg.value()/2;
//            low = qMin(255, 280-bg.value());
            if (isDark)
            {
                high=32;
                low=192;
            }
            else
            {
                high=192;
                low=32;
            }
        }
        lg.setColorAt(0.1f, QColor(0, 0, 0, low));
        lg.setColorAt(1.0f, QColor(255, 255, 255, high));
        renderMask(r, p, lg, rnd, sides, offSet);
        const int m(2);
        const bool needHor(!qobject_cast<const QRadioButton *>(w)&&!qobject_cast<const QCheckBox *>(w)&&r.width()!=r.height());
        r.sAdjust((m+needHor), m, -(m+needHor), -m);
        rnd = qMin(rnd-m, MAXRND);
        renderShadow(Strenghter, r.adjusted(0, 0, 0, 1), p, rnd, sides, opacity);
        r.sShrink(1);
        rnd = qMin(MAXRND, rnd+1);
    }
    else
    {
        r.sAdjust(1, 1, -1, -2);
        rnd = qMax(1, rnd-1);
    }

    if (mask)
        renderMask(r, p, *mask, rnd, sides, offSet);
    else if (s==Carved)
    {
        QBrush newMask(Qt::black);
        const QPainter::CompositionMode mode(p->compositionMode());
        p->setCompositionMode(QPainter::CompositionMode_DestinationOut);
        renderMask(r, p, newMask, rnd, sides, offSet);
        p->setCompositionMode(mode);
    }

    if (s==Sunken||s==Etched)
    {
        r.sAdjust(-1, -1, 1, 2);
        rnd = qMin(rnd+1, MAXRND);
        renderShadow(s, r, p, rnd, sides, opacity);
        if (needStrong)
            renderShadow(Strenghter, r, p, rnd, sides, opacity);
    }
}

Render::Pos
Render::pos(const Sides s, const Qt::Orientation o)
{
    if (o == Qt::Horizontal)
    {
        if (s&Left&&!(s&Right))
            return First;
        else if (s&Right&&!(s&Left))
            return Last;
        else if (s==All)
            return Alone;
        else
            return Middle;
    }
    else
    {
        if (s&Top&&!(s&Bottom))
            return First;
        else if (s&Bottom&&!(s&Top))
            return Last;
        else if (s==All)
            return Alone;
        else
            return Middle;
    }
}

int
Render::maskHeight(const Shadow s, const int height)
{
    switch (s)
    {
    case Render::Sunken:
    case Render::Etched:
    case Render::Raised:
        return height-3;
    case Render::Simple:
        return height-1;
    case Render::Carved:
        return height-6;
    default: return 0;
    }
}

int
Render::maskWidth(const Shadow s, const int width)
{
    switch (s)
    {
    case Render::Sunken:
    case Render::Etched:
    case Render::Raised:
        return width-2;
    case Render::Simple:
        return width;
    case Render::Carved:
        return width-6;
    default: return 0;
    }
}

QRect
Render::maskRect(const Shadow s, const QRect &r, const Sides sides)
{
    switch (s)
    {
    case Sunken:
    case Etched:
    case Raised: return r.sAdjusted(1, 1, -1, -2); break;
    case Simple: return r.sAdjusted(0, 0, 0, -1); break;
    case Carved: return r.sAdjusted(3, 3, -3, -3); break;
    default: return r;
    }
}

QPixmap
Render::sunkenized(const QRect &r, const QPixmap &source, const bool isDark, const QColor &ref)
{
    int m(4);
    QImage img(r.size()+QSize(m, m), QImage::Format_ARGB32);
    m/=2;
    img.fill(Qt::transparent);
    QPainter p(&img);
    int rgb(isDark?255:0);
    int alpha(qMin(255.0f, 2*Settings::conf.shadows.opacity*255.0f));
    p.fillRect(img.rect(), QColor(rgb, rgb, rgb, alpha));
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.drawTiledPixmap(r.translated(m, m), source);
    p.end();
    const QImage blur = Render::blurred(img, img.rect(), m*2);
    QPixmap highlight(r.size());
    highlight.fill(Qt::transparent);
    p.begin(&highlight);
    rgb = isDark?0:255;
    p.fillRect(highlight.rect(), QColor(rgb, rgb, rgb, alpha));
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawTiledPixmap(highlight.rect(), source);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.drawTiledPixmap(highlight.rect().translated(0, -1), source);
    p.end();

//    const int y(isDark?-1:1);
    QPixmap pix(r.size());
    pix.fill(Qt::transparent);
    p.begin(&pix);
    p.drawTiledPixmap(pix.rect(), source);
    if (!isDark)
        p.drawTiledPixmap(pix.rect().translated(0, 1), QPixmap::fromImage(blur), QPoint(m, m));
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawTiledPixmap(pix.rect(), source);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    p.drawTiledPixmap(pix.rect().translated(0, 1), highlight);
    p.end();
    return pix;
}

//colortoalpha directly stolen from gimp

void
colortoalpha (float *a1,
          float *a2,
          float *a3,
          float *a4,
          float c1,
          float c2,
          float c3)
{
  float alpha1, alpha2, alpha3, alpha4;

  alpha4 = *a4;

  if ( *a1 > c1 )
    alpha1 = (*a1 - c1)/(255.0-c1);
  else if ( *a1 < c1 )
    alpha1 = (c1 - *a1)/(c1);
  else alpha1 = 0.0;

  if ( *a2 > c2 )
    alpha2 = (*a2 - c2)/(255.0-c2);
  else if ( *a2 < c2 )
    alpha2 = (c2 - *a2)/(c2);
  else alpha2 = 0.0;

  if ( *a3 > c3 )
    alpha3 = (*a3 - c3)/(255.0-c3);
  else if ( *a3 < c3 )
    alpha3 = (c3 - *a3)/(c3);
  else alpha3 = 0.0;

  if ( alpha1 > alpha2 )
    if ( alpha1 > alpha3 )
      {
    *a4 = alpha1;
      }
    else
      {
    *a4 = alpha3;
      }
  else
    if ( alpha2 > alpha3 )
      {
    *a4 = alpha2;
      }
    else
      {
    *a4 = alpha3;
      }

  *a4 *= 255.0;

  if ( *a4 < 1.0 )
    return;
  *a1 = 255.0 * (*a1-c1)/ *a4 + c1;
  *a2 = 255.0 * (*a2-c2)/ *a4 + c2;
  *a3 = 255.0 * (*a3-c3)/ *a4 + c3;

  *a4 *= alpha4/255.0;
}

static int stretch(const int v, const float n = 2.0f)
{
    static bool isInit(false);
    static float table[256];
    if (!isInit)
    {
        for (int i = 127; i < 256; ++i)
            table[i] = (pow(((float)i/127.5f-1.0f), (1.0f/n))+1.0f)*127.5f;
        for (int i = 0; i < 128; ++i)
            table[i] = 255-table[255-i];
        isInit = true;
    }
    return qRound(table[v]);
}

/**
    http://spatial-analyst.net/ILWIS/htm/ilwisapp/stretch_algorithm.htm
    OUTVAL
    (INVAL - INLO) * ((OUTUP-OUTLO)/(INUP-INLO)) + OUTLO
    where:
    OUTVAL
    Value of pixel in output map
    INVAL
    Value of pixel in input map
    INLO
    Lower value of 'stretch from' range
    INUP
    Upper value of 'stretch from' range
    OUTLO
    Lower value of 'stretch to' range
    OUTUP
    Upper value of 'stretch to' range
 */

static QImage stretched(QImage img, const QColor &c)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    int size = img.width() * img.height();
    QRgb *pixels[2] = { 0, 0 };
    pixels[0] = reinterpret_cast<QRgb *>(img.bits());
    int r, g, b;
    c.getRgb(&r, &g, &b); //foregroundcolor

#define ENSUREALPHA if (!qAlpha(pixels[0][i])) continue
    QList<int> hues;
    for (int i = 0; i < size; ++i)
    {
        ENSUREALPHA;
        const int hue(QColor::fromRgba(pixels[0][i]).hue());
        if (!hues.contains(hue))
            hues << hue;
    }
    const bool useAlpha(hues.count() == 1);
    if (useAlpha)
    {
        /**
          * Alpha gets special treatment...
          * we simply steal the colortoalpha
          * function from gimp, color white to
          * alpha, as in remove all white from
          * the image (in case of monochrome icons
          * add some sorta light bevel). Then simply
          * push all remaining alpha values so the
          * highest one is 255.
          */
        float alpha[size], lowAlpha(255), highAlpha(0);
        for (int i = 0; i < size; ++i)
        {
            ENSUREALPHA;
            const QRgb rgb(pixels[0][i]);
            float val(255.0f);
            float red(qRed(rgb)), green(qGreen(rgb)), blue(qBlue(rgb)), a(qAlpha(rgb));
            colortoalpha(&red, &green, &blue, &a, val, val, val);
            if (a < lowAlpha)
                lowAlpha = a;
            if (a > highAlpha)
                highAlpha = a;
            alpha[i] = a;
        }
        float add(255.0f/highAlpha);
        for (int i = 0; i < size; ++i)
        {
            ENSUREALPHA;
            pixels[0][i] = qRgba(r, g, b, stretch(qMin<int>(255, alpha[i]*add), 2.0f));
        }
        return img;
    }

    const int br(img.width()/4);
    QImage bg(img.size() + QSize(br*2, br*2), QImage::Format_ARGB32); //add some padding avoid 'edge folding'
    bg.fill(Qt::transparent);

    QPainter bp(&bg);
    bp.drawPixmap(br, br, QPixmap::fromImage(img), 0, 0, img.width(), img.height());
    bp.end();
    Render::blurred(bg, bg.rect(), br);
    bg = bg.copy(bg.rect().adjusted(br, br, -br, -br)); //remove padding so we can easily access relevant pixels with [i]

    enum Rgba { Red = 0, Green, Blue, Alpha };
    enum ImageType { Fg = 0, Bg }; //fg is the actual image, bg is the blurred one we use as reference for the most contrasting channel

    pixels[1] = reinterpret_cast<QRgb *>(bg.bits());
    double rgbCount[3] = { 0.0d, 0.0d, 0.0d };
    int (*rgba[4])(QRgb rgb) = { qRed, qGreen, qBlue, qAlpha };
    int cVal[2][4] = {{ 0, 0, 0, 0 }, {0, 0, 0, 0}};

    for (int i = 0; i < size; ++i) //pass 1, determine most contrasting channel, usually RED
    for (int it = 0; it < 2; ++it)
    for (int d = 0; d < 4; ++d)
    {
        ENSUREALPHA;
        cVal[it][d] = (*rgba[d])(pixels[it][i]);
        if (it && d < 3)
            rgbCount[d] += (cVal[Fg][Alpha]+cVal[Bg][Alpha]) * qAbs(cVal[Fg][d]-cVal[Bg][d]) / (2.0f * cVal[Bg][d]);
    }
    double count = 0;
    int channel = 0;
    for (int i = 0; i < 3; ++i)
        if (rgbCount[i] > count)
        {
            channel = i;
            count = rgbCount[i];
        }
    float values[size];
    float inLo = 255.0f, inHi = 0.0f;
    qulonglong loCount(0), hiCount(0);
    for ( int i = 0; i < size; ++i ) //pass 2, store darkest/lightest pixels
    {
        ENSUREALPHA;
        const float px((*rgba[channel])(pixels[Fg][i]));
        values[i] = px;
        if ( px > inHi )
            inHi = px;
        if ( px < inLo )
            inLo = px;
        const int a(qAlpha(pixels[Fg][i]));
        if (px < 128)
            loCount += a*(255-px);
        else
            hiCount += a*px;
    }
    const bool isDark(loCount>hiCount);
    for ( int i = 0; i < size; ++i )
    {
        ENSUREALPHA;
        int a(qBound<int>(0, qRound((values[i] - inLo) * (255.0f / (inHi - inLo))), 255));
        if (isDark)
            a = qMax(0, qAlpha(pixels[Fg][i])-a);
        pixels[Fg][i] = qRgba(r, g, b, stretch(a));
    }
#undef ENSUREALPHA
    return img;
}

QPixmap
Render::monochromized(const QPixmap &source, const QColor &color, const Effect effect, const bool isDark)
{
    const QPixmap &result = QPixmap::fromImage(stretched(source.toImage(), color));
    if (effect == Inset)
        return sunkenized(result.rect(), result, isDark);
    return result;
}

