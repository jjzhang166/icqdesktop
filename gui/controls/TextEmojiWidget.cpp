#include "stdafx.h"
#include "TextEmojiWidget.h"
#include "../utils/SChar.h"
#include "../cache/emoji/Emoji.h"

#include <thread>

namespace Ui
{
	class tew_lex
	{
	protected:
		int		width_;
		int     height_;

	public:
		tew_lex() : width_(-1), height_(-1) {}

		virtual int draw(QPainter& _painter, int _x, int _y) = 0;
		virtual int width(QPainter& _painter) = 0;
		virtual int height(QPainter& _painter) = 0;

		virtual bool is_space() const = 0;
		virtual bool is_eol() const = 0;
	};

	class tew_lex_text : public tew_lex
	{
		QString		text_;

		virtual int draw(QPainter& _painter, int _x, int _y) override
		{
			_painter.fillRect(_x, _y - height(_painter) + 1, width(_painter) + 1, height(_painter) + 1, _painter.brush());
			_painter.drawText(_x, _y, text_);
			return width(_painter);
		}

		virtual int width(QPainter& _painter) override
		{
			if (width_ == -1)
				width_ = _painter.fontMetrics().width(text_);
			return width_;
		}

		virtual int height(QPainter& _painter) override
		{
			if (height_ == -1)
				height_ = _painter.fontMetrics().height();
			return height_;
		}

		virtual bool is_space() const override
		{
			return text_ == " ";
		}

		virtual bool is_eol() const override
		{
			return text_ == "\n";
		}

	public:

		tew_lex_text()
		{
			text_.reserve(50);
		}

		void append(const QString& _text)
		{
			text_.append(_text);
		}
	};

	class tew_lex_emoji : public tew_lex
	{
		int			main_code_;
		int			ext_code_;

		QImage		image_;

		const QImage& get_image(const QFontMetrics& _m)
		{
			if (image_.isNull())
			{
				image_ = Emoji::GetEmoji(main_code_, ext_code_, Emoji::GetFirstLesserOrEqualSizeAvailable(_m.ascent() - _m.descent()));
			}
			return image_;
		}

		virtual int draw(QPainter& _painter, int _x, int _y) override
		{
			int image_height = width(_painter);

			QRect draw_rect(_x, _y - image_height, image_height, image_height);

			QFontMetrics m(_painter.font());

			_painter.fillRect(draw_rect, _painter.brush());
			_painter.drawImage(draw_rect, get_image(m));

			return image_height;
		}

		virtual int width(QPainter& _painter) override
		{
			if (width_ == -1)
			{
				QFontMetrics m(_painter.font());
				int image_max_height = m.ascent();
				int image_height = get_image(m).height();
				if (image_height > image_max_height)
					image_height = image_max_height;
				width_ = image_height;
			}
			return width_;
		}

		virtual int height(QPainter& _painter) override
		{
			if (height_ == -1)
			{
				QFontMetrics m(_painter.font());
				int image_max_height = m.ascent();
				int image_height = get_image(m).height();
				if (image_height > image_max_height)
					image_height = image_max_height;
				height_ = image_height;
			}
			return height_;
		}

		virtual bool is_space() const override
		{
			return false;
		}

		virtual bool is_eol() const override
		{
			return false;
		}

	public:

		tew_lex_emoji(int _main_code, int _ext_code)
			:	main_code_(_main_code),
			ext_code_(_ext_code)
		{
		}
	};

	compiled_text::compiled_text()
		:	width_(-1), height_(-1)
	{
	}

	int compiled_text::width(QPainter& _painter)
	{
		if (width_ == -1)
			for (auto lex : lexs_)
				width_ += lex->width(_painter);
		return width_ + 1;
	}

	int compiled_text::height(QPainter& _painter)
	{
		if (height_ == -1)
			for (auto lex : lexs_)
				height_ = std::max(height_, lex->height(_painter));
		return height_ + 1;
	}

