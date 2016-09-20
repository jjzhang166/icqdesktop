#pragma once

#include "GenericBlockLayout.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

class ImagePreviewBlock;

class ImagePreviewBlockLayout final : public GenericBlockLayout
{
public:
    ImagePreviewBlockLayout();

    virtual ~ImagePreviewBlockLayout() override;

    virtual QSize blockSizeForMaxWidth(const int32_t maxWidth) override;

    const QRect& getPreviewRect() const;

protected:
    virtual QSize setBlockGeometryInternal(const QRect &blockLtr);

private:
    void setActionButtonGeometry(const QRect &previewRect, ImagePreviewBlock &block);

    void setPreviewGeometry(const QRect &blockLtr, ImagePreviewBlock &block);

    QRect PreviewRect_;

};

UI_COMPLEX_MESSAGE_NS_END