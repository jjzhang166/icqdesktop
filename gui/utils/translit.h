#pragma once

namespace Translit
{
    std::vector<QStringList> getPossibleStrings(const QString& text);
    int getMaxSearchTextLength();
}
