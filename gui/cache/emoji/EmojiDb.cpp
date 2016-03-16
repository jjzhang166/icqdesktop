#include "stdafx.h"

#include "EmojiDb.h"

namespace
{
	using namespace Emoji;

	std::vector<EmojiRecordSptr> EmojiIndexByOrder_;

	std::unordered_map<uint64_t, EmojiRecordSptr> EmojiIndexByCodepoint_;

	std::map<QString, EmojiRecordSptrVec> EmojiIndexByCategory_;

	QStringList EmojiCategories_;

	uint64_t MakeComplexCodepoint(const uint32_t codepoint, const uint32_t extendedCodepoint)
	{
		return ((uint64_t)codepoint) | ((uint64_t)extendedCodepoint << 32);
	}
}

namespace Emoji
{
	EmojiRecord::EmojiRecord(const QString &category, const int index, const unsigned codepoint, const unsigned extendedCodepoint, const QString &name)
		: Category_(category)
		, Index_(index)
		, Name_(name)
		, Codepoint_(codepoint)
		, ExtendedCodepoint_(extendedCodepoint)
	{
		assert(!Category_.isEmpty());
		assert(!Name_.isEmpty());
		assert(Index_ >= 0);
		assert(Codepoint_ > 0);
	}

	void InitEmojiDb()
	{
		#include "EmojiIndexData.cpp"
		assert(!EmojiIndexByOrder_.empty());

		for (const auto &record : EmojiIndexByOrder_)
		{
			const auto &category = record->Category_;
			assert(!category.isEmpty());

			if (!EmojiCategories_.contains(category))
			{
				EmojiCategories_.append(category);
			}

			const auto extendedCodepoint = MakeComplexCodepoint(record->Codepoint_, record->ExtendedCodepoint_);
			EmojiIndexByCodepoint_.emplace(extendedCodepoint, record);

			auto iter = EmojiIndexByCategory_.find(category);
			if (iter == EmojiIndexByCategory_.end())
			{
				EmojiRecordSptrVec v;
				v.reserve(EmojiIndexByCodepoint_.size());
				iter = EmojiIndexByCategory_.emplace(category, std::move(v)).first;
			}

			iter->second.push_back(record);
		}
	}

	const EmojiRecordSptr& GetEmojiInfoByCodepoint(const uint32_t codepoint, const uint32_t extendedCodepoint)
	{
		assert(codepoint > 0);
		assert(!EmojiIndexByCodepoint_.empty());

		static const EmojiRecordSptr empty;

		const auto complexCodepoint = MakeComplexCodepoint(codepoint, extendedCodepoint);
		const auto iter = EmojiIndexByCodepoint_.find(complexCodepoint);
		if (iter == EmojiIndexByCodepoint_.end())
		{
			return empty;
		}

		return iter->second;
	}

	const QStringList& GetEmojiCategories()
	{
		assert(!EmojiCategories_.isEmpty());
		return EmojiCategories_;
	}

	const EmojiRecordSptrVec& GetEmojiInfoByCategory(const QString &category)
	{
		assert(!category.isEmpty());

		static const EmojiRecordSptrVec empty;

		const auto iter = EmojiIndexByCategory_.find(category);
		if (iter == EmojiIndexByCategory_.end())
		{
			return empty;
		}

		return iter->second;
	}
}
