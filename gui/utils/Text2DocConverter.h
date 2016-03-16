#pragma once

#include "../cache/emoji/Emoji.h"

namespace Ui
{
	class TextEditEx;
}

namespace Logic
{
	typedef std::function<void(const QString &uri, const int startPos)> Text2HtmlUriCallback;
	typedef std::map<QString, QString>	ResourceMap;

	enum class Text2DocHtmlMode
	{
		Invalid,
		Min,

		Escape = Min,
		Pass,

		Max = Pass
	};

	void FormatDocument(QTextDocument &doc, const int lineHeight);

	void Text4Edit(const QString& _text,
		Ui::TextEditEx& _edit,
		const Text2DocHtmlMode _htmlMode = Text2DocHtmlMode::Escape,
		const bool _convertLinks = true,
        const bool _breakDocument = false,
		const Text2HtmlUriCallback _uriCallback = nullptr,
		const Emoji::EmojiSizePx _emojiSize = Emoji::EmojiSizePx::Auto);

	void Text2Doc(
		const QString &text,
		QTextCursor &cursor,
		const Text2DocHtmlMode htmlMode = Text2DocHtmlMode::Escape,
		const bool convertLinks = true,
		const Text2HtmlUriCallback uriCallback = nullptr);

	ResourceMap InsertEmoji(int _main, int _ext, Ui::TextEditEx& _edit);
}