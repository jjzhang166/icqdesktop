#pragma once

#include "GenericBlock.h"

UI_NS_BEGIN

class TextEditEx;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;
class TextBlockLayout;

class TextBlock final : public GenericBlock
{
    friend class TextBlockLayout;

    Q_OBJECT

public:
    TextBlock(ComplexMessageItem *parent, const QString &text);

    virtual ~TextBlock() override;

    virtual void clearSelection() override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    virtual QString getSelectedText(bool isFullSelect = false) const override;

    virtual bool hasRightStatusPadding() const override;

    virtual bool isDraggable() const override;

    virtual bool isSelected() const override;

    virtual bool isSharingEnabled() const override;

    virtual void selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection) override;

    virtual void setFontSize(int size) override;

    virtual void setTextOpacity(double opacity) override;

protected:
    virtual void drawBlock(QPainter &p) override;

    virtual void initialize() override;

private:
    TextEditEx* createTextEditControl(const QString &text);

    void setTextEditTheme(TextEditEx *textControl);

    QString Text_;

    QString TrailingSpaces_;

    QString TrimmedText_;

    TextBlockLayout *Layout_;

    TextEditEx *TextCtrl_;

    BlockSelectionType Selection_;

    int TextFontSize_;

    double TextOpacity_;

private Q_SLOTS:
    void onAnchorClicked(const QUrl &_url);

};

UI_COMPLEX_MESSAGE_NS_END
