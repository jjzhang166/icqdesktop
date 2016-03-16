#pragma once

class QPushButton;

namespace Themes
{
    class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;
}

namespace Emoji
{
    struct EmojiRecord;
    typedef std::shared_ptr<EmojiRecord> EmojiRecordSptr;
    typedef std::vector<EmojiRecordSptr> EmojiRecordSptrVec;
}

namespace Ui
{
    namespace stickers
    {
        class set;
        class sticker;
        typedef std::vector<std::shared_ptr<stickers::set>> sets_array;
        typedef std::shared_ptr<set> set_sptr;
    }

    class smiles_Widget;

    namespace Smiles
    {
        class TabButton;
        class Toolbar;

        struct emoji_category
        {
            QString									name_;
            const Emoji::EmojiRecordSptrVec&		emojis_;

            emoji_category(const QString& _name, const Emoji::EmojiRecordSptrVec& _emojis)
                :	name_(_name), emojis_(_emojis)
            {
            }
        };

        class EmojiViewItemModel : public QStandardItemModel
        {
            QSize	prev_size_;
            int		emojis_count_;
            int		need_height_;
            bool	single_line_;
            int		spacing_;

            std::vector<emoji_category>		emoji_categories_;

            QVariant data(const QModelIndex &idx, int role) const;

        public:

            EmojiViewItemModel(QWidget* _parent, bool _single_line = false);
            ~EmojiViewItemModel();

            int add_category(const QString& _category);
            int add_category(const emoji_category& _category);
            bool resize(const QSize& _size, bool _force = false);
            int get_emojis_count() const;
            int get_need_height() const;
            int get_category_pos(int _index);
            const std::vector<emoji_category>& get_categories() const;
            void on_emoji_added();
            int spacing() const;
            void setSpacing(int _spacing);

            Emoji::EmojiRecordSptr get_emoji(int _col, int _row) const;

        };

        class EmojiTableItemDelegate : public QItemDelegate
        {
            void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
            QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
        };

        class EmojiTableView : public QTableView
        {
            EmojiViewItemModel*		model_;
            EmojiTableItemDelegate* itemDelegate_;

        public:

            EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model);
            ~EmojiTableView();

            virtual void resizeEvent(QResizeEvent * _e);

            int add_category(const QString& _category);
            int add_category(const emoji_category& _category);
            int get_category_pos(int _index);
            const std::vector<emoji_category>& get_categories() const;
            Emoji::EmojiRecordSptr get_emoji(int _col, int _row) const;
            void on_emoji_added();

        };

        class EmojisWidget : public QWidget
        {
        Q_OBJECT

            EmojiTableView* view_;

            Toolbar* toolbar_;
            std::vector<TabButton*> buttons_;

            void place_toolbar(const QRect& _view_rect);

        Q_SIGNALS:

            void emoji_selected(Emoji::EmojiRecordSptr _emoji);
            void scroll_to_group(const int _pos);

        public:
            void on_viewport_changed(const QRect& _rect, bool _block_toolbar_switch);
            void select_first_button();
            EmojisWidget(QWidget* _parent);
            ~EmojisWidget();
        };



        class RecentsStickersTable;

        
        class RecentsWidget : public QWidget
        {
        Q_OBJECT
        Q_SIGNALS:
            void emoji_selected(Emoji::EmojiRecordSptr _emoji);
            void sticker_selected(qint32 _set_id, qint32 _sticker_id);

        private Q_SLOTS:
            void stickers_event();

        private:
            Emoji::EmojiRecordSptrVec emojis_;

            QVBoxLayout* v_layout_;
            EmojiTableView* emoji_view_;
            RecentsStickersTable* stickers_view_;

            void init();

        public:

            RecentsWidget(QWidget* _parent);
            ~RecentsWidget();

            void add_sticker(int32_t _set_id, int32_t _sticker_id);
            void add_emoji(Emoji::EmojiRecordSptr _emoji);
            void init_emojis_from_settings();
            void init_stickers_from_settings();
            void on_sticker_updated(int32_t _set_id, int32_t _sticker_id);
        };




