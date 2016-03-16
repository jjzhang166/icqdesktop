#include "stdafx.h"

#include "utils.h"

#include "Text.h"

namespace Utils
{
    int applyMultilineTextFix(const int32_t textHeight, const int32_t contentHeight)
    {
        assert(textHeight > 0);
        assert(contentHeight > 0);

        if (platform::is_windows())
        {
            const auto isMultiline = (textHeight >= Utils::scale_value(40));
            const auto fix = (
                isMultiline ?
                    Utils::scale_value(9) :
                    Utils::scale_value(-4)
            );

            return (contentHeight + fix);
        }

        if (platform::is_apple())
        {
            const auto isMultiline = (textHeight >= Utils::scale_value(36));
            const auto fix = (
                isMultiline ?
                    Utils::scale_value(10) :
                    Utils::scale_value(-5)
            );

            return (contentHeight + fix);
        }

        return contentHeight;
    }
}