	void compiled_text::push_back(std::shared_ptr<tew_lex> _lex)
	{
		lexs_.push_back(_lex);
	}

	int compiled_text::draw(QPainter& _painter, int _x, int _y, int _w)
	{
		auto xmax = _w;
		if (_w > 0)
		{
			xmax = _w - _painter.fontMetrics().width("...");
		}
		int i = 0;
		for (auto lex : lexs_)
		{
			_x += lex->draw(_painter, _x, _y);
			if (xmax > 0 && _x > xmax && i < (int) lexs_.size() - 1)
			{
				_painter.drawText(_x, _y, "...");
				break;
			}
			++i;
		}
		return _x;
	}

	int compiled_text::draw(QPainter& _painter, int _x, int _y, int _w, int _dh)
	{
		auto h = _y + _dh;
		for (auto lex : lexs_)
		{
			auto lexw = lex->width(_painter);
			if ((_x + lexw) >= _w || lex->is_eol())
			{
				_x = 0;
				_y += height(_painter);
			}
			h = _y + _dh;
			if (!(lex->is_space() && _x == 0))
				_x += lex->draw(_painter, _x, _y);
		}
		return h;
	}

	bool compile_text(const QString& _text, compiled_text& _result, bool multiline)
	{
		QTextStream input_stream(const_cast<QString*>(&_text), QIODevice::ReadOnly);

		std::shared_ptr<tew_lex_text> last_text_lex;

		auto is_eos = [&](int _offset)
		{
			assert(_offset >= 0);
			assert(input_stream.string());

			const auto &text = *input_stream.string();
			return ((input_stream.pos() + _offset) >= text.length());
		};

		auto peek_schar = [&]
		{
			return Utils::PeekNextSuperChar(input_stream);
		};

		auto read_schar = [&]
		{
			return Utils::ReadNextSuperChar(input_stream);
		};

		auto convert_plain_character = [&]
		{
			if (!last_text_lex)
			{
				last_text_lex = std::make_shared<tew_lex_text>();
				_result.push_back(last_text_lex);
			}
			last_text_lex->append(read_schar().ToQString());
		};

		auto convert_eol_space = [&]
		{
			const auto ch = peek_schar();
			if (ch.IsNull())
			{
				read_schar();
				last_text_lex.reset();
				return true;
			}
			if (ch.IsNewline() || ch.IsSpace())
			{
				read_schar();
				last_text_lex.reset();
				last_text_lex = std::make_shared<tew_lex_text>();
				_result.push_back(last_text_lex);
				last_text_lex->append(ch.ToQString());
				last_text_lex.reset();
				return true;
			}
			return false;
		};

		auto convert_emoji = [&]()
		{
			const auto ch = peek_schar();
			if (ch.IsEmoji())
			{
#ifdef __APPLE__
                last_text_lex.reset();
#else
                read_schar();
				_result.push_back(std::make_shared<tew_lex_emoji>(ch.Main(), ch.Ext()));
                last_text_lex.reset();
                return true;
#endif
			}
			return false;
		};

		while (!is_eos(0))
		{
			if (multiline && convert_eol_space())
				continue;

			if (convert_emoji())
				continue;
            
			convert_plain_character();
		}

		return true;
	}

	TextEmojiWidgetEvents& TextEmojiWidget::events()
	{
		static TextEmojiWidgetEvents e;
		return e;
	}

    TextEmojiWidget::TextEmojiWidget(QWidget* _parent, Utils::FontsFamily _font_family, int _font_size, const QColor& _color, int _size_to_baseline)
		:	QWidget(_parent),
		align_(TextEmojiAlign::allign_left),
		color_(_color),
		size_to_baseline_(_size_to_baseline),
		ellipsis_(false),
		multiline_(false),
		selectable_(false)
	{
        font_ = Utils::appFont(_font_family, _font_size);

		QFontMetrics metrics = QFontMetrics(font_);

		int ascent = metrics.ascent();
		descent_ = metrics.descent();

		int height = metrics.height();

		if (_size_to_baseline != -1)
		{
			if (_size_to_baseline > ascent)
			{
				height = size_to_baseline_ + descent_;
			}
		}
		else
		{
			size_to_baseline_ = ascent;
		}

		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setFixedHeight(height);

		setFocusPolicy(Qt::StrongFocus);
	}

