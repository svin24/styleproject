#include <QStylePlugin>
#include <QWidget>
#include <QStyleOption>
#include <QStyleOptionComplex>
#include <QStyleOptionSlider>
#include <QStyleOptionComboBox>
#include <QDebug>
#include <qmath.h>
#include <QEvent>
#include <QToolBar>
#include <QAbstractItemView>
#include <QMainWindow>
#include <QGroupBox>
#include <QStyleOptionProgressBarV2>
#include <QStyleOptionDockWidget>
#include <QStyleOptionDockWidgetV2>
#include <QProgressBar>
#include <QStyleOptionToolButton>

#include "styleproject.h"
#include "stylelib/progresshandler.h"
#include "stylelib/ops.h"

QRect
StyleProject::subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const
{
    if (cc < CCSize && m_sc[cc])
        return (this->*m_sc[cc])(opt, sc, w);

    switch (cc)
    {
    default: return QCommonStyle::subControlRect(cc, opt, sc, w);
    }
}

QRect
StyleProject::subElementRect(SubElement r, const QStyleOption *opt, const QWidget *widget) const
{
    switch (r)
    {
    case SE_TabBarTabLeftButton:
    case SE_TabBarTabRightButton:
    case SE_TabBarTabText:
    {
        castOpt(TabV3, tab, opt);
        castObj(const QTabBar *, bar, widget);
        if (!tab)
            return QRect();

        int overlap(pixelMetric(PM_TabBarTabOverlap));
        QRect textRect(tab->rect);
        QRect leftRect;

        const bool east(tab->shape == QTabBar::RoundedEast || tab->shape == QTabBar::TriangularEast);
        const bool west(tab->shape == QTabBar::RoundedWest || tab->shape == QTabBar::TriangularWest);

        if (tab->leftButtonSize.isValid())
        {
            if ((tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab) && Ops::isSafariTabBar(bar))
                overlap *= 2;
            const QSize sz(tab->leftButtonSize);
            leftRect.setSize(sz);
            leftRect.moveCenter(tab->rect.center());
            if (west)
                leftRect.moveBottom(tab->rect.bottom()-overlap);
            else if (east)
                leftRect.moveTop(tab->rect.top()+overlap);
            else
                leftRect.moveLeft(tab->rect.left()+overlap);
        }

        if (leftRect.isValid())
        {
            if (west)
                textRect.setBottom(leftRect.top());
            else if (east)
                textRect.setTop(leftRect.bottom());
            else
                textRect.setLeft(leftRect.right());
        }

        QRect rightRect;
        if (tab->rightButtonSize.isValid())
        {
            const QSize sz(tab->rightButtonSize);
            rightRect.setSize(sz);
            rightRect.moveCenter(tab->rect.center());
            if (west)
                rightRect.moveBottom(tab->rect.bottom()-overlap);
            else if (east)
                rightRect.moveTop(tab->rect.top()+overlap);
            else
                rightRect.moveRight(tab->rect.right()-overlap);
        }

        if (rightRect.isValid())
        {
            if (west)
                textRect.setBottom(rightRect.top());
            else if (east)
                textRect.setTop(rightRect.bottom());
            else
                textRect.setRight(rightRect.left());
        }

        if (tab->text.isEmpty())
            textRect = QRect();

        switch (r)
        {
        case SE_TabBarTabLeftButton:
            return visualRect(tab->direction, tab->rect, leftRect);
        case SE_TabBarTabRightButton:
            return visualRect(tab->direction, tab->rect, rightRect);
        case SE_TabBarTabText:
            return visualRect(tab->direction, tab->rect, textRect);
        default: return QCommonStyle::subElementRect(r, opt, widget);
        }
    }
    case SE_ProgressBarLabel:
    case SE_ProgressBarGroove:
    case SE_ProgressBarContents: return opt->rect;
    case SE_ViewItemCheckIndicator:
    case SE_RadioButtonIndicator:
    case SE_CheckBoxIndicator:
    {
        QRect r(opt->rect.topLeft(), QPoint(opt->rect.topLeft()+QPoint(pixelMetric(PM_IndicatorWidth), pixelMetric(PM_IndicatorHeight))));
        return visualRect(opt->direction, opt->rect, r);
    }
    case SE_RadioButtonContents:
    case SE_CheckBoxContents:
    {
        const int pmInd(pixelMetric(PM_IndicatorWidth)), pmSpc(pixelMetric(PM_CheckBoxLabelSpacing)), w(opt->rect.width()-(pmInd+pmSpc));
        return visualRect(opt->direction, opt->rect, QRect(opt->rect.left()+pmInd+pmSpc, opt->rect.top(), w, opt->rect.height()));
    }
    case SE_DockWidgetCloseButton:
    case SE_DockWidgetFloatButton:
    case SE_DockWidgetTitleBarText:
    case SE_DockWidgetIcon:
    {
        castOpt(DockWidget, dw, opt);
        if (!dw)
            return QRect();

        QRect rect(dw->rect);

        QRect cr, fr;
        int left(rect.left());
        if (dw->closable)
        {
            cr = rect;
            cr.setWidth(cr.height());
            cr.moveLeft(left);
            left += cr.width();
        }
        if (dw->floatable)
        {
            fr = rect;
            fr.setWidth(fr.height());
            fr.moveLeft(left);
            left += cr.width();
        }
        switch (r)
        {
        case SE_DockWidgetCloseButton:
            return visualRect(dw->direction, rect, cr);
        case SE_DockWidgetFloatButton:
            return visualRect(dw->direction, rect, fr);
        case SE_DockWidgetTitleBarText:
            return visualRect(dw->direction, rect, QRect(left, rect.top(), qMax(0, rect.width()-left), rect.height()));
        case SE_DockWidgetIcon:
            return QRect(); // ever needed?
        default: return QRect();
        }
    }
    default: break;
    }
    return QCommonStyle::subElementRect(r, opt, widget);
}

