#include "stdafx.h"
#include "TextEditEx.h"
#include "../cache/emoji/Emoji.h"
#include "../utils/Text2DocConverter.h"
#include "../utils/utils.h"

namespace Ui
{
    TextEditEx::TextEditEx(QWidget* parent, const Utils::FontsFamily fontFamily, int fontSize, const QPalette& palette, bool input, bool _isFitToText)
        :	QTextBrowser(parent),
        index_(0),
        font_(Utils::appFont(fontFamily, fontSize)),
        prev_pos_(-1),
        input_(input),
        is_fit_to_text_(_isFitToText),
        is_catch_enter_(false)
    {
        init(fontSize);
        setPalette(palette);
    }

    TextEditEx::TextEditEx(QWidget* parent, const Utils::FontsFamily fontFamily, int fontSize, const QColor& color, bool input, bool _isFitToText)
        :	QTextBrowser(parent),
        index_(0),
        font_(Utils::appFont(fontFamily, fontSize)),
        color_(color),
        prev_pos_(-1),
        input_(input),
        is_fit_to_text_(_isFitToText),
        is_catch_enter_(false)
    {
        init(fontSize);
        setTextColor(color_);
    }

    void TextEditEx::init(int /*fontSize*/)
    {
        setAcceptRichText(false);
        setFont(font_);
        document()->setDefaultFont(font_);

        QPalette p = palette();
        p.setColor(QPalette::Highlight, Utils::getSelectionColor());
        setPalette(p);

        if (is_fit_to_text_)
        {
            connect(this->document(), SIGNAL(contentsChanged()), this, SLOT(edit_content_changed()), Qt::QueuedConnection);
        }
    }

    void TextEditEx::edit_content_changed()
    {
        int doc_height = this->document()->size().height();

        if (doc_height < Utils::scale_value(20) || doc_height > Utils::scale_value(250))
        {
            return;
        }

        auto old_height = height();

        setFixedHeight(doc_height);

        if (height() != old_height)
        {
            emit setSize(0, height() - old_height);
        }
    }

    QString TextEditEx::getPlainText(int _from, int _to) const
    {
        if (_from == _to)
            return "";

        if (_to != -1 && _to < _from)
        {
            assert(!"invalid data");
            return "";
        }

        QString out_string;
        QTextStream result(&out_string);

        int pos_start = 0;
        int length = 0;

        bool first = true;

        for (QTextBlock it_block = document()->begin(); it_block != document()->end(); it_block = it_block.next())
        {
            if (!first)
                result << '\n';

            pos_start = it_block.position();
            if (_to != -1 && pos_start >= _to)
                break;

            for (QTextBlock::iterator it_fragment = it_block.begin(); it_fragment != it_block.end(); ++it_fragment)
            {
                QTextFragment currentFragment = it_fragment.fragment();

                if (currentFragment.isValid())
                {
                    pos_start = currentFragment.position();
                    length = currentFragment.length();

                    if (pos_start + length <= _from)
                        continue;

                    if (_to != -1 && pos_start >= _to)
                        break;

                    first = false;

                    if (currentFragment.charFormat().isImageFormat())
                    {
                        if (pos_start < _from)
                            continue;

                        QTextImageFormat imgFmt = currentFragment.charFormat().toImageFormat();

                        auto iter = resource_index_.find(imgFmt.name());
                        if (iter != resource_index_.end())
                            result << iter->second;
                    }
                    else
                    {
                        QString fragment_text = currentFragment.text();

                        int c_start = std::max((_from - pos_start), 0);
                        int count = -1;
                        if (_to != -1 && _to <= pos_start + length)
                            count = _to - pos_start - c_start;

                        QString txt = fragment_text.mid(c_start, count);
                        txt.remove(QChar::SoftHyphen);

                        QChar *uc = txt.data();
                        QChar *e = uc + txt.size();

                        for (; uc != e; ++uc) {
                            switch (uc->unicode()) {
                            case 0xfdd0: // QTextBeginningOfFrame
                            case 0xfdd1: // QTextEndOfFrame
                            case QChar::ParagraphSeparator:
                            case QChar::LineSeparator:
                                *uc = QLatin1Char('\n');
                                break;
                            case QChar::Nbsp:
                                *uc = QLatin1Char(' ');
                                break;
                            default:
                                ;
                            }
                        }

                        result << txt;
                    }
                }
            }
        }

        return out_string;
    }

    QMimeData* TextEditEx::createMimeDataFromSelection() const
    {
        QMimeData* dt =  new QMimeData();

        dt->setText(getPlainText(textCursor().selectionStart(), textCursor().selectionEnd()));

        return dt;
    }

    void TextEditEx::insertFromMimeData(const QMimeData* _source)
    {
        if (_source->hasText())
        {
            Logic::Text4Edit(_source->text(), *this, Logic::Text2DocHtmlMode::Pass, false);
        }
    }

