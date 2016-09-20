#pragma once

#include "GenericBlockLayout.h"
#include "ILinkPreviewBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class LinkPreviewBlock;

class LinkPreviewBlockLayout final
    : public GenericBlockLayout
    , public ILinkPreviewBlockLayout
{
public:
    LinkPreviewBlockLayout();

    virtual IItemBlockLayout* asBlockLayout() override;

    virtual QLayout* asQLayout() override;

    virtual void cutTextByPreferredSize(
        const QString &title,
        const QString &annotation,
        const QRect &blockLtr,
        Out QString &cutTitle,
        Out QString &cutAnnotation) const override;

    virtual QFont getAnnotationFont() const override;

    virtual QRect getAnnotationRect() const override;

    virtual const IBoxModel& getBlockBoxModel() const override;

    virtual QSize getMaxPreviewSize() const override;

    virtual QRect getFaviconImageRect() const override;

    virtual QRect getPreviewImageRect() const override;

    virtual QPoint getSiteNamePos() const override;

    virtual QRect getSiteNameRect() const override;

    virtual QFont getTitleFont() const override;

    virtual QRect getTitleRect() const override;

    virtual bool isAnnotationVisible() const override;

private:
    QRect FaviconImageRect_;

    QRect PreviewImageRect_;

    QRect CurrentTitleGeometry_;

    QRect SiteNameGeometry_;

    QRect TitleGeometry_;

    QRect AnnotationGeometry_;

    QRect evaluateAnnotationLtr(const QRect &titleGeometry) const;

    QRect evaluateFaviconImageRect(const QRect &bottomTextBlockGeometry) const;

    QRect evaluatePreviewContentLtr(const QRect &widgetLtr) const;

    QRect evaluatePreviewImageRect(const QRect &previewContentLtr, const bool isInPreloadingState) const;

    QRect evaluateSiteNameGeometry(const QRect &bottomTextBlockGeometry, const QRect &faviconGeometry, const QRect &titleGeometry) const;

    QRect evaluateTitleLtr(const QRect &previewContentLtr, const int32_t previewWidth, const bool isPlaceholder) const;

    int32_t evaluateWidgetHeight(
        const QRect &previewImageGeometry,
        const QRect &faviconGeometry,
        const QRect &siteNameGeometry);

    QRect setAnnotationGeometry(const QRect &ltr);

    virtual QSize setBlockGeometryInternal(const QRect &widgetLtr) override;

    QRect setTitleGeometry(const QRect &ltr);

};

UI_COMPLEX_MESSAGE_NS_END