/**
enum SubElement {
    SE_PushButtonContents,
    SE_PushButtonFocusRect,

    SE_CheckBoxIndicator,
    SE_CheckBoxContents,
    SE_CheckBoxFocusRect,
    SE_CheckBoxClickRect,

    SE_RadioButtonIndicator,
    SE_RadioButtonContents,
    SE_RadioButtonFocusRect,
    SE_RadioButtonClickRect,

    SE_ComboBoxFocusRect,

    SE_SliderFocusRect,

    SE_Q3DockWindowHandleRect,

    SE_ProgressBarGroove,
    SE_ProgressBarContents,
    SE_ProgressBarLabel,

    // ### Qt 5: These values are unused; eliminate them
    SE_DialogButtonAccept,
    SE_DialogButtonReject,
    SE_DialogButtonApply,
    SE_DialogButtonHelp,
    SE_DialogButtonAll,
    SE_DialogButtonAbort,
    SE_DialogButtonIgnore,
    SE_DialogButtonRetry,
    SE_DialogButtonCustom,

    SE_ToolBoxTabContents,

    SE_HeaderLabel,
    SE_HeaderArrow,

    SE_TabWidgetTabBar,
    SE_TabWidgetTabPane,
    SE_TabWidgetTabContents,
    SE_TabWidgetLeftCorner,
    SE_TabWidgetRightCorner,

    SE_ViewItemCheckIndicator,
    SE_ItemViewItemCheckIndicator = SE_ViewItemCheckIndicator,

    SE_TabBarTearIndicator,

    SE_TreeViewDisclosureItem,

    SE_LineEditContents,
    SE_FrameContents,

    SE_DockWidgetCloseButton,
    SE_DockWidgetFloatButton,
    SE_DockWidgetTitleBarText,
    SE_DockWidgetIcon,

    SE_CheckBoxLayoutItem,
    SE_ComboBoxLayoutItem,
    SE_DateTimeEditLayoutItem,
    SE_DialogButtonBoxLayoutItem, // ### remove
    SE_LabelLayoutItem,
    SE_ProgressBarLayoutItem,
    SE_PushButtonLayoutItem,
    SE_RadioButtonLayoutItem,
    SE_SliderLayoutItem,
    SE_SpinBoxLayoutItem,
    SE_ToolButtonLayoutItem,

    SE_FrameLayoutItem,
    SE_GroupBoxLayoutItem,
    SE_TabWidgetLayoutItem,

    SE_ItemViewItemDecoration,
    SE_ItemViewItemText,
    SE_ItemViewItemFocusRect,

    SE_TabBarTabLeftButton,
    SE_TabBarTabRightButton,
    SE_TabBarTabText,

    SE_ShapedFrameContents,

    SE_ToolBarHandle,

    // do not add any values below/greater than this
    SE_CustomBase = 0xf0000000
};
*/