        class StickersTable : public QWidget
        {
            Q_OBJECT
            Q_SIGNALS:

            void sticker_selected(qint32 _set_id, qint32 _sticker_id);

        protected:
            int32_t stickers_set_id_;

            int need_height_;
            
            QSize prev_size_;
            int column_count_;
            int row_count_;

            Themes::IThemePixmapSptr preloader_;

            virtual void resizeEvent(QResizeEvent * _e) override;
            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;

            QRect& get_sticker_rect(int _index);

            virtual bool resize(const QSize& _size, bool _force = false);
            int get_need_height() const;

        public:

            virtual void on_sticker_updated(int32_t _set_id, int32_t _sticker_id);
            void on_sticker_added();

            StickersTable(QWidget* _parent, int32_t _stickers_set_id);
            virtual ~StickersTable();
        };

        typedef std::vector<std::pair<int32_t, int32_t>> recent_stickers_array;

        class RecentsStickersTable : public StickersTable
        {
            recent_stickers_array recent_stickers_array_;

            int max_row_count_;

            virtual bool resize(const QSize& _size, bool _force = false) override;
            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;

        public:

            bool add_sticker(int32_t _set_id, int32_t _sticker_id);
            virtual void on_sticker_updated(int32_t _set_id, int32_t _sticker_id) override;

            void set_max_row_count(int _val);

            const recent_stickers_array& get_stckers() const;

            RecentsStickersTable(QWidget* _parent);
            virtual ~RecentsStickersTable();
        };



        class StickersWidget : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:

            void sticker_selected(int32_t _set_id, int32_t _sticker_id);
            void scroll_to_set(int _pos);

        private Q_SLOTS:

        private:

            void insert_next_set(int32_t _set_id);

            stickers::sets_array::const_iterator	iter_current_set_;

            QVBoxLayout* v_layout_;
            Toolbar* toolbar_;
            std::map<int32_t, StickersTable*> set_tables_;

            bool initialized_;

        public:

            void init();
            void on_sticker_updated(int32_t _set_id, int32_t _sticker_id);

            StickersWidget(QWidget* _parent, Toolbar* _toolbar);
            ~StickersWidget();
        };


        class SmilesMenu : public QFrame
        {
            Q_OBJECT
            Q_SIGNALS:

            void emoji_selected(int32_t _main, int32_t _ext);
            void sticker_selected(int32_t _set_id, int32_t _sticker_id);

        private Q_SLOTS:

                void im_created();
                void touchScrollStateChanged(QScroller::State);
                void stickers_meta_event();
                void sticker_event(qint32 _set_id, qint32 _sticker_id);

        public:

            SmilesMenu(QWidget* parent);
            ~SmilesMenu();

            void Show();
            void Hide();
            void ShowHide();
            bool IsHidden() const;

            Q_PROPERTY(int current_height READ get_current_height WRITE set_current_height)

            void set_current_height(int _val);
            int get_current_height() const;

        private:

            Toolbar* top_toolbar_;
            Toolbar* bottom_toolbar_;

            QScrollArea* view_area_;

            RecentsWidget* recents_view_;
            EmojisWidget* emoji_view_;
            StickersWidget* stickers_view_;
            QVBoxLayout* rootVerticalLayout_;
            QPropertyAnimation* anim_height_;
            QPropertyAnimation* anim_scroll_;

            bool is_visible_;
            bool block_toolbar_switch_;

            bool sticker_meta_requested_;

            void InitSelector();
            void InitStickers();
            void InitResents();

            void ScrollTo(int _pos);
            void HookScroll();

        protected:

            int current_height_;

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void focusOutEvent(QFocusEvent* _event) override;
            virtual void focusInEvent(QFocusEvent* _event) override;
            virtual void resizeEvent(QResizeEvent * _e) override;

        };
    }
}