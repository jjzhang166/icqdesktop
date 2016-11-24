#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"


#include "../KnownFileTypes.h"

#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"
#include "FileSharingUtils.h"
#include "ImagePreviewBlock.h"
#include "LinkPreviewBlock.h"
#include "LinkPreviewBlockLayout.h"
#include "PttBlock.h"
#include "TextBlock.h"
#include "QuoteBlock.h"
#include "YoutubeLinkPreviewBlockLayout.h"
#include "StickerBlock.h"

#include "../../../utils/UrlParser.h"

#include "ComplexMessageItemBuilder.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    enum class ChunkType
    {
        Invalid,

        Min,

        Undefined,
        Text,
        GenericLink,
        ImageLink,
        Junk,
        FileSharingImage,
        FileSharingGif,
        FileSharingVideo,
        FileSharingImageSnap,
        FileSharingGifSnap,
        FileSharingVideoSnap,
        FileSharingPtt,
        FileSharingGeneral,
        Sticker,

        Max
    };

    struct TextChunk
    {
        static const TextChunk Empty;

        TextChunk(const ChunkType type, const int32_t beginIndex, const int32_t endIndex, const QString &imageType, const int32_t durationSec);

        int32_t length() const;

        TextChunk mergeWith(const TextChunk &chunk) const;

        ChunkType Type_;

        int32_t BeginIndex_;

        int32_t EndIndex_;

        QString ImageType_;

        int32_t DurationSec_;

        Data::Quote Quote_;

        HistoryControl::StickerInfoSptr Sticker_;
    };

    typedef QVector<QStringRef> QStringRefVec;

    TextChunk findNextChunk(const QString &text, const int32_t beginIndex);
}

namespace ComplexMessageItemBuilder
{
    ComplexMessageItem* makeComplexItem(
        QWidget *parent,
        const int64_t id,
        const QDate date,
        const int64_t prev,
        const QString &text,
        const QString &chatAimid,
        const QString &senderAimid,
        const QString &senderFriendly,
        const QList<Data::Quote>& quotes,
        HistoryControl::StickerInfoSptr sticker,
        const bool isOutgoing)
    {
        assert(id >= -1);
        assert(!senderAimid.isEmpty());
        assert(!senderFriendly.isEmpty());
        assert(date.isValid());

        std::unique_ptr<ComplexMessage::ComplexMessageItem> complexItem(
            new ComplexMessage::ComplexMessageItem(
                parent,
                id,
                date,
                chatAimid,
                senderAimid,
                senderFriendly,
                sticker ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : text,
                isOutgoing));

        std::list<TextChunk> chunks;

        int i = 0;
        for (auto quote : quotes)
        {
            ++i;
            quote.id_ = i;
            auto text = quote.text_;

            if (quote.isSticker())
            {
                TextChunk chunk(ChunkType::Sticker, 0, text.length(), QString(), -1);
                chunk.Sticker_ = HistoryControl::StickerInfo::Make(quote.setId_, quote.stickerId_);
                chunk.Quote_ = quote;
                chunk.Quote_.isFirstQuote_ = (i == 1);
                chunk.Quote_.isLastQuote_ = (i == quotes.size());
                chunks.emplace_back(std::move(chunk));
                continue;
            }

            if (text.startsWith(">"))
            {
                TextChunk chunk(ChunkType::Text, 0, text.length(), QString(), -1);
                chunk.Quote_ = quote;
                chunk.Quote_.isFirstQuote_ = (i == 1);
                chunk.Quote_.isLastQuote_ = (i == quotes.size());
                chunks.emplace_back(std::move(chunk));
                continue;
            }

            for (auto beginIndex = 0; beginIndex < text.length(); )
            {
                auto chunk = findNextChunk(text, beginIndex);
                chunk.Quote_ = quote;
                chunk.Quote_.isFirstQuote_ = (i == 1);
                chunk.Quote_.isLastQuote_ = (i == quotes.size());
                chunks.emplace_back(std::move(chunk));
                beginIndex = chunk.EndIndex_;
            }
        }


        if (sticker)
        {
            TextChunk chunk(ChunkType::Sticker, 0, text.length(), QString(), -1);
            chunk.Sticker_ = sticker;
            chunks.emplace_back(std::move(chunk));
        }
        else
        {
            for (auto beginIndex = 0; beginIndex < text.length(); )
            {
                auto chunk = findNextChunk(text, beginIndex);

                chunks.emplace_back(std::move(chunk));

                beginIndex = chunk.EndIndex_;
            }
        }

        for (auto iter = chunks.begin(); iter != chunks.end(); )
        {
            auto next = iter;
            ++next;

            if (next == chunks.end())
            {
                break;
            }

            if (iter->Quote_.isEmpty())
            {
                auto merged = iter->mergeWith(*next);

                if (merged.Type_ != ChunkType::Undefined)
                {
                    chunks.erase(iter);
                    *next = std::move(merged);
                }
            }
            iter = next;
        }

        IItemBlocksVec items;
        items.reserve(chunks.size());

        QuoteBlock* prevQuote = 0;
        std::vector<QuoteBlock*> quoteBlocks;

        int count = 0;
        for (const auto &chunk : chunks)
        {
            ++count;
            const auto chunkLength = (chunk.EndIndex_ - chunk.BeginIndex_);
            assert(chunkLength > 0 || chunk.Sticker_);

            const auto chunkText = !chunk.Quote_.isEmpty() ? chunk.Quote_.text_.mid(chunk.BeginIndex_, chunkLength) : text.mid(chunk.BeginIndex_, chunkLength);

            GenericBlock *block = nullptr;
            switch(chunk.Type_)
            {
                case ChunkType::Text:
                    block = new TextBlock(
                                complexItem.get(),
                                chunkText);
                    break;

                case ChunkType::GenericLink:
                    block = new LinkPreviewBlock(
                                complexItem.get(),
                                chunkText);
                    break;

                case ChunkType::ImageLink:
                    block = new ImagePreviewBlock(
                                complexItem.get(), chatAimid, chunkText, chunk.ImageType_);
                    break;

                case ChunkType::FileSharingImage:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::image);
                    break;

                case ChunkType::FileSharingGif:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::gif);
                    break;

                case ChunkType::FileSharingVideo:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::video);
                    break;

