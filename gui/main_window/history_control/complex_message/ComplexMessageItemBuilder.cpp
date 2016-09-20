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

    bool isImageOrVideoUri(const QString &uri, Out QString &ext)
    {
        assert(!uri.isEmpty());

        Out ext.resize(0);

        const auto uriInfo = QUrl::fromUserInput(uri);

        if (!uriInfo.isValid())
        {
            assert(!"unexpected parser issue");
            return false;
        }

        const auto filename = uriInfo.fileName();
        if (filename.isEmpty())
        {
            return false;
        }

        const QFileInfo fileInfo(filename);

        const auto suffix = fileInfo.suffix().toLower();
        if (suffix.isEmpty())
        {
            return false;
        }

        if (History::IsImageExtension(suffix) ||
            History::IsVideoExtension(suffix))
        {
            Out ext = suffix;
            return true;
        }

        return false;
    }

    bool isFileSharingLink(const QString &link, Out core::file_sharing_content_type &content_type, Out int32_t &duration_sec)
    {
        Out duration_sec = -1;

        Out content_type = core::file_sharing_content_type::undefined;

        const auto id = extractIdFromFileSharingUri(link);

        if (id.isEmpty())
        {
            return false;
        }

        Out content_type = extractContentTypeFromFileSharingId(id);

        Out duration_sec = extractDurationFromFileSharingId(id);

        return true;
    }

    TextChunk findNextChunk(const QString &text, const int32_t beginIndex)
    {
        assert(!text.isEmpty());
        assert(beginIndex >= 0);
        assert(beginIndex < text.length());

        QString linkType;

        auto chunkType = ChunkType::Undefined;
        QChar prevChar;

        auto index = beginIndex;

        for (; index < text.length(); ++index)
        {
            const auto currentChar = text[index];

            const auto isEndOfPreview = (currentChar.isSpace() && (chunkType == ChunkType::GenericLink));
            if (isEndOfPreview)
            {
                break;
            }

            const auto tail = text.midRef(index);
            assert(!tail.isEmpty());

            const auto isLink = (
                tail.startsWith("http://") ||
                tail.startsWith("www.") ||
                tail.startsWith("https://"));

            const auto isEndOfText = (isLink && prevChar.isSpace() && (chunkType == ChunkType::Text));
            if (isEndOfText)
            {
                break;
            }

            const auto isFirstChar = (chunkType == ChunkType::Undefined);
            if (isFirstChar)
            {
                assert(prevChar.isNull());

                chunkType = (isLink ? ChunkType::GenericLink : ChunkType::Text);
            }

            prevChar = currentChar;
        }

        auto durationSec = -1;

        if (chunkType == ChunkType::GenericLink)
        {
            const auto textLength = (index - beginIndex);
            assert(textLength > 0);

            const auto link = text.mid(beginIndex, textLength);

            auto fileSharingType = core::file_sharing_content_type::undefined;
            const auto isFsLink = isFileSharingLink(link, Out fileSharingType, Out durationSec);

            const auto isFsGifSnapLink = (fileSharingType == core::file_sharing_content_type::snap_gif);
            const auto isFsImageSnapLink = (fileSharingType == core::file_sharing_content_type::snap_image);
            const auto isFsVideoSnapLink = (fileSharingType == core::file_sharing_content_type::snap_video);
            const auto isFsVideoLink = (fileSharingType == core::file_sharing_content_type::video);
            const auto isFsImageLink = (fileSharingType == core::file_sharing_content_type::image);
            const auto isFsGifImageLink = (fileSharingType == core::file_sharing_content_type::gif);
            const auto isFsPtt = (fileSharingType == core::file_sharing_content_type::ptt);

            if (isFsImageLink)
                chunkType = ChunkType::FileSharingImage;
            else if (isFsImageSnapLink)
                chunkType = ChunkType::FileSharingImageSnap;
            else if (isFsGifImageLink)
                chunkType = ChunkType::FileSharingGif;
            else if (isFsGifSnapLink)
                chunkType = ChunkType::FileSharingGifSnap;
            else if (isFsVideoLink)
                chunkType = ChunkType::FileSharingVideo;
            else if (isFsVideoSnapLink)
                chunkType = ChunkType::FileSharingVideoSnap;
            else if (isFsPtt)
                chunkType = ChunkType::FileSharingPtt;
            else if (isFsLink)
                chunkType = ChunkType::FileSharingGeneral;
            else
            {
                const auto urlParser = QUrl::fromUserInput(link);
                const auto isValidLink = (urlParser.isValid() && !urlParser.isRelative());

                const auto replaceWithText = !isValidLink;
                if (replaceWithText)
                {
                    chunkType = ChunkType::Text;
                }
                else if (isImageOrVideoUri(link, Out linkType))
                {
                    chunkType = ChunkType::ImageLink;
                }
            }
        }

        if (chunkType == ChunkType::Text)
        {
            const auto textLength = (index - beginIndex);
            assert(textLength > 0);

            const auto textChunk = text.mid(beginIndex, textLength).trimmed();
            if (textChunk.isEmpty())
            {
                chunkType = ChunkType::Junk;
            }
        }

        return TextChunk(chunkType, beginIndex, index, linkType, durationSec);
    }
}

UI_COMPLEX_MESSAGE_NS_END