	int TextEmojiWidget::ascent()
	{
		return size_to_baseline_;
	}

	int TextEmojiWidget::descent()
	{
		return descent_;
	}

	TextEmojiWidget::~TextEmojiWidget(void)
	{
	}

	void TextEmojiWidget::setText(const QString& _text, TextEmojiAlign _align)
	{
		align_ = _align;
		text_ = _text;
		compiled_text_.reset(new compiled_text());
		if (!compile_text(text_, *compiled_text_, multiline_))
			assert(!"TextEmojiWidget: couldn't compile text.");
        QWidget::update();
	}

    void TextEmojiWidget::setText(const QString &_text, const QColor &_color, TextEmojiAlign _align)
    {
        color_ = _color;
        setText(_text, _align);
    }

    void TextEmojiWidget::setColor(const QColor &_color)
    {
        color_ = _color;
        setText(text_);
    }

	void TextEmojiWidget::set_ellipsis(bool _v)
	{
        if (ellipsis_ != _v)
        {
            ellipsis_ = _v;
            setText(text_);
        }
	}

	void TextEmojiWidget::set_multiline(bool _v)
	{
		if (multiline_ != _v)
		{
			if (multiline_)
				ellipsis_ = false;
			multiline_ = _v;
			setText(text_);
		}
	}

	void TextEmojiWidget::set_selectable(bool _v)
	{
		if (selectable_ != _v)
		{
			if (_v)
			{
				disconnector_.add("selected", connect(&events(), &TextEmojiWidgetEvents::selected, [this](TextEmojiWidget* obj)
				{
					if ((TextEmojiWidget*)this != obj)
					{
						selection_ = QPoint();
                        QWidget::update();
					}
				}));
			}
			else
			{
                disconnector_.remove("selected");
			}
			selectable_ = _v;
			selection_ = QPoint();
            QWidget::update();
		}
	}