QRect
StyleProject::comboBoxRect(const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const
{
    QRect ret;
    castOpt(ComboBox, cb, opt);
    if (!cb)
        return ret;
    const int arrowSize(cb->rect.height());
    switch (sc)
    {
    case SC_ComboBoxListBoxPopup: //what kinda rect should be returned here? seems only topleft needed...
    case SC_ComboBoxFrame: ret = cb->rect; break;
    case SC_ComboBoxArrow: ret = QRect(cb->rect.width()-arrowSize, 0, arrowSize, arrowSize); break;
    case SC_ComboBoxEditField: ret = cb->rect.adjusted(0, 0, -arrowSize, 0); break;
    default: ret = QRect(); break;
    }
    return visualRect(cb->direction, cb->rect, ret);
}

QRect
StyleProject::scrollBarRect(const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const
{
    QRect ret;
    castOpt(Slider, slider, opt);
    if (!slider)
        return ret;

    QRect r(slider->rect);
    switch (sc)
    {
    case SC_ScrollBarAddLine:
    case SC_ScrollBarSubLine:
        return ret;
    default: break;
    }

    bool hor(slider->orientation == Qt::Horizontal);
    int grooveSize(hor ? r.width() : r.height());
    unsigned int range(slider->maximum-slider->minimum);
    int gs((int)((slider->pageStep*grooveSize)));
    int ss((range+slider->pageStep));
    int sliderSize(0);
    if (ss && gs)
        sliderSize = gs/ss;
    if (sliderSize < pixelMetric(PM_ScrollBarSliderMin, opt, w))
        sliderSize = pixelMetric(PM_ScrollBarSliderMin, opt, w);
    int pos = sliderPositionFromValue(slider->minimum, slider->maximum, slider->sliderPosition, grooveSize-sliderSize, slider->upsideDown);

    switch (sc)
    {
    case SC_ScrollBarGroove: ret = r; break;
    case SC_ScrollBarSlider:
    {
        if (!range)
            break;
        if (hor)
            ret = QRect(pos, 0, sliderSize, r.height());
        else
            ret = QRect(0, pos, r.width(), sliderSize);
        break;
    }
    case SC_ScrollBarLast:
    case SC_ScrollBarAddPage:
        if (hor)
            ret = QRect(pos+sliderSize, 0, r.width()-(pos+sliderSize), r.height());
        else
            ret = QRect(0, pos+sliderSize, r.width(), r.height()-(pos+sliderSize));
        break;
    case SC_ScrollBarFirst:
    case SC_ScrollBarSubPage:
        if (hor)
            ret = QRect(0, 0, pos, r.height());
        else
            ret = QRect(0, 0, r.width(), pos);
        break;
    default: return QCommonStyle::subControlRect(CC_ScrollBar, opt, sc, w);
    }
    return visualRect(slider->direction, slider->rect, ret);
}

QRect
StyleProject::groupBoxRect(const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const
{
    QRect ret;
    castOpt(GroupBox, box, opt);
    if (!box)
        return ret;
    ret = box->rect;
    const int top(qMax(16, opt->fontMetrics.height()));
    const int left(8);
    switch (sc)
    {
    case SC_GroupBoxCheckBox:
        ret = QRect(ret.left()+left, ret.top(), top, top);
        break;
    case SC_GroupBoxContents:
        ret.setTop(ret.top()+top);
        break;
    case SC_GroupBoxFrame:
        break;
    case SC_GroupBoxLabel:
        ret = QRect(ret.left()+left+(box->subControls&SC_GroupBoxCheckBox?top:0), ret.top(), ret.width()-top, top);
        break;
    default: return QCommonStyle::subControlRect(CC_GroupBox, opt, sc, w);
    }
    return visualRect(box->direction, opt->rect, ret);
}

QRect
StyleProject::toolButtonRect(const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const
{
    QRect ret;
    castOpt(ToolButton, tb, opt);
    if (!tb)
        return ret;
    switch (sc)
    {
    case SC_ToolButton: ret = tb->rect; break;
    case SC_ToolButtonMenu:
    {
        ret = tb->rect;
        ret.setLeft(ret.right()-16);
        break;
    }
    default: break;
    }
    return visualRect(tb->direction, tb->rect, ret);
}

