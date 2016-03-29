#include "stdafx.h"

#include "Text2DocConverter.h"
#include "../cache/emoji/Emoji.h"
#include "../controls/TextEditEx.h"
#include "../utils/SChar.h"
#include "../utils/profiling/auto_stop_watch.h"

namespace
{
	using namespace Logic;

	class Text2DocConverter
	{
	public:
		Text2DocConverter();

		void Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize);
		ResourceMap InsertEmoji(int _main, int _ext, QTextCursor& _cursor);

		void make_uniq_resources(bool _make);
		const ResourceMap& get_resources() const;

	private:

		bool ConvertEmail();

		bool ConvertEmoji(const Emoji::EmojiSizePx _emojiSize);

		bool ConvertHttpFtp(bool breakDocument);

		bool ConvertNewline();

		void ConvertPlainCharacter(bool brealDocument);

		bool IsEos(const int offset = 0) const;

		bool IsHtmlEscapingEnabled() const;

		bool ExtractUrl(QString &url, bool breakDocument);

		void FlushLastWord();

		Utils::SChar PeekSChar();

		void PushInputCursor();

		void PopInputCursor(const int steps = 1);

		Utils::SChar ReadSChar();

		QString ReadString(const int length);

		void ReplaceEmoji(Utils::SChar ch, const Emoji::EmojiSizePx _emojiSize);

		void WriteAnchor();

		void WriteEmail();

		QTextStream Input_;

		QTextCursor Writer_;

		QString LastWordAccum_;

		QString UrlAccum_;

		QString TmpBuf_;

		Text2HtmlUriCallback UriCallback_;

		std::vector<int> InputCursorStack_;

		Text2DocHtmlMode HtmlMode_;

		ResourceMap		resources_;
		bool			make_uniq_resources_;
	};

//	bool IsEmailCharacter(const QChar ch);
//
//	bool IsOneCharacterEmoji(const unsigned main, const unsigned ext);
//
//	bool IsTwoCharacterEmoji(const unsigned main, const unsigned ext);

	bool ExtractDomain(QTextStream &s, QString &domain);

//	bool ExtractUrl(const QString &text, int &cursorPos, QString &url);

	void ReplaceUrlSpec(const QString &url, QString &out);

	const QChar CH_KEYCAP(0x20E3);
}

namespace Logic
{
	void FormatDocument(QTextDocument &doc, const int lineHeight)
	{
		assert(lineHeight > 0);

        if (platform::is_apple())
        {
            doc.setDocumentMargin(0);

            return;
        }

		for (auto block = doc.firstBlock(); block.isValid(); block = block.next())
		{
			QTextCursor c(block);
			auto f = c.blockFormat();
			f.setLineHeight(lineHeight, QTextBlockFormat::FixedHeight);
			c.setBlockFormat(f);
		}

		doc.setDocumentMargin(0);
	}

	void Text4Edit(const QString& _text,
		Ui::TextEditEx& _edit,
		const Text2DocHtmlMode _htmlMode,
		const bool _convertLinks,
        const bool _breakDocument,
		const Text2HtmlUriCallback _uriCallback,
		const Emoji::EmojiSizePx _emojiSize)
	{
        _edit.document()->blockSignals(true);
		Text2DocConverter converter;
		converter.make_uniq_resources(true);
        QTextCursor cursor = _edit.textCursor();
		converter.Convert(_text, cursor, _htmlMode, _convertLinks, _breakDocument, _uriCallback, _emojiSize);
		_edit.merge_resources(converter.get_resources());
        _edit.document()->blockSignals(false);
        emit (_edit.document()->contentsChanged());
	}

	void Text2Doc(const QString &text,
		QTextCursor &cursor,
		const Text2DocHtmlMode htmlMode,
		const bool convertLinks,
		const Text2HtmlUriCallback uriCallback)
	{
		Text2DocConverter converter;
        cursor.document()->blockSignals(true);
		converter.Convert(text, cursor, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto);
        cursor.document()->blockSignals(false);
        emit (cursor.document()->contentsChanged());
	}

	ResourceMap InsertEmoji(int _main, int _ext, Ui::TextEditEx& _edit)
	{
		Text2DocConverter converter;
		converter.make_uniq_resources(true);
        QTextCursor cursor = _edit.textCursor();
		return converter.InsertEmoji(_main, _ext, cursor);
	}
}

namespace
{
    const int max_word_length = 15;

