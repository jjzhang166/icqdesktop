#pragma once

namespace Logic
{
    struct TypingFires
    {
        QString aimId_;
        QString chatterAimId_;
        QString chatterName_;

        int counter_;

        TypingFires() : counter_(1) {}
        TypingFires(const QString& _aimid, const QString& _chatterAimid, const QString& _chatterName)
            : aimId_(_aimid), chatterAimId_(_chatterAimid), chatterName_(_chatterName), counter_(1)
        {
        }

        bool operator == (const TypingFires& _other) const
        {
            return (aimId_ == _other.aimId_ && chatterAimId_ == _other.chatterAimId_);
        }

        QString getChatterName() const;
    };
}
