#pragma once

namespace Utils
{
    class IntValidator : public QValidator
    {
        Q_OBJECT
    public:
        IntValidator(int min, int max, QObject *parent = 0);

        State validate(QString &input, int &pos) const;

    private:
        QRegExp mRexp;
        int mMin;
        int mMax;
    };
}