	void TextEmojiWidget::setSizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver)
	{
		if (hor == QSizePolicy::Preferred)
        {
            setFixedWidth(geometry().width());
            QWidget::update();
        }

		return QWidget::setSizePolicy(hor, ver);
	}

	void TextEmojiWidget::paintEvent(QPaintEvent* _e)
	{
		QPainter painter(this);
		painter.save();
		painter.setFont(font_);
		painter.setPen(color_);
		if (selection_.y() > 0)
			painter.setBrush(QColor("#cae6b3"));

		QStyleOption opt;
		opt.init(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

		int offset_x = 0;

		int width = compiled_text_->width(painter);

		QRect rc = geometry();

		switch (align_)
		{
		case Ui::allign_left:
			offset_x = 0;
			break;
		case Ui::allign_right:
			offset_x = rc.width() - width;
			break;
		case Ui::allign_center:
			offset_x = (rc.width() - width)/2;
			break;
		default:
			assert(false);
			break;
		}

		int offset_y = size_to_baseline_;

		if (ellipsis_ || !multiline_)
		{
            if (ellipsis_)
            {
                QLinearGradient gradient(contentsRect().topLeft(), contentsRect().topRight());
                gradient.setColorAt(0.7, color_);
                gradient.setColorAt(1.0, palette().color(QPalette::Window));
                QPen pen;
                pen.setBrush(QBrush(gradient));
                painter.setPen(pen);
            }
			auto w = compiled_text_->draw(painter, offset_x, offset_y, ellipsis_ ? geometry().width() : -1);
			if (sizePolicy().horizontalPolicy() == QSizePolicy::Preferred && w != geometry().width())
				setFixedWidth(w);
		}
		else
		{
			auto h = compiled_text_->draw(painter, offset_x, offset_y, geometry().width(), descent());
			if (h != geometry().height())
            {
                QFontMetrics metrics = QFontMetrics(font_);
				setFixedHeight(std::max(metrics.height(), h));
            }
		}

		painter.restore();
		QWidget::paintEvent(_e);
	}

	void TextEmojiWidget::resizeEvent(QResizeEvent *_e)
	{
		QWidget::resizeEvent(_e);
        if (_e->oldSize().width() != -1 && _e->oldSize().height() != -1)
            emit setSize(_e->size().width() - _e->oldSize().width(), _e->size().height() - _e->oldSize().height());
	}

	void TextEmojiWidget::mousePressEvent(QMouseEvent *_e)
	{
		if (selectable_)
		{
			selection_ = QPoint();
            QWidget::update();
			return;
		}
        emit clicked();
		return QWidget::mousePressEvent(_e);
	}

	void TextEmojiWidget::mouseDoubleClickEvent(QMouseEvent *_e)
	{
		if (selectable_)
		{
			selection_ = QPoint(0, text_.count());
            QWidget::update();
			emit events().selected(this);
			return;
		}
		return QWidget::mouseDoubleClickEvent(_e);
	}

	void TextEmojiWidget::keyPressEvent(QKeyEvent *_e)
	{
		if (selectable_)
		{
			if (_e->matches(QKeySequence::Copy))
			{
				qApp->clipboard()->setText(text());
				selection_ = QPoint();
                QWidget::update();
				return;
			}
		}
		return QWidget::keyPressEvent(_e);
	}

	void TextEmojiWidget::focusOutEvent(QFocusEvent *_e)
	{
		if (selectable_)
		{
			selection_ = QPoint();
            QWidget::update();
		}
		return QWidget::focusOutEvent(_e);
	}

	////

	TextEmojiLabel::TextEmojiLabel(QWidget* _parent) :
		LabelEx(_parent)
	{
		setSizeToBaseline(-1);
        setSizeToOffset(5);
	}

	TextEmojiLabel::~TextEmojiLabel()
	{
		//
	}

    void TextEmojiLabel::setSizeToOffset(int size)
    {
       left_offset_ = size;
    }

	void TextEmojiLabel::setSizeToBaseline(int size)
	{
		QFontMetrics metrics = QFontMetrics(QLabel::font());
		int height = metrics.height();
		ascent_ = metrics.ascent();
		descent_ = metrics.descent();

		size_to_baseline_ = size;
		if (size != -1)
		{
			if (size > ascent_)
				height = size_to_baseline_ + descent_;
		}
		else
			size_to_baseline_ = ascent_;

        QWidget::update();
	}

	void TextEmojiLabel::setText(const QString& text)
	{
		QLabel::setText(text);
		compiled_text_.reset(new compiled_text());
		if (!compile_text(QLabel::text(), *compiled_text_, false))
			assert(false && "TextEmojiLabel::setText: couldn't compile text.");
	}

	void TextEmojiLabel::paintEvent(QPaintEvent* event)
	{
		QPainter painter(this);
		internalDraw(painter, geometry());
		QWidget::paintEvent(event);
	}

	int TextEmojiLabel::internalWidth(QPainter& painter) {
		return !!compiled_text_ ? compiled_text_->width(painter) + 5 : 0;
	}

	void TextEmojiLabel::internalDraw(QPainter& painter, const QRect& rectDraw) {
		if (!compiled_text_) {
			//assert(!"error: compiled text is not initialized");
			return;
		}

		QStyleOption opt;
		opt.init(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

		int width = compiled_text_->width(painter);

		int offset_x = left_offset_;//geometry().x();
		if (alignment() == Qt::AlignRight)
			offset_x = rectDraw.width() - width;
		else if (alignment() == Qt::AlignCenter)
			offset_x = (rectDraw.width() - width)/2;

		int offset_y = size_to_baseline_;

		compiled_text_->draw(painter, offset_x, offset_y, -1);
	}
}

