#include "stdafx.h"

#include "IntValidator.h"

namespace Utils
{
    IntValidator::IntValidator(int min, int max, QObject *parent) :
        QValidator(parent)
      , mRexp("\\d*")
      , mMin(min)
      , mMax(max)
    {}

    IntValidator::State IntValidator::validate(QString &input, int &pos) const
    {
        if (input.isEmpty())
            return Acceptable;
        if (!mRexp.exactMatch(input))
            return Invalid;
        const int val = input.toInt();
        if (mMin <= val && val <= mMax)
            return Acceptable;
        return Invalid;
    }
}
