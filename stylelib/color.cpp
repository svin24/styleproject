#include "color.h"

QColor Color::titleBarColors[2];

QColor
Color::mid(const QColor &c1, const QColor c2, int i1, int i2)
{
    const int i3 = i1+i2;
    int r,g,b,a;
    r = qMin(255,(i1*c1.red() + i2*c2.red())/i3);
    g = qMin(255,(i1*c1.green() + i2*c2.green())/i3);
    b = qMin(255,(i1*c1.blue() + i2*c2.blue())/i3);
    a = qMin(255,(i1*c1.alpha() + i2*c2.alpha())/i3);
    return QColor(r,g,b,a);
}

int
Color::luminosity(const QColor &c)
{
    int r, g, b, a;
    c.getRgb(&r, &g, &b, &a);
    return (r*299 + g*587 + b*114)/1000;
}

bool
Color::contrast(const QColor &c1, const QColor &c2)
{
    int lum1 = luminosity(c1), lum2 = luminosity(c2);
    if (qAbs(lum2-lum1)<125)
        return false;

    int r(qAbs(c1.red()-c2.red())),
            g(qAbs(c1.green()-c2.green())),
            b(qAbs(c1.blue()-c2.blue()));
    return (r+g+b>500);
}

void
Color::setValue(const int value, QColor &c)
{
    c.setHsv(c.hue(), c.saturation(), qBound(0, value, 255), c.alpha());
}

void
Color::ensureContrast(QColor &c1, QColor &c2)
{
    if (contrast(c1, c2))
        return;
    QColor dark = c1;
    QColor light = c2;
    bool inv(false);
    if (luminosity(c2)<luminosity(c1))
    {
        dark = c2;
        light = c1;
        inv = true;
    }
    while (!contrast(dark, light))
    {
        setValue(dark.value()-1, dark);
        setValue(light.value()+1, light);
    }
    c1 = inv ? light : dark;
    c2 = inv ? dark : light;
}

