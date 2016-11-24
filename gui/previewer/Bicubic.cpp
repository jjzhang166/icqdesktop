#include "stdafx.h"

#include "../main_window/mplayer/ffmpeg.h"

#include "Bicubic.h"

namespace Previewer
{
    QImage scaleBicubic(const QImage &_src, const QSize &_dstSize)
    {
        using namespace ffmpeg;

        // 1. prepare source slices and strides

        const auto srcSize = _src.size();

        auto srcRgbaImage = _src.convertToFormat(QImage::Format_ARGB32, Qt::ColorOnly);

        std::vector<const uint8_t*> srcSlices;
        srcSlices.reserve(srcRgbaImage.height());
        for (auto y = 0; y < srcRgbaImage.height(); ++y)
        {
            const auto scanline = static_cast<const uint8_t*>(srcRgbaImage.constScanLine(y));
            srcSlices.emplace_back(scanline);
        }

        std::vector<int> srcStrides(srcRgbaImage.height(), srcRgbaImage.bytesPerLine());

        // 2. prepare destination

        QImage dstRgbaImage(_dstSize, QImage::Format_ARGB32);

        std::vector<uint8_t*> dstSlices;
        dstSlices.reserve(static_cast<size_t>(dstRgbaImage.height()));
        for (auto y = 0; y < dstRgbaImage.height(); ++y)
        {
            const auto scanline = static_cast<uint8_t*>(dstRgbaImage.scanLine(y));
            dstSlices.emplace_back(scanline);
        }

        std::vector<int> dstStrides(dstRgbaImage.height(), dstRgbaImage.bytesPerLine());

        // 3. scale

        auto context = sws_getContext(
            srcSize.width(), srcSize.height(), AV_PIX_FMT_ARGB,
            _dstSize.width(), _dstSize.height(), AV_PIX_FMT_ARGB,
            SWS_BICUBIC, nullptr, nullptr, nullptr);

        sws_scale(context, srcSlices.data(), srcStrides.data(), 0, srcSize.height(), dstSlices.data(), dstStrides.data());

        sws_freeContext(context);

        return dstRgbaImage;
    }

}
