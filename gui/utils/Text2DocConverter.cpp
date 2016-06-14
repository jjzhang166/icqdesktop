#include "stdafx.h"

#include "Text2DocConverter.h"
#include "../cache/emoji/Emoji.h"
#include "../controls/TextEditEx.h"
#include "../utils/log/log.h"
#include "../utils/SChar.h"
#include "../utils/profiling/auto_stop_watch.h"

namespace
{
	using namespace Logic;

	class Text2DocConverter
	{
	public:
		Text2DocConverter();

		void Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

		const ResourceMap& InsertEmoji(const int32_t main, const int32_t ext, QTextCursor& cursor);

		void MakeUniqueResources(const bool make);

		const ResourceMap& GetResources() const;

        static void AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled);

	private:

		bool ConvertEmail();

		bool ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

		bool ConvertHttpFtp(bool breakDocument);

		bool ConvertNewline();

		void ConvertPlainCharacter(const bool isWordWrapEnabled);

		bool IsEos(const int offset = 0) const;

		bool IsHtmlEscapingEnabled() const;

		bool ExtractUrl(QString &url, const bool isWordWrapEnabled);

		void FlushBuffers();

		Utils::SChar PeekSChar();

		void PushInputCursor();

		void PopInputCursor(const int steps = 1);

		Utils::SChar ReadSChar();

		QString ReadString(const int length);

		void ReplaceEmoji(Utils::SChar ch, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment);

		void WriteAnchor();

		void WriteEmail();

		QTextStream Input_;

		QTextCursor Writer_;

		QString LastWord_;

        QString Buffer_;

		QString UrlAccum_;

		QString TmpBuf_;

		Text2HtmlUriCallback UriCallback_;

		std::vector<int> InputCursorStack_;

		Text2DocHtmlMode HtmlMode_;

		ResourceMap		Resources_;

		bool			MakeUniqResources_;
	};

	bool ExtractDomain(QTextStream &s, QString &domain);

	void ReplaceUrlSpec(const QString &url, QString &out);

	const QChar CH_KEYCAP(0x20E3);

    const QString SPACE_ENTITY("<span style='white-space: pre'>&#32;</span>");
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

	void Text4Edit(
        const QString& _text,
		Ui::TextEditEx& _edit,
		const Text2DocHtmlMode _htmlMode,
		const bool _convertLinks,
        const bool _breakDocument,
		const Text2HtmlUriCallback _uriCallback,
		const Emoji::EmojiSizePx _emojiSize,
        const QTextCharFormat::VerticalAlignment _aligment)
	{
        _edit.blockSignals(true);
        _edit.document()->blockSignals(true);
        _edit.setUpdatesEnabled(false);

		Text2DocConverter converter;
		converter.MakeUniqueResources(true);

        auto cursor = _edit.textCursor();
        cursor.beginEditBlock();

		converter.Convert(_text, cursor, _htmlMode, _convertLinks, _breakDocument, _uriCallback, _emojiSize, _aligment);

        cursor.endEditBlock();

		_edit.mergeResources(converter.GetResources());

        _edit.setUpdatesEnabled(true);
        _edit.document()->blockSignals(false);
        _edit.blockSignals(false);

        emit (_edit.document()->contentsChanged());
	}

	void Text2Doc(
        const QString &text,
		QTextCursor &cursor,
		const Text2DocHtmlMode htmlMode,
		const bool convertLinks,
		const Text2HtmlUriCallback uriCallback)
	{
		Text2DocConverter converter;
        cursor.document()->blockSignals(true);
		converter.Convert(text, cursor, htmlMode, convertLinks, false, uriCallback, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBaseline);
        cursor.document()->blockSignals(false);
        emit (cursor.document()->contentsChanged());
	}

	ResourceMap InsertEmoji(int _main, int _ext, Ui::TextEditEx& _edit)
	{
		Text2DocConverter converter;
		converter.MakeUniqueResources(true);
        QTextCursor cursor = _edit.textCursor();
		return converter.InsertEmoji(_main, _ext, cursor);
	}
}

namespace
{
    const auto WORD_WRAP_LIMIT = 15;