	Text2DocConverter::Text2DocConverter()
		: HtmlMode_(Text2DocHtmlMode::Pass)
		, make_uniq_resources_(false)
	{
		// allow downsizing without reallocation
		const auto defaultSize = 1024;
		UrlAccum_.reserve(defaultSize);
		TmpBuf_.reserve(defaultSize);
		LastWordAccum_.reserve(defaultSize);

		// must be sufficient
		InputCursorStack_.reserve(32);
	}

	void Text2DocConverter::Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize)
	{
		assert(!UriCallback_);
		assert(htmlMode >= Text2DocHtmlMode::Min);
		assert(htmlMode <= Text2DocHtmlMode::Max);

		Input_.setString((QString*)&text, QIODevice::ReadOnly);
		Writer_ = cursor;
		UriCallback_ = uriCallback;
		HtmlMode_ = htmlMode;

		while (!IsEos())
		{
			InputCursorStack_.resize(0);

			if (ConvertNewline())
			{
				continue;
			}

			if (convertLinks && ConvertHttpFtp(breakDocument))
			{
				continue;
			}

			if (convertLinks && ConvertEmail())
			{
				continue;
			}

#ifndef __APPLE__
			if (ConvertEmoji(_emojiSize))
			{
				continue;
			}
#endif

			ConvertPlainCharacter(breakDocument);
		}

		FlushLastWord();

		InputCursorStack_.resize(0);
		TmpBuf_.resize(0);
		LastWordAccum_.resize(0);
		UrlAccum_.resize(0);
		UriCallback_ = nullptr;
	}


	ResourceMap Text2DocConverter::InsertEmoji(int _main, int _ext, QTextCursor& _cursor)
	{
		Writer_ = _cursor;
        Utils::SChar ch(_main, _ext);

		ReplaceEmoji(ch, Emoji::EmojiSizePx::Auto);

		return resources_;
	}

	bool Text2DocConverter::ConvertEmail()
	{
		if (!IsHtmlEscapingEnabled())
		{
			return false;
		}

		if (!PeekSChar().EqualTo('@'))
		{
			return false;
		}

		if (LastWordAccum_.isEmpty())
		{
			return false;
		}

		PushInputCursor();

		ReadSChar();

		UrlAccum_.resize(0);
		UrlAccum_ += LastWordAccum_;

		TmpBuf_.resize(0);
		if (!ExtractDomain(Input_, TmpBuf_))
		{
			PopInputCursor();
			return false;
		}

		LastWordAccum_.resize(0);

		assert(!TmpBuf_.isEmpty());
		UrlAccum_ += '@';
		UrlAccum_ += TmpBuf_;

		WriteEmail();

		return true;
	}

	bool Text2DocConverter::ConvertEmoji(const Emoji::EmojiSizePx _emojiSize)
	{
		const auto ch = PeekSChar();

		if (ch.IsEmoji())
		{
			ReadSChar();
			ReplaceEmoji(ch, _emojiSize);
			return true;
		}

		return false;
	}

	bool Text2DocConverter::ConvertHttpFtp(bool breakDocument)
	{
		if (!IsHtmlEscapingEnabled())
		{
			return false;
		}

		PushInputCursor();

		const auto firstSchemeChar = PeekSChar();

		const auto probablyHttp = firstSchemeChar.EqualToI('h');
		const auto probablyFtp = firstSchemeChar.EqualToI('f');

		if (!probablyHttp && !probablyFtp)
		{
			return false;
		}

		// [h]ttp://x or [f]tp://x
		const auto minLength = (probablyHttp ? 8 : 7);
		if (IsEos(minLength))
		{
			return false;
		}

		ReadSChar();

		const auto restLength = (probablyHttp ? 3 : 2);
		const auto rest = ReadString(restLength);

		if (probablyHttp && rest.compare("ttp", Qt::CaseInsensitive))
		{
			PopInputCursor();
			return false;
		}

		if (probablyFtp && rest.compare("tp", Qt::CaseInsensitive))
		{
			PopInputCursor();
			return false;
		}

		UrlAccum_.resize(0);
		UrlAccum_ += firstSchemeChar.ToQChar();
		UrlAccum_ += rest;

		const auto lastSchemeChar = PeekSChar();
		if (lastSchemeChar.EqualToI('s'))
		{
			ReadSChar();
			UrlAccum_ += lastSchemeChar.ToQString();
		}

		if (!ExtractUrl(UrlAccum_, breakDocument))
		{
			PopInputCursor();
			return false;
		}

		FlushLastWord();
		WriteAnchor();

        LastWordAccum_ += "<span style='white-space: pre'>&#32;</span>";//"<span>&nbsp;</span>";
        FlushLastWord();

		assert(!UrlAccum_.isEmpty());
		if (UriCallback_)
		{
			UriCallback_(UrlAccum_, Writer_.position());
		}

		return true;
	}

	bool Text2DocConverter::ConvertNewline()
	{
		const auto ch = PeekSChar();

		const auto isNewline = (ch.IsNewline() || ch.IsCarriageReturn());
		if (!isNewline)
		{
			return false;
		}

		ReadSChar();

		FlushLastWord();

		Writer_.insertText(QChar('\n'));

		if (!ch.IsCarriageReturn())
		{
			return true;
		}

		const auto hasTrailingNewline = PeekSChar().IsNewline();
		if (hasTrailingNewline)
		{
			// eat a newline after the \r because we had been processed it already
			ReadSChar();
		}

		return true;
	}

	void Text2DocConverter::ConvertPlainCharacter(bool breakDocument)
	{
		const auto nextChar = ReadSChar();

		if (IsHtmlEscapingEnabled())
		{
			if (nextChar.IsSpace())
			{
                static const QString spaceEntity("<span style='white-space: pre'>&#32;</span>");
                LastWordAccum_ += spaceEntity;
                FlushLastWord();
			}
			else
			{
				const auto htmlEscaped = nextChar.ToQString().toHtmlEscaped();
				LastWordAccum_ += htmlEscaped;
			}
		}
		else
		{
			LastWordAccum_ += nextChar.ToQString();
		}

        if (breakDocument && LastWordAccum_.length() >= max_word_length)
            LastWordAccum_ += QChar::SoftHyphen;
	}

	bool Text2DocConverter::ExtractUrl(QString &url, bool breakDocument)
	{
		PushInputCursor();

		const auto schemeTail = ReadString(3);
		if (schemeTail != "://")
		{
			PopInputCursor();
			return false;
		}

		url += schemeTail;

        bool needSoftHyphen = true;
        int skipChars = 0;
		for(;;)
		{
			const auto nextChar = PeekSChar();
			if (!nextChar.IsValidInUrl())
			{
				break;
			}

            if (nextChar.IsColon())
            {
                needSoftHyphen = false;
            }

			ReadSChar();

			const auto characterAfterNext = PeekSChar();

			if (!nextChar.IsValidOnUrlEnd() && !characterAfterNext.IsValidInUrl())
			{
				break;
			}

			url += nextChar.ToQString();
            if (breakDocument && url.length() >= max_word_length && needSoftHyphen)
            {
                url += QChar::SoftHyphen;
            }

            if (!needSoftHyphen && ++skipChars >= max_word_length)
            {
                needSoftHyphen = true;
                skipChars = 0;
            }
		}

		const auto isLongEnough = (url.length() > 8);
        if (!isLongEnough)
        {
            PopInputCursor();
        }

		return isLongEnough;
	}

	bool Text2DocConverter::IsEos(const int offset) const
	{
		assert(offset >= 0);
		assert(Input_.string());

		const auto &text = *Input_.string();
		return ((Input_.pos() + offset) >= text.length());
	}

	bool Text2DocConverter::IsHtmlEscapingEnabled() const
	{
        assert(HtmlMode_ >= Text2DocHtmlMode::Min);
		assert(HtmlMode_ <= Text2DocHtmlMode::Max);

		return (HtmlMode_ == Text2DocHtmlMode::Escape);
	}

	void Text2DocConverter::FlushLastWord()
	{
		if (IsHtmlEscapingEnabled())
		{
			Writer_.insertHtml(LastWordAccum_);
		}
		else
		{
			Writer_.insertText(LastWordAccum_);
		}

		LastWordAccum_.resize(0);
	}

	Utils::SChar Text2DocConverter::PeekSChar()
	{
		return Utils::PeekNextSuperChar(Input_);
	}

	void Text2DocConverter::PushInputCursor()
	{
		InputCursorStack_.push_back((int)Input_.pos());
		assert(InputCursorStack_.size() < InputCursorStack_.capacity());
	}

	void Text2DocConverter::PopInputCursor(const int steps)
	{
		assert(!InputCursorStack_.empty());
		assert(steps > 0);
		assert(steps < 100);

		const auto newSize = (InputCursorStack_.size() - (unsigned)steps);
		const auto storedPos = InputCursorStack_[newSize];
		InputCursorStack_.resize(newSize);

		const auto success = Input_.seek(storedPos);
		assert(success);
        (void)success;
	}

	Utils::SChar Text2DocConverter::ReadSChar()
	{
		return Utils::ReadNextSuperChar(Input_);
	}

	QString Text2DocConverter::ReadString(const int length)
	{
		return Input_.read(length);
	}

    void Text2DocConverter::ReplaceEmoji(Utils::SChar ch, const Emoji::EmojiSizePx _emojiSize)
	{
#ifndef __APPLE__
        const unsigned _main = ch.Main();
        const unsigned _ext = ch.Ext();

		assert(_main > 0);

        const auto charFormat = Writer_.charFormat();

		Writer_.beginEditBlock();

		auto make_from_code = [](int _code)->QString
		{
			QString result;

			if (QChar::requiresSurrogates(_code))
			{
				QChar high = QChar::highSurrogate(_code);
				QChar low = QChar::lowSurrogate(_code);

				result += high;
				result += low;
			}
			else
			{
				result += (QChar) _code;
			}

			return result;
		};

		FlushLastWord();

		const QImage& img = Emoji::GetEmoji(_main, _ext, _emojiSize);

		QString emoji_code = make_from_code(_main);

		if (_ext)
			emoji_code += make_from_code(_ext);

		static int64_t uniq_index = 0;

		QString resource_name;
		if (!make_uniq_resources_)
		{
			resource_name = QString::number(img.cacheKey());
		}
		else
		{
			resource_name = emoji_code + "_" + QString::number(++uniq_index);
			resources_[resource_name] = emoji_code;
		}

		Writer_.document()->addResource(
            QTextDocument::ImageResource,
            QUrl(resource_name),
            img
        );

		QTextImageFormat format;
		format.setName(resource_name);
		format.setVerticalAlignment(QTextCharFormat::AlignBaseline);
		Writer_.insertText(QString(QChar::ObjectReplacementCharacter), format);

		Writer_.endEditBlock();

        Writer_.setCharFormat(charFormat);

#else
        Writer_.insertText(ch.ToQString());//, format);
#endif
	}

	void Text2DocConverter::WriteAnchor()
	{
		assert(!UrlAccum_.isEmpty());

		TmpBuf_.resize(0);
		ReplaceUrlSpec(UrlAccum_, TmpBuf_);

		UrlAccum_.resize(0);
		QTextStream anchor(&UrlAccum_);
		anchor << "<a href=\"" << TmpBuf_ << "\">" << TmpBuf_ << "</a>";

		Writer_.insertHtml(UrlAccum_);
	}

	void Text2DocConverter::WriteEmail()
	{
		assert(!UrlAccum_.isEmpty());

		TmpBuf_.resize(0);
		ReplaceUrlSpec(UrlAccum_, TmpBuf_);

		UrlAccum_.resize(0);
		QTextStream anchor(&UrlAccum_);
		anchor << "<a type=\"email\" href=\"mailto:" << TmpBuf_ << "\">" << TmpBuf_ << "</a>";

		Writer_.insertHtml(UrlAccum_);
	}

	void Text2DocConverter::make_uniq_resources(bool _make)
	{
		make_uniq_resources_ = _make;
	}

	const ResourceMap& Text2DocConverter::get_resources() const
	{
		return resources_;
	}
}

namespace
{
	bool ExtractDomain(QTextStream &s, QString &domain)
	{
		assert(domain.isEmpty());

		for (;;)
		{
			const auto pos = s.pos();

			const auto ch = Utils::ReadNextSuperChar(s);

			if (!ch.IsEmailCharacter())
			{
				const auto seekSucceed = s.seek(pos);
				assert(seekSucceed);
				(void)seekSucceed;
				break;
			}

			domain += ch.ToQString();
		}

		return !domain.isEmpty();
	}

	void ReplaceUrlSpec(const QString &url, QString &out)
	{
		assert(out.isEmpty());
		assert(!url.isEmpty());

		for (const auto ch : url)
		{
			switch (ch.unicode())
			{
			case '&':
				{
					out += "&amp;";
					break;
				}

			default:
				{
					out += ch;
					break;
				}
			}
		}
	}

}