                case ChunkType::FileSharingImageSnap:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::snap_image);
                    break;

                case ChunkType::FileSharingGifSnap:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::snap_gif);
                    break;

                case ChunkType::FileSharingVideoSnap:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::snap_video);
                    break;

                case ChunkType::FileSharingPtt:
                    block = new PttBlock(complexItem.get(), chunkText, chunk.DurationSec_, id, prev);
                    break;

                case ChunkType::FileSharingGeneral:
                    block = new FileSharingBlock(
                                complexItem.get(), chunkText, core::file_sharing_content_type::undefined);
                    break;

                case  ChunkType::Sticker:
                    block = new StickerBlock(complexItem.get(), chunk.Sticker_);
                    break;

                case ChunkType::Junk:
                    break;

                default:
                    assert(!"unexpected chunk type");
                    break;
            }

            if (block)
            {
                if (!chunk.Quote_.isEmpty())
                {
                    if (prevQuote && prevQuote->getQuote().id_ == chunk.Quote_.id_)
                    {
                        prevQuote->addBlock(block);
                    }
                    else
                    {
                        auto quoteBlock = new QuoteBlock(complexItem.get(), chunk.Quote_);
                        quoteBlock->addBlock(block);
                        items.emplace_back(quoteBlock);
                        quoteBlocks.push_back(quoteBlock);
                        prevQuote = quoteBlock;
                    }
                }
                else
                {
                    for (auto q : quoteBlocks)
                    {
                        q->setReplyBlock(block);
                    }
                    items.emplace_back(block);
                }
            }
        }

        if (!quoteBlocks.empty())
        {
            QString sourceText;
            for (auto i : items)
            {
                sourceText += i->getSourceText();
            }
            complexItem->setSourceText(sourceText);
        }
        complexItem->setItems(std::move(items));

        return complexItem.release();
    }
}

namespace
{
    const TextChunk TextChunk::Empty(ChunkType::Undefined, 0, 1, QString(), -1);

