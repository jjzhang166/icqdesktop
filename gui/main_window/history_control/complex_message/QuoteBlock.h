#pragma once

#include "GenericBlock.h"
#include "QuoteBlockLayout.h"
#include "../../../types/message.h"

UI_NS_BEGIN

class TextEmojiWidget;
class PictureWidget;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;

class QuoteBlock final : public GenericBlock
{
    friend class QuoteBlockLayout;

    Q_OBJECT

public:
    QuoteBlock(ComplexMessageItem *parent, const Data::Quote & quote);

    virtual ~QuoteBlock() override;

    virtual void clearSelection() override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    virtual QString getSelectedText() const override;

    virtual bool hasRightStatusPadding() const override;

    virtual bool isSelected() const override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    virtual void onVisibilityChanged(const bool isVisible) override;

    virtual QRect setBlockGeometry(const QRect &ltr) override;
    
    virtual void onActivityChanged(const bool isActive) override;
    
    virtual bool replaceBlockWithSourceText(IItemBlock *block) override;
    
    virtual bool isSharingEnabled() const override;

    virtual bool containSharingBlock() const override;
    
    virtual QString getSourceText() const override;
    
    virtual QString formatRecentsText() const override;
    
    virtual bool standaloneText() const override;

    virtual bool isSimple() const override { return false; }

    virtual Data::Quote getQuote() const;

    virtual bool needFormatQuote() const { return false; }

    void addBlock(GenericBlock* block);

    bool needForwardBlock() const;

    void setReplyBlock(GenericBlock* block);

protected:
    virtual void drawBlock(QPainter &p) override;

    virtual void initialize() override;

private:
    bool quoteOnly() const;

private:

    Data::Quote Quote_;

    QuoteBlockLayout *Layout_;

    std::vector<GenericBlock*> Blocks_;

    QLabel* ForwardLabel_;

    TextEmojiWidget *TextCtrl_;
    
    QRect Geometry_;

    BlockSelectionType Selection_;
    
    ComplexMessageItem* Parent_;

    GenericBlock* ReplyBlock_;

    PictureWidget* ForwardIcon_;
};

UI_COMPLEX_MESSAGE_NS_END
