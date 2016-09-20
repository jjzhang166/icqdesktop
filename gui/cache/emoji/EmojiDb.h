#pragma once

namespace Emoji
{
	struct EmojiRecord
	{
		EmojiRecord(const QString& _category, const int _index, const unsigned _codepoint, const unsigned _extendedCodepoint, const QString& _name);

		const QString Category_;

		const int Index_;

		const QString Name_;

		const unsigned Codepoint_;

		const unsigned ExtendedCodepoint_;		
	};

	typedef std::shared_ptr<EmojiRecord> EmojiRecordSptr;

	typedef std::vector<EmojiRecordSptr> EmojiRecordSptrVec;

	void InitEmojiDb();

    
    static const EmojiRecordSptr EmptyEmoji;
	const EmojiRecordSptr& GetEmojiInfoByCodepoint(const uint32_t _codepoint, const uint32_t _extendedCodepoint);

	const QStringList& GetEmojiCategories();

	const EmojiRecordSptrVec& GetEmojiInfoByCategory(const QString& _category);
}