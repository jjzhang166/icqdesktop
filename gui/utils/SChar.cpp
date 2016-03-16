#include "stdafx.h"

#include "SChar.h"

namespace
{
	const QChar CH_KEYCAP(0x20E3);

	bool IsSingleCharacterEmoji(const uint32_t main);

	bool IsTwoCharacterEmoji(const uint32_t main, const uint32_t ext);

	uint32_t ReadNextCodepoint(QTextStream &s);
}

namespace Utils
{

	const SChar SChar::Null(0, 0);

	SChar::SChar(const uint32_t base, const uint32_t ext)
		: Main_(base)
		, Ext_(ext)
	{
		assert(!HasExt() || HasMain());
	}

	bool SChar::EqualTo(const char ch) const
	{
		return (!HasExt() && (Main_ == ch));
	}

	bool SChar::EqualToI(const char ch) const
	{
		return EqualTo(QChar(ch).toLower().unicode());
	}

	uint32_t SChar::Ext() const
	{
		return Ext_;
	}

	bool SChar::HasMain() const
	{
		return (Main_ > 0);
	}

	bool SChar::HasExt() const
	{
		return (Ext_ > 0);
	}

	bool SChar::IsCarriageReturn() const
	{
		return (!HasExt() && (Main_ == '\r'));
	}

	bool SChar::IsComplex() const
	{
		return (LengthQChars() > 1);
	}

	bool SChar::IsEmailCharacter() const
	{
		if (HasExt())
		{
			return false;
		}

		return (
			QChar::isLetterOrNumber(Main_) ||
			(Main_ == '_') ||
			(Main_ == '.') ||
			(Main_ == '-')
		);
	}

	bool SChar::IsEmoji() const
	{
		return (IsTwoCharacterEmoji() || IsSingleCharacterEmoji());
	}

	bool SChar::IsSimple() const
	{
		return (
            (LengthQChars() == 1) && 
            !IsSingleCharacterEmoji()
        );
	}

	bool SChar::IsSingleCharacterEmoji() const
	{
		return ::IsSingleCharacterEmoji(Main_);
	}

	bool SChar::IsSpace() const
	{
		return (IsSimple() && QChar::isSpace(Main_));
	}

	bool SChar::IsTwoCharacterEmoji() const
	{
		return ::IsTwoCharacterEmoji(Main_, Ext_);
	}

	QString::size_type SChar::LengthQChars() const
	{
		if (IsNull())
		{
			return 0;
		}

		QString::size_type length = 0;

		if (QChar::requiresSurrogates(Main_))
		{
			length += 2;
		}
		else
		{
			length += 1;
		}

		if (!HasExt())
		{
			return length;
		}

		if (QChar::requiresSurrogates(Ext_))
		{
			length += 2;
		}
		else
		{
			length += 1;
		}

		return length;
	}

	uint32_t SChar::Main() const
	{
		return Main_;
	}

	QChar SChar::ToQChar() const
	{
		if (LengthQChars() > 1)
		{
			assert(!"conversion not possible");
			return QChar();
		}

		return QChar(Main_);
	}

	QString SChar::ToQString() const
	{
		QString result;

		const auto maxSize = 4;
		result.reserve(maxSize);

		if (IsNull())
		{
			return result;
		}

		if (QChar::requiresSurrogates(Main_))
		{
			result += QChar(QChar::highSurrogate(Main_));
			result += QChar(QChar::lowSurrogate(Main_));
		}
		else
		{
			result += QChar(Main_);
		}

		if (!HasExt())
		{
			return result;
		}

		if (QChar::requiresSurrogates(Ext_))
		{
			result += QChar(QChar::highSurrogate(Ext_));
			result += QChar(QChar::lowSurrogate(Ext_));
		}
		else
		{
			result += QChar(Ext_);
		}

		return result;
	}

	bool SChar::IsNewline() const
	{
		return (!HasExt() && (Main_ == '\n'));
	}

