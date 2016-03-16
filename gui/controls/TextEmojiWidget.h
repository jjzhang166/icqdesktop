#pragma once
#include "../utils/utils.h"
#include "LabelEx.h"

namespace Ui
{
	class tew_lex;
	struct paint_info;
    class LabelEx;

	typedef std::list<std::shared_ptr<tew_lex>> tew_lex_list;

	class compiled_text
	{
		tew_lex_list	lexs_;

		int				width_;
        int				height_;

	public:
		compiled_text();

		void push_back(std::shared_ptr<tew_lex> _lex);
		int draw(QPainter& _painter, int _x, int _y, int _w);
        int draw(QPainter& _painter, int _x, int _y, int _w, int _h);
		int width(QPainter& _painter);
        int height(QPainter& _painter);
	};

	enum TextEmojiAlign
	{
		allign_left,
		allign_right,
		allign_center
	};

    class TextEmojiWidgetEvents;
	class TextEmojiWidget : public QWidget
	{
		Q_OBJECT

	protected:
		QFont							font_;
		QColor							color_;
		QString							text_;
		TextEmojiAlign					align_;
		std::unique_ptr<compiled_text>	compiled_text_;
		int								size_to_baseline_;
		int								descent_;
        bool                            ellipsis_;
        bool                            multiline_;
        bool                            selectable_;

        QPoint                          selection_;

		virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent* _e) override;

        virtual void mousePressEvent(QMouseEvent *_e) override;
        virtual void mouseDoubleClickEvent(QMouseEvent *_e) override;
        virtual void keyPressEvent(QKeyEvent *_e) override;
        virtual void focusOutEvent(QFocusEvent *_e) override;

    Q_SIGNALS:
        void clicked();
        
    public:
        TextEmojiWidget(QWidget* _parent, Utils::FontsFamily _font_family, int _font_size, const QColor& _color, int _size_to_baseline = -1);
		virtual ~TextEmojiWidget();

		int ascent();
		int descent();

		void setText(const QString& _text, TextEmojiAlign _align = TextEmojiAlign::allign_left);
        void setText(const QString& _text, const QColor& _color, TextEmojiAlign _align = TextEmojiAlign::allign_left);
        inline QString text() const { return text_; }

        void setColor(const QColor& _color);

        void set_ellipsis(bool _v);
        void set_multiline(bool _v);
        void set_selectable(bool _v);

        void setSizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver);

    private:
        static TextEmojiWidgetEvents& events();
        
        Utils::SignalsDisconnector disconnector_;
	};

    class TextEmojiWidgetEvents : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void selected(TextEmojiWidget*);

    private:
        TextEmojiWidgetEvents() { ; }
        TextEmojiWidgetEvents(const TextEmojiWidgetEvents&);
        TextEmojiWidgetEvents(TextEmojiWidgetEvents&&);

        friend class TextEmojiWidget;
    };

    //

    class TextEmojiLabel : public LabelEx
    {
    private:
        std::unique_ptr< compiled_text >    compiled_text_;
        int                                 size_to_baseline_;
        int                                 ascent_;
        int                                 descent_;
        int                                 left_offset_;

        void paintEvent(QPaintEvent* event) override;

    protected:
        void internalDraw(QPainter& painter, const QRect& rc);
        int internalWidth(QPainter& painter);

    public:
        TextEmojiLabel(QWidget* _parent = nullptr);
        ~TextEmojiLabel();

        void setSizeToBaseline(int size);
        void setText(const QString& text);
        void setSizeToOffset(int size);

        int ascent() const
        {
            return ascent_;
        }
        int descent() const
        {
            return descent_;
        }
    };

}