    void TextEditEx::insertPlainText_(const QString& _text)
    {
        if (!_text.isEmpty())
        {
            Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Pass, false);
        }
    }

    bool TextEditEx::canInsertFromMimeData(const QMimeData* _source) const
    {
        return QTextBrowser::canInsertFromMimeData(_source);
    }

    void TextEditEx::focusInEvent(QFocusEvent* _event)
    {
        emit focusIn();
        QTextBrowser::focusInEvent(_event);
    }

    void TextEditEx::focusOutEvent(QFocusEvent* _event)
    {
        if (_event->reason() != Qt::ActiveWindowFocusReason)
            emit focusOut();

        QTextBrowser::focusOutEvent(_event);
    }

    void TextEditEx::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->source() == Qt::MouseEventNotSynthesized)
        {
            emit clicked();
            QTextBrowser::mousePressEvent(_event);
            if (!input_)
                _event->ignore();
        }
    }

    void TextEditEx::mouseReleaseEvent(QMouseEvent* _event)
    {
        QTextBrowser::mouseReleaseEvent(_event);
        if (!input_)
            _event->ignore();
    }

    void TextEditEx::mouseMoveEvent(QMouseEvent * _event)
    {
        if (_event->buttons() & Qt::LeftButton && !input_)
        {
            _event->ignore();
        }
        else
        {
            QTextEdit::mouseMoveEvent(_event);
        }
    }

    bool TextEditEx::catch_enter(int /*_modifiers*/)
    {
        return is_catch_enter_;
    }

    void TextEditEx::keyPressEvent(QKeyEvent* _event)
    {
        emit keyPressed(_event->key());

        if (_event->key() == Qt::Key_Backspace && toPlainText().isEmpty())
            emit emptyTextBackspace();

        if (_event->key() == Qt::Key_Escape)
            emit escapePressed();

        if (_event->key() == Qt::Key_Up)
            emit upArrow();

        if (_event->key() == Qt::Key_Down)
            emit downArrow();

        if (_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter)
        {
            Qt::KeyboardModifiers modifiers = _event->modifiers();

            if (catch_enter(modifiers))
            {
                emit enter();
                return;
            }
        }

        QTextBrowser::keyPressEvent(_event);
    }

    QString TextEditEx::getPlainText() const
    {
        return getPlainText(0, -1);
    }

    void TextEditEx::setPlainText(const QString& _text)
    {
        clear();
        resource_index_.clear();

        Logic::Text4Edit(_text, *this);
    }

    void TextEditEx::insert_emoji(int _main, int _ext)
    {
        merge_resources(Logic::InsertEmoji(_main, _ext, *this));
    }

    void TextEditEx::selectByPos(const QPoint& p)
    {
        QTextCursor c = cursorForPosition(mapFromGlobal(p));
        int pos = c.position();
        QTextCursor cur = textCursor();

        if (!cur.hasSelection() && prev_pos_ == -1)
        {
            cur.setPosition(pos);
            cur.setPosition(pos > prev_pos_ ? pos + 1  : pos -1, QTextCursor::KeepAnchor);
        }
        else
        {
            cur.setPosition(pos, QTextCursor::KeepAnchor);
        }
        prev_pos_ = pos;
        setTextCursor(cur);
    }

    void TextEditEx::clearSelection()
    {
        QTextCursor cur = textCursor();
        cur.clearSelection();
        setTextCursor(cur);
        prev_pos_ = -1;
    }

    bool TextEditEx::isAllSelected()
    {
        QTextCursor cur = textCursor();
        if (!cur.hasSelection())
            return false;

        const int start = cur.selectionStart();
        const int end = cur.selectionEnd();
        cur.setPosition(start);
        if (cur.atStart())
        {
            cur.setPosition(end);
            return cur.atEnd();
        }
        else if (cur.atEnd())
        {
            cur.setPosition(start);
            return cur.atStart();
        }

        return false;
    }

    QString TextEditEx::selection()
    {
        return getPlainText(textCursor().selectionStart(), textCursor().selectionEnd());
    }

    QSize TextEditEx::getTextSize() const
    {
        return document()->documentLayout()->documentSize().toSize();
    }

    void TextEditEx::merge_resources(const ResourceMap& _resources)
    {
        for (auto iter = _resources.cbegin(); iter != _resources.cend(); iter++)
            resource_index_[iter->first] = iter->second;
    }

    QSize TextEditEx::sizeHint() const
    {
        QSize sizeRect(document()->idealWidth(), document()->size().height());

        return sizeRect;
    }

    void TextEditEx::set_catch_enter(bool _is_catch_enter)
    {
        is_catch_enter_ = _is_catch_enter;
    }
}