    TextChunk::TextChunk(
        const ChunkType type,
        const int32_t beginIndex,
        const int32_t endIndex,
        const QString &imageType,
        const int32_t durationSec)
        : Type_(type)
        , BeginIndex_(beginIndex)
        , EndIndex_(endIndex)
        , ImageType_(imageType)
        , DurationSec_(durationSec)
    {
        assert(Type_ > ChunkType::Min);
        assert(Type_ < ChunkType::Max);
        assert(BeginIndex_ >= 0);
        assert((Type_ != ChunkType::ImageLink) || !ImageType_.isEmpty());
        assert(DurationSec_ >= -1);
    }

    int32_t TextChunk::length() const
    {
        assert(BeginIndex_ >= 0);
        assert(BeginIndex_ < EndIndex_);

        return (EndIndex_ - BeginIndex_);
    }

    TextChunk TextChunk::mergeWith(const TextChunk &chunk) const
    {
        if (Type_ != ChunkType::Text)
        {
            return TextChunk::Empty;
        }

        if (chunk.Type_ != ChunkType::Text)
        {
            return TextChunk::Empty;
        }

        return TextChunk(
            ChunkType::Text,
            BeginIndex_,
            chunk.EndIndex_,
            QString(),
            -1);
    }

    TextChunk findNextChunk(const QString &text, const int32_t beginIndex)
    {
        assert(!text.isEmpty());
        assert(beginIndex >= 0);
        assert(beginIndex < text.length());

        Utils::UrlParser parser;

        bool textFound = false;
        for (auto i = beginIndex, end = text.length(); i < end; )
        {
            parser.process(text.midRef(i, text.length() - i));

            const auto length = parser.charsProcessed();

            const auto prev = i;

            i = beginIndex + length;

            if (!parser.hasUrl())
            {
                textFound = true;
                continue;
            }

            if (textFound)
                return TextChunk(ChunkType::Text, beginIndex, prev, QString(), -1);

            const auto url = parser.getUrl();
            const auto chunkText = text.mid(beginIndex, length).trimmed();

            switch (url.type_)
            {
            case common::tools::url::type::image:
            case common::tools::url::type::video:
                return TextChunk(ChunkType::ImageLink, beginIndex, beginIndex + length, to_string(url.extension_), -1);
            case common::tools::url::type::filesharing:
            {
                const QString& id = extractIdFromFileSharingUri(chunkText);
                const auto content_type = extractContentTypeFromFileSharingId(id);

                auto chunkType = ChunkType::FileSharingGeneral;
                switch (content_type)
                {
                case core::file_sharing_content_type::image:
                    chunkType = ChunkType::FileSharingImage;
                    break;
                case core::file_sharing_content_type::gif:
                    chunkType = ChunkType::FileSharingGif;
                    break;
                case core::file_sharing_content_type::video:
                    chunkType = ChunkType::FileSharingVideo;
                    break;
                case core::file_sharing_content_type::snap_image:
                    chunkType = ChunkType::FileSharingImageSnap;
                    break;
                case core::file_sharing_content_type::snap_gif:
                    chunkType = ChunkType::FileSharingGifSnap;
                    break;
                case core::file_sharing_content_type::snap_video:
                    chunkType = ChunkType::FileSharingVideoSnap;
                    break;
                case core::file_sharing_content_type::ptt:
                    chunkType = ChunkType::FileSharingPtt;
                    break;
                }

                const auto durationSec = extractDurationFromFileSharingId(id);

                return TextChunk(chunkType, beginIndex, beginIndex + length, QString(), durationSec);
            }
            case common::tools::url::type::site:
                return TextChunk(ChunkType::GenericLink, beginIndex, beginIndex + length, QString(), -1);
            case common::tools::url::type::email:
                continue;
            case common::tools::url::type::ftp:
                return TextChunk(ChunkType::GenericLink, beginIndex, beginIndex + length, QString(), -1);
            }
        }

        return TextChunk(ChunkType::Text, beginIndex, text.length(), QString(), -1);
    }
}

UI_COMPLEX_MESSAGE_NS_END
