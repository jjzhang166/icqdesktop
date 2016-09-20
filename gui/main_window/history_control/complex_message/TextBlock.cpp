#include "stdafx.h"

#include "../../../cache/themes/themes.h"
#include "../../../controls/TextEditEx.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/log/log.h"
#include "../../../utils/Text2DocConverter.h"

#include "../MessageStyle.h"

#include "TextBlockLayout.h"
#include "Selection.h"

#include "TextBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

namespace
{
    const QString SELECTION_STYLE =
        QString("background: transparent; selection-background-color: %1;")
        .arg(Utils::rgbaStringFromColor(Utils::getSelectionColor()));

    QString extractTrailingSpaces(const QString &text, Out QString &trimmedText)
    {
        assert(!text.isEmpty());

        Out trimmedText.resize(0);
        Out trimmedText += text;

        QString trailingSpaces;

        if (text.isEmpty())
        {
            return trailingSpaces;
        }

        trailingSpaces.reserve(trimmedText.length());

        while(!trimmedText.isEmpty())
        {
            const auto lastCharIndex = (trimmedText.length() - 1);

            const auto lastChar = trimmedText[lastCharIndex];

            if (!lastChar.isSpace())
            {
                break;
            }

            trailingSpaces += lastChar;
            trimmedText.resize(lastCharIndex);
        }

        return trailingSpaces;
    }
}

TextBlock::TextBlock(ComplexMessageItem *parent, const QString &text)
    : GenericBlock(parent, text, MenuFlagNone, false)
    , Text_(text)
    , Layout_(nullptr)
    , TextCtrl_(nullptr)
    , Selection_(BlockSelectionType::None)
{
    assert(!Text_.isEmpty());

    TrimmedText_.reserve(Text_.length());
    TrailingSpaces_ = extractTrailingSpaces(Text_, Out TrimmedText_);

    Layout_ = new TextBlockLayout();
    setLayout(Layout_);
}

TextBlock::~TextBlock()
{

}

void TextBlock::clearSelection()
{
    if (TextCtrl_)
    {
        TextCtrl_->clearSelection();
    }

    Selection_ = BlockSelectionType::None;
}

IItemBlockLayout* TextBlock::getBlockLayout() const
{
    return Layout_;
}

QString TextBlock::getSelectedText() const
{
    if (!TextCtrl_)
    {
        return QString();
    }

    switch (Selection_)
    {
        case BlockSelectionType::Full:
            return Text_;

        case BlockSelectionType::TillEnd:
            return (TextCtrl_->selection() + TrailingSpaces_);
    }

    return TextCtrl_->selection();
}

bool TextBlock::hasRightStatusPadding() const
{
    return true;
}

bool TextBlock::isDraggable() const
{
    return false;
}

bool TextBlock::isSelected() const
{
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    return (Selection_ != BlockSelectionType::None);
}

bool TextBlock::isSharingEnabled() const
{
    return false;
}

void TextBlock::selectByPos(const QPoint& from, const QPoint& to, const BlockSelectionType selection)
{
    from;

    assert(selection > BlockSelectionType::Min);
    assert(selection < BlockSelectionType::Max);
    assert(Selection_ > BlockSelectionType::Min);
    assert(Selection_ < BlockSelectionType::Max);

    if (!TextCtrl_)
    {
        return;
    }

    Selection_ = selection;

    const auto selectAll = (selection == BlockSelectionType::Full);
    if (selectAll)
    {
        TextCtrl_->selectWholeText();
        return;
    }

    const auto selectFromBeginning = (selection == BlockSelectionType::FromBeginning);
    if (selectFromBeginning)
    {
        TextCtrl_->selectFromBeginning(to);
        return;
    }

    const auto selectTillEnd = (selection == BlockSelectionType::TillEnd);
    if (selectTillEnd)
    {
        TextCtrl_->selectTillEnd(from);
        return;
    }

    TextCtrl_->selectByPos(from, to);
}

void TextBlock::drawBlock(QPainter &p)
{
    p;
}

void TextBlock::initialize()
{
    GenericBlock::initialize();

    assert(!TextCtrl_);
    assert(!Text_.isEmpty());
    TextCtrl_ = createTextEditControl(Text_);
    TextCtrl_->setVisible(true);

    QObject::connect(
        TextCtrl_,
        &QTextBrowser::anchorClicked,
        this,
        &TextBlock::onAnchorClicked);
}

TextEditEx* TextBlock::createTextEditControl(const QString &text)
{
    assert(!text.isEmpty());

    blockSignals(true);
    setUpdatesEnabled(false);

    auto textControl = new Ui::TextEditEx(
        this,
        MessageStyle::getTextFont(),
        MessageStyle::getTextColor(),
        false,
        false);

    textControl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    textControl->setStyle(QApplication::style());
    textControl->setFrameStyle(QFrame::NoFrame);
    textControl->setFocusPolicy(Qt::NoFocus);
    textControl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textControl->setOpenLinks(false);
    textControl->setOpenExternalLinks(false);
    textControl->setWordWrapMode(QTextOption::WordWrap);
    textControl->setStyleSheet("background: transparent");
    textControl->setContextMenuPolicy(Qt::NoContextMenu);
    textControl->setReadOnly(true);
    textControl->setUndoRedoEnabled(false);

    setTextEditTheme(textControl);

    textControl->verticalScrollBar()->blockSignals(true);

    Logic::Text4Edit(TrimmedText_, *textControl, Logic::Text2DocHtmlMode::Escape, true, true);

    textControl->document()->setDocumentMargin(0);

    textControl->verticalScrollBar()->blockSignals(false);

    setUpdatesEnabled(true);
    blockSignals(false);

    return textControl;
}

void TextBlock::setTextEditTheme(TextEditEx *textControl)
{
    assert(textControl);

    Utils::ApplyStyle(textControl, SELECTION_STYLE);
    auto textColor = MessageStyle::getTextColor();

    const auto theme = getTheme();
    if (theme)
    {
        textColor = (
            isOutgoing() ?
                theme->outgoing_bubble_.text_color_ :
                theme->incoming_bubble_.text_color_);
    }

    QPalette palette;
    palette.setColor(QPalette::Text, textColor);
    textControl->setPalette(palette);
}

void TextBlock::onAnchorClicked(const QUrl &_url)
{
    QString uin;

    if (Utils::extractUinFromIcqLink(_url.toString(), Out uin))
    {
        assert(!uin.isEmpty());
        emit Utils::InterConnector::instance().profileSettingsShow(uin);
        return;
    }

    QDesktopServices::openUrl(_url);
}

UI_COMPLEX_MESSAGE_NS_END