	Text2DocConverter::Text2DocConverter()
		: HtmlMode_(Text2DocHtmlMode::Pass)
		, MakeUniqResources_(false)
	{
		// allow downsizing without reallocation
		const auto DEFAULT_SIZE = 1024;
		UrlAccum_.reserve(DEFAULT_SIZE);
		TmpBuf_.reserve(DEFAULT_SIZE);
		LastWord_.reserve(DEFAULT_SIZE);
        Buffer_.reserve(DEFAULT_SIZE);

		// must be sufficient
		InputCursorStack_.reserve(32);
	}

	void Text2DocConverter::Convert(const QString &text, QTextCursor &cursor, const Text2DocHtmlMode htmlMode, const bool convertLinks, const bool breakDocument, const Text2HtmlUriCallback uriCallback, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
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
            if (ConvertEmoji(_emojiSize, _aligment))
			{
				continue;
			}
#endif //__APPLE__

			ConvertPlainCharacter(breakDocument);
		}

		FlushBuffers();

		InputCursorStack_.resize(0);
		TmpBuf_.resize(0);
		LastWord_.resize(0);
        Buffer_.resize(0);
		UrlAccum_.resize(0);
		UriCallback_ = nullptr;
	}

    void Text2DocConverter::AddSoftHyphenIfNeed(QString& output, const QString& word, bool isWordWrapEnabled)
    {
        const auto applyHyphen = (isWordWrapEnabled && (word.length() >= WORD_WRAP_LIMIT));
        if (applyHyphen)
        {
            output += QChar::SoftHyphen;
        }
    }

	const ResourceMap& Text2DocConverter::InsertEmoji(int _main, int _ext, QTextCursor& _cursor)
	{
		Writer_ = _cursor;
        Utils::SChar ch(_main, _ext);

        ReplaceEmoji(ch, Emoji::EmojiSizePx::Auto, QTextCharFormat::AlignBaseline);

		return Resources_;
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

		if (LastWord_.isEmpty())
		{
			return false;
		}

		PushInputCursor();

		ReadSChar();

		UrlAccum_.resize(0);
		UrlAccum_ += LastWord_;

		TmpBuf_.resize(0);
		if (!ExtractDomain(Input_, TmpBuf_))
		{
			PopInputCursor();
			return false;
		}

        Buffer_.resize(Buffer_.size() - LastWord_.size());
        LastWord_.resize(0);

		assert(!TmpBuf_.isEmpty());
		UrlAccum_ += '@';
		UrlAccum_ += TmpBuf_;

		WriteEmail();

		return true;
	}

	bool Text2DocConverter::ConvertEmoji(const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
	{
		const auto ch = PeekSChar();

		if (ch.IsEmoji())
		{
			ReadSChar();
			ReplaceEmoji(ch, _emojiSize, _aligment);
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

        static const QString STR_TTP("ttp");
		if (probablyHttp && rest.compare(STR_TTP, Qt::CaseInsensitive))
		{
			PopInputCursor();
			return false;
		}

        static const QString STR_TP("tp");
		if (probablyFtp && rest.compare(STR_TP, Qt::CaseInsensitive))
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

		WriteAnchor();

        Buffer_ += SPACE_ENTITY;
        LastWord_.resize(0);

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

        if (IsHtmlEscapingEnabled())
        {
            static const QString HTML_NEWLINE("<br/>");
		    Buffer_ += HTML_NEWLINE;
        }
        else
        {
            static const QString TEXT_NEWLINE("\n");
            Buffer_ += TEXT_NEWLINE;
        }

        LastWord_.resize(0);

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

	void Text2DocConverter::ConvertPlainCharacter(const bool isWordWrapEnabled)
	{
		const auto nextChar = ReadSChar();

        const auto nextCharStr = nextChar.ToQString();

        const auto isNextCharSpace = nextChar.IsSpace();
        
        const auto isEmoji = nextChar.IsEmoji();

        if (isNextCharSpace)
        {
            LastWord_.resize(0);
        }
        else
        {
            LastWord_ += nextCharStr;
            if (!isEmoji)
                AddSoftHyphenIfNeed(LastWord_, LastWord_, isWordWrapEnabled);
        }

		if (IsHtmlEscapingEnabled())
		{
			if (isNextCharSpace)
			{
                Buffer_ += SPACE_ENTITY;
			}
			else
			{
				const auto htmlEscaped = nextCharStr.toHtmlEscaped();
				Buffer_ += htmlEscaped;
			}
		}
		else
		{
            Buffer_ += nextCharStr;
		}

        if (!isEmoji)
            AddSoftHyphenIfNeed(Buffer_, LastWord_, isWordWrapEnabled);
	}

	bool Text2DocConverter::ExtractUrl(QString &url, const bool breakDocument)
	{
		PushInputCursor();

        static const QString SCHEME_TAIL("://");

		const auto schemeTail = ReadString(3);
		if (schemeTail != SCHEME_TAIL)
		{
			PopInputCursor();
			return false;
		}

		url += schemeTail;

        auto isWordWrapEnabled = breakDocument;
		for(;;)
		{
			const auto nextChar = PeekSChar();
			if (!nextChar.IsValidInUrl())
			{
				break;
			}

            if (nextChar.IsColon())
            {
                isWordWrapEnabled = false;
            }

			ReadSChar();

			const auto characterAfterNext = PeekSChar();

			if (!nextChar.IsValidOnUrlEnd() && !characterAfterNext.IsValidInUrl())
			{
				break;
			}

			url += nextChar.ToQString();

            AddSoftHyphenIfNeed(url, url, isWordWrapEnabled);
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

	void Text2DocConverter::FlushBuffers()
	{
        if (Buffer_.isEmpty())
        {
            return;
        }

		if (IsHtmlEscapingEnabled())
		{
			Writer_.insertHtml(Buffer_);
		}
		else
		{
			Writer_.insertText(Buffer_);
		}

		Buffer_.resize(0);
        LastWord_.resize(0);
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

    void Text2DocConverter::ReplaceEmoji(Utils::SChar ch, const Emoji::EmojiSizePx _emojiSize, const QTextCharFormat::VerticalAlignment _aligment)
	{
        const unsigned _main = ch.Main();
        const unsigned _ext = ch.Ext();

		assert(_main > 0);

        FlushBuffers();

        const auto charFormat = Writer_.charFormat();

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

		const QImage& img = Emoji::GetEmoji(_main, _ext, _emojiSize);

		QString emoji_code = make_from_code(_main);

		if (_ext)
			emoji_code += make_from_code(_ext);

		static int64_t uniq_index = 0;

		QString resource_name;
		if (!MakeUniqResources_)
		{
			resource_name = QString::number(img.cacheKey());
		}
		else
		{
			resource_name = emoji_code + "_" + QString::number(++uniq_index);
			Resources_[resource_name] = emoji_code;
		}

		Writer_.document()->addResource(
            QTextDocument::ImageResource,
            QUrl(resource_name),
            img
        );

		QTextImageFormat format;
		format.setName(resource_name);
		format.setVerticalAlignment(_aligment);
		Writer_.insertText(QString(QChar::ObjectReplacementCharacter), format);

        Writer_.setCharFormat(charFormat);
	}

	void Text2DocConverter::WriteAnchor()
	{
		assert(!UrlAccum_.isEmpty());

		TmpBuf_.resize(0);
		ReplaceUrlSpec(UrlAccum_, TmpBuf_);

		UrlAccum_.resize(0);
		QTextStream anchor(&UrlAccum_);
		anchor << "<a href=\"" << TmpBuf_ << "\">" << TmpBuf_ << "</a>";

		Buffer_ += UrlAccum_;
	}

	void Text2DocConverter::WriteEmail()
	{
		assert(!UrlAccum_.isEmpty());

		TmpBuf_.resize(0);
		ReplaceUrlSpec(UrlAccum_, TmpBuf_);

		UrlAccum_.resize(0);
		QTextStream anchor(&UrlAccum_);
		anchor << "<a type=\"email\" href=\"mailto:" << TmpBuf_ << "\">" << TmpBuf_ << "</a>";

		Buffer_ += UrlAccum_;
	}

	void Text2DocConverter::MakeUniqueResources(const bool _make)
	{
		MakeUniqResources_ = _make;
	}

	const ResourceMap& Text2DocConverter::GetResources() const
	{
		return Resources_;
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
            Text2DocConverter::AddSoftHyphenIfNeed(domain, domain, true);
		}

		return !domain.isEmpty() && domain.contains('.');
	}

	void ReplaceUrlSpec(const QString &url, QString &out)
	{
		assert(out.isEmpty());
		assert(!url.isEmpty());

        static const QString AMP_ENTITY("&amp;");

		for (const QChar ch : url)
		{
			switch (ch.unicode())
			{
			    case '&':
					out += AMP_ENTITY;
					break;

			    default:
					out += ch;
					break;
			}
		}
	}

}