	bool SChar::IsNull() const
	{
		return (!HasExt() && !HasMain());
	}

	bool SChar::IsValidInUrl() const
	{
		if (IsComplex() || IsNull())
		{
			return false;
		}

		static QString invalidChars(" \r\n\t$<>^\\{}|\"");
		return !invalidChars.contains(ToQChar());
	}

    bool SChar::IsColon() const
    {
        return (!HasExt() && (Main_ == ':'));
    }

	bool SChar::IsValidOnUrlEnd() const
	{
		if (IsComplex() || IsNull())
		{
			return false;
		}

		static QString invalidChars(" \r\n\t;,'");
		return !invalidChars.contains(ToQChar());
	}

	SChar ReadNextSuperChar(QTextStream &s)
	{
		const auto main = ReadNextCodepoint(s);
		if (main == 0)
		{
			return SChar::Null;
		}

		const auto pos = s.pos();

		const auto ext = ReadNextCodepoint(s);
		if (ext == 0)
		{
			s.seek(pos);
			return SChar(main, 0);
		}

		if (IsTwoCharacterEmoji(main, ext))
		{
			return SChar(main, ext);
		}

		s.seek(pos);
		return SChar(main, 0);
	}

	SChar PeekNextSuperChar(QTextStream &s)
	{
		const auto pos = s.pos();
		const auto ch = ReadNextSuperChar(s);
		s.seek(pos);

		return ch;
	}

	SChar PeekNextSuperChar(const QString &s, const QString::size_type offset)
	{
		assert(offset < s.length());

		QTextStream input((QString*)&s, QIODevice::ReadOnly);

		const auto success = input.seek(offset);
        if (!success)
        {
            assert(success);
        }

		return ReadNextSuperChar(input);
	}

}

namespace
{
	bool IsSingleCharacterEmoji(const uint32_t main)
	{
		const auto isEmoji = (
			(main == 0x00a9) || (main == 0x00ae) || (main == 0x203c) || (main == 0x2049)
			|| ((main >= 0x1f004) && (main <= 0x1f9c0))
			|| ((main >= 0x2122) && (main <= 0x21aa)) || ((main >= 0x231a) && (main <= 0x27bf))
			|| ((main >= 0x2934) && (main <= 0x2935)) || ((main >= 0x2b05) && (main <= 0x2b55)) || ((main >= 0x3030) && (main <= 0x3299))
		);
		if (isEmoji)
		{
			return true;
		}

		return false;
	}

	bool IsTwoCharacterEmoji(const uint32_t main, const uint32_t ext)
	{
		if (ext == CH_KEYCAP)
		{
			const auto isNumber = ((main >= '0') || (main <= '9'));
			const auto isSharp = (main == '#');

			if (isNumber || isSharp)
			{
				return true;
			}
		}
		const auto isEyeEmoji = ((main == 0x1f441) && (ext == 0x1f5e8));
		if (isEyeEmoji)
		{
			return true;
		}
		const auto FLAG_LOW = 0x1f1e6;
		const auto FLAG_HIGH = 0x1f1ff;
		const auto isFlag = ((main >= FLAG_LOW) && (main <= FLAG_HIGH) && (ext >= FLAG_LOW) && (ext <= FLAG_HIGH));
		return isFlag;
	}

	uint32_t ReadNextCodepoint(QTextStream &s)
	{
		if (s.atEnd())
		{
			return 0;
		}

		QChar high;
		s >> high;

		if (s.atEnd() || !high.isHighSurrogate())
		{
			return high.unicode();
		}

		const auto pos = s.pos();

		QChar low;
		s >> low;

		if (!low.isLowSurrogate())
		{
			const auto seekSucceed = s.seek(pos);
            if (!seekSucceed)
            {
			    assert(seekSucceed);
            }

			return high.unicode();
		}

		return QChar::surrogateToUcs4(high, low);
	}
}
