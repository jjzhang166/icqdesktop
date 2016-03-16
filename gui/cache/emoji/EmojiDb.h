#pragma once

namespace Emoji
{
	struct EmojiRecord
	{
		EmojiRecord(const QString &category, const int index, const unsigned codepoint, const unsigned extendedCodepoint, const QString &name);

		const QString Category_;

		const int Index_;

		const QString Name_;

		const unsigned Codepoint_;

		const unsigned ExtendedCodepoint_;		
	};

	typedef std::shared_ptr<EmojiRecord> EmojiRecordSptr;

	typedef std::vector<EmojiRecordSptr> EmojiRecordSptrVec;

	void InitEmojiDb();

	const EmojiRecordSptr& GetEmojiInfoByCodepoint(const uint32_t codepoint, const uint32_t extendedCodepoint);

	const QStringList& GetEmojiCategories();

	const EmojiRecordSptrVec& GetEmojiInfoByCategory(const QString &category);
}