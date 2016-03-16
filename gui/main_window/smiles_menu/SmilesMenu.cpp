#include "stdafx.h"

#include "../../../corelib/enumerations.h"

#include "../../main_window/MainWindow.h"
#include "SmilesMenu.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../cache/emoji/Emoji.h"
#include "../../cache/emoji/EmojiDb.h"
#include "../../cache/stickers/stickers.h"
#include "toolbar.h"
#include "../../utils/gui_coll_helper.h"

#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"

namespace Ui
{
    const int32_t max_stickers_count = 20;

    namespace
    {
        qint32 getEmojiItemSize();

        qint32 getStickerItemSize();

        qint32 getStickerSize();
    }

    using namespace Smiles;

    Emoji::EmojiSizePx get_picker_emoji_size()
    {
        Emoji::EmojiSizePx emoji_size = Emoji::EmojiSizePx::_32;
        int scale = (int) (Utils::get_scale_coefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
        case 100:
            emoji_size = Emoji::EmojiSizePx::_32;
            break;
        case 125:
            emoji_size = Emoji::EmojiSizePx::_40;
            break;
        case 150:
            emoji_size = Emoji::EmojiSizePx::_48;
            break;
        case 200:
            emoji_size = Emoji::EmojiSizePx::_64;
            break;
        default:
            assert(!"invalid scale");
        }

        return emoji_size;
    }


    //////////////////////////////////////////////////////////////////////////
    // class ViewItemModel
    //////////////////////////////////////////////////////////////////////////
    EmojiViewItemModel::EmojiViewItemModel(QWidget* _parent, bool _single_line)
        :	QStandardItemModel(_parent),
        emojis_count_(0),
        need_height_(0),
        single_line_(_single_line),
        spacing_(12)
    {
        emoji_categories_.reserve(10);
    }

    EmojiViewItemModel::~EmojiViewItemModel()
    {
    }

    int EmojiViewItemModel::spacing() const
    {
        return spacing_;
    }

    void EmojiViewItemModel::setSpacing(int _spacing)
    {
        spacing_ = _spacing;
    }

    Emoji::EmojiRecordSptr EmojiViewItemModel::get_emoji(int _col, int _row) const
    {
        int index = _row * columnCount() + _col;

        if (index < get_emojis_count())
        {
            int count = 0;
            for (const auto& category : emoji_categories_)
            {
                if ((count + (int) category.emojis_.size()) > index)
                {
                    int index_in_category = index - count;

                    assert(index_in_category >= 0);
                    assert(index_in_category < (int) category.emojis_.size());

                    return category.emojis_[index_in_category];
                }

                count += category.emojis_.size();
            }
        }

        assert(!"invalid emoji number");

        return nullptr;
    }

    QVariant EmojiViewItemModel::data(const QModelIndex& _idx, int _role) const
    {
        if (_role == Qt::DecorationRole)
        {
            int index = _idx.row() * columnCount() + _idx.column();
            if (index < get_emojis_count())
            {
                auto emoji = get_emoji(_idx.column(), _idx.row());
                if (emoji)
                {
                    auto emoji_ = Emoji::GetEmoji(emoji->Codepoint_, emoji->ExtendedCodepoint_, get_picker_emoji_size());
                    QPixmap emojiPixmap = QPixmap::fromImage(emoji_);
                    Utils::check_pixel_ratio(emojiPixmap);
                    return emojiPixmap;
                }
            }
        }

        return QVariant();
    }

    int EmojiViewItemModel::add_category(const QString& _category)
    {
        const Emoji::EmojiRecordSptrVec& emojis_vector = Emoji::GetEmojiInfoByCategory(_category);
        emoji_categories_.emplace_back(_category, emojis_vector);

        emojis_count_ += emojis_vector.size();

        resize(prev_size_);

        return ((int)emoji_categories_.size() - 1);
    }

    int EmojiViewItemModel::add_category(const emoji_category& _category)
    {
        emoji_categories_.push_back(_category);

        emojis_count_ += _category.emojis_.size();

        return ((int)emoji_categories_.size() - 1);
    }

    int EmojiViewItemModel::get_emojis_count() const
    {
        return emojis_count_;
    }

    int EmojiViewItemModel::get_need_height() const
    {
        return need_height_;
    }

    int EmojiViewItemModel::get_category_pos(int _index)
    {
        const int column_width = getEmojiItemSize();
        int columnCount = prev_size_.width() / column_width;

        int emoji_count_before = 0;

        for (int i = 0; i < _index; i++)
            emoji_count_before += emoji_categories_[i].emojis_.size();

        if (columnCount == 0)
            return 0;

        int rowCount = (emoji_count_before / columnCount) + (((emoji_count_before % columnCount) > 0) ? 1 : 0);

        return (((rowCount == 0) ? 0 : (rowCount - 1)) * getEmojiItemSize());
    }

    const std::vector<emoji_category>& EmojiViewItemModel::get_categories() const
    {
        return emoji_categories_;
    }

    bool EmojiViewItemModel::resize(const QSize& _size, bool _force)
    {
        const int column_width = getEmojiItemSize();
        int emoji_count = get_emojis_count();

        bool resized = false;

        if ((prev_size_.width() != _size.width() && _size.width() > column_width) || _force)
        {
            int columnCount = _size.width()/column_width;
            if (columnCount > emoji_count)
                columnCount = emoji_count;

            int rowCount  = 0;
            if (columnCount > 0)
            {
                rowCount = (emoji_count / columnCount) + (((emoji_count % columnCount) > 0) ? 1 : 0);
                if (single_line_ && rowCount > 1)
                    rowCount = 1;
            }

            setColumnCount(columnCount);
            setRowCount(rowCount);

            need_height_ = getEmojiItemSize() * rowCount;

            resized = true;
        }

        prev_size_ = _size;

        return resized;
    }

    void EmojiViewItemModel::on_emoji_added()
    {
        emojis_count_ = 0;
        for (uint32_t i = 0; i < emoji_categories_.size(); i++)
            emojis_count_ += emoji_categories_[i].emojis_.size();
    }

    //////////////////////////////////////////////////////////////////////////
    // TableView class
    //////////////////////////////////////////////////////////////////////////
    EmojiTableView::EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model)
        :	QTableView(_parent), model_(_model), itemDelegate_(new EmojiTableItemDelegate)
    {
        setModel(model_);
        setItemDelegate(itemDelegate_);
        setShowGrid(false);
        verticalHeader()->hide();
        horizontalHeader()->hide();
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        verticalHeader()->setDefaultSectionSize(getEmojiItemSize());
        horizontalHeader()->setDefaultSectionSize(getEmojiItemSize());
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFocusPolicy(Qt::NoFocus);
        setSelectionMode(QAbstractItemView::NoSelection);
        setCursor(QCursor(Qt::PointingHandCursor));
    }

    EmojiTableView::~EmojiTableView()
    {
    }

    void EmojiTableView::resizeEvent(QResizeEvent * _e)
    {
        if (model_->resize(_e->size()))
            setFixedHeight(model_->get_need_height());

        QTableView::resizeEvent(_e);
    }

    int EmojiTableView::add_category(const QString& _category)
    {
        return model_->add_category(_category);
    }

    int EmojiTableView::add_category(const emoji_category& _category)
    {
        return model_->add_category(_category);
    }

    int EmojiTableView::get_category_pos(int _index)
    {
        return model_->get_category_pos(_index);
    }

    const std::vector<emoji_category>& EmojiTableView::get_categories() const
    {
        return model_->get_categories();
    }

    Emoji::EmojiRecordSptr EmojiTableView::get_emoji(int _col, int _row) const
    {
        return model_->get_emoji(_col, _row);
    }

    void EmojiTableView::on_emoji_added()
    {
        model_->on_emoji_added();

        QRect rect = geometry();

        if (model_->resize(QSize(rect.width(), rect.height()), true))
            setFixedHeight(model_->get_need_height());
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiTableItemDelegate
    //////////////////////////////////////////////////////////////////////////
    void EmojiTableItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem&, const QModelIndex &index) const
    {
        const EmojiViewItemModel *itemModel = (EmojiViewItemModel *)index.model();
        QPixmap data = index.data(Qt::DecorationRole).value<QPixmap>();
        int col = index.column();
        int row = index.row();
        int spacing = itemModel->spacing();
        int size = (int)get_picker_emoji_size() / Utils::scale_bitmap(1);
        painter->drawPixmap(col * (size + Utils::scale_value(spacing)), row * (size + Utils::scale_value(spacing)), size, size, data);
    }

    QSize EmojiTableItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
    {
        int size = (int)get_picker_emoji_size() / Utils::scale_bitmap(1);
        return QSize(size, size);
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiViewWidget
    //////////////////////////////////////////////////////////////////////////
    EmojisWidget::EmojisWidget(QWidget* _parent)
        :	QWidget(_parent)
    {
        QVBoxLayout* v_layout = new QVBoxLayout();
        v_layout->setContentsMargins(0, 0, 0, 0);
        setLayout(v_layout);

        QLabel* set_header = new QLabel(this);
        set_header->setObjectName("set_header");
        set_header->setText(QT_TRANSLATE_NOOP("input_widget", "EMOJI"));
        v_layout->addWidget(set_header);

        view_ = new EmojiTableView(this, new EmojiViewItemModel(this));
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        Utils::grabTouchWidget(view_);

        toolbar_ = new Toolbar(this, buttons_align::center);
        toolbar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        toolbar_->setFixedHeight(Utils::scale_value(48));
        toolbar_->setObjectName("smiles_cat_selector");

        TabButton* first_button = nullptr;

        QStringList emoji_categories = Emoji::GetEmojiCategories();

        buttons_.reserve(emoji_categories.size());

        for (auto category : emoji_categories)
        {
            QString resource_string = QString(":/resources/smiles_menu/picker_emojitab_") + category + "_100.png";
            TabButton* button = toolbar_->addButton(resource_string);
            button->setProperty("underline", false);
            buttons_.push_back(button);

            int category_index = view_->add_category(category);

            if (!first_button)
                first_button = button;

            connect(button, &TabButton::clicked, [this, category_index]()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_cathegory_click);
                emit scroll_to_group(view_->geometry().top() + view_->get_category_pos(category_index));
            });
        }

        v_layout->addWidget(view_);

        QWidget* toolbar_place = new QWidget(this);
        toolbar_place->setObjectName("red_marker");
        toolbar_place->setFixedHeight(toolbar_->height());
        v_layout->addWidget(toolbar_place);

        if (first_button)
            first_button->toggle();

        connect(view_, &EmojiTableView::clicked, [this](const QModelIndex & _index)
        {
            auto emoji = view_->get_emoji(_index.column(), _index.row());
            if (emoji)
            {
                emit emoji_selected(emoji);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_picker);
            }
        });

        toolbar_->raise();
    }

    EmojisWidget::~EmojisWidget()
    {
    }

    void EmojisWidget::on_viewport_changed(const QRect& _view_rect, bool _block_toolbar_switch)
    {
        if (!_block_toolbar_switch)
        {
            auto categories = view_->get_categories();
            for (uint32_t i = 0; i < categories.size(); i++)
            {
                int pos = view_->get_category_pos(i);

                if (_view_rect.top() < pos && pos < (_view_rect.top() + _view_rect.width()/2))
                {
                    buttons_[i]->setChecked(true);
                    break;
                }
            }
        }

        place_toolbar(_view_rect);
    }

    void EmojisWidget::place_toolbar(const QRect& _view_rect)
    {
        int h = toolbar_->height();

        QRect this_rect = rect();

        int y = _view_rect.bottom() - h;
        if (_view_rect.bottom() > this_rect.bottom() || ((-1 *(_view_rect.top()) > (_view_rect.height() / 2))))
            y = this_rect.bottom() - h;

        toolbar_->setGeometry(0, y, this_rect.width(), h);
    }

    void EmojisWidget::select_first_button()
    {
        if (buttons_.size() > 0)
        {
            buttons_[0]->setChecked(true);
        }
    }









    StickersTable::StickersTable(QWidget* _parent, int32_t _stickers_set_id)
        :	QWidget(_parent),
        need_height_(0),
        stickers_set_id_(_stickers_set_id),
        column_count_(0),
        row_count_(0)
    {
        setCursor(QCursor(Qt::PointingHandCursor));

        preloader_ = Themes::GetPixmap(Themes::PixmapResourceId::StickerPickerPlaceholder);
        assert(preloader_);
    }

    StickersTable::~StickersTable()
    {
    }

    void StickersTable::resizeEvent(QResizeEvent * _e)
    {
        if (resize(_e->size()))
            setFixedHeight(get_need_height());

        QWidget::resizeEvent(_e);
    }

    void StickersTable::on_sticker_added()
    {
        QRect rect = geometry();

        if (resize(QSize(rect.width(), rect.height()), true))
            setFixedHeight(get_need_height());
    }

    bool StickersTable::resize(const QSize& _size, bool _force)
    {
        const int column_width = getStickerItemSize();
        
        int stickers_count = (int) stickers::get_set_stickers_count(stickers_set_id_);

        bool resized = false;

        if ((prev_size_.width() != _size.width() && _size.width() > column_width) || _force)
        {
            column_count_ = _size.width()/column_width;
            if (column_count_ > stickers_count)
                column_count_ = stickers_count;

            row_count_ = 0;
            if (stickers_count > 0 && column_count_ > 0)
            {
                row_count_ = (stickers_count / column_count_) + (((stickers_count % column_count_) > 0) ? 1 : 0);
            }

            need_height_ = getStickerItemSize() * row_count_;

            resized = true;
        }

        prev_size_ = _size;

        return resized;
    }

    int StickersTable::get_need_height() const
    {
        return need_height_;
    }

    void StickersTable::on_sticker_updated(int32_t _set_id, int32_t _sticker_id)
    {
        auto sticker_pos_in_set = stickers::get_sticker_pos_in_set(_set_id, _sticker_id);
        if (sticker_pos_in_set >= 0)
            update(get_sticker_rect(sticker_pos_in_set));
    }

    QRect& StickersTable::get_sticker_rect(int _index)
    {
        static QRect rect;

        if (column_count_ > 0 && row_count_ > 0)
        {
            int item_y = _index / column_count_;
            int item_x = _index % column_count_;

            int item_size = getStickerItemSize();
            int stick_size = getStickerSize();

            int x = item_size * item_x;
            int y = item_size * item_y;

            int space = ((getStickerItemSize() - getStickerSize()) / 2);

            rect.setRect(x + space, y + space, stick_size, stick_size);
        }
        else
        {
            rect.setRect(0, 0, 0, 0);
        }

        return rect;
    }

    void StickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        const auto& stickers_list = stickers::get_stickers(stickers_set_id_);

        for (uint32_t i = 0; i < stickers_list.size(); ++i)
        {
            const auto& sticker_rect = get_sticker_rect(i);
            if (!_e->rect().intersects(sticker_rect))
            {
                continue;
            }

            auto image = stickers::get_sticker_image(stickers_set_id_, stickers_list[i], core::sticker_size::small);
            if (image.isNull())
            {
                assert(preloader_);
                preloader_->Draw(p, sticker_rect);

                continue;
            }

            if (platform::is_apple() && Utils::is_mac_retina())
            {
                QSize newSize = image.size() / 2;

                if (newSize.width() > sticker_rect.size().width() ||
                    newSize.height() > sticker_rect.size().height())
                {
                    newSize = image.size().scaled(sticker_rect.size(), Qt::KeepAspectRatio);
                }

                QRect target_rect(
                                  sticker_rect.left(),
                                  sticker_rect.top(),
                                  newSize.width(),
                                  newSize.height()
                                  );

                target_rect.moveCenter(sticker_rect.center());

                p.drawImage(target_rect, image, image.rect());
            }
            else
            {
                auto pixmap = QPixmap::fromImage(image);
                pixmap = pixmap.scaled(sticker_rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

                QRect target_rect(
                    sticker_rect.left(),
                    sticker_rect.top(),
                    pixmap.width(),
                    pixmap.height()
                );

                target_rect.moveCenter(sticker_rect.center());
                p.drawPixmap(target_rect, pixmap);
            }
        }

        return QWidget::paintEvent(_e);
    }

    void StickersTable::mousePressEvent(QMouseEvent* _e)
    {
        const auto& stickers_list = stickers::get_stickers(stickers_set_id_);

        for (uint32_t i = 0; i < stickers_list.size(); ++i)
        {
            if (get_sticker_rect(i).contains(_e->pos()))
            {
                emit sticker_selected(stickers_set_id_, stickers_list[i]);
            }
        }

        return QWidget::mousePressEvent(_e);
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentsStickersTable
    //////////////////////////////////////////////////////////////////////////
    RecentsStickersTable::RecentsStickersTable(QWidget* _parent)
        :   StickersTable(_parent, -1),
            max_row_count_(-1)
    {

    }

    RecentsStickersTable::~RecentsStickersTable()
    {

    }

    void RecentsStickersTable::set_max_row_count(int _val)
    {
        max_row_count_ = _val;
    }

    bool RecentsStickersTable::resize(const QSize& _size, bool _force)
    {
        const int column_width = getStickerItemSize();

        int stickers_count = (int) recent_stickers_array_.size();

        bool resized = false;

        if ((prev_size_.width() != _size.width() && _size.width() > column_width) || _force)
        {
            column_count_ = _size.width()/column_width;
            if (column_count_ > stickers_count)
                column_count_ = stickers_count;

            row_count_ = 0;
            if (stickers_count > 0 && column_count_ > 0)
            {
                row_count_ = (stickers_count / column_count_) + (((stickers_count % column_count_) > 0) ? 1 : 0);
                if (max_row_count_ > 0 && row_count_ > max_row_count_)
                    row_count_ = max_row_count_;
            }

            need_height_ = getStickerItemSize() * row_count_;

            resized = true;
        }

        prev_size_ = _size;

        return resized;
    }

    void RecentsStickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        for (uint32_t i = 0; i < recent_stickers_array_.size(); ++i)
        {
            const auto& sticker_rect = get_sticker_rect(i);
            if (!_e->rect().intersects(sticker_rect))
            {
                continue;
            }

            auto image = stickers::get_sticker_image(recent_stickers_array_[i].first, recent_stickers_array_[i].second, core::sticker_size::small);
            if (image.isNull())
            {
                assert(preloader_);
                preloader_->Draw(p, sticker_rect);

                continue;
            }

            if (platform::is_apple() && Utils::is_mac_retina())
            {
                QSize newSize = image.size() / 2;

                if (newSize.width() > sticker_rect.size().width() ||
                    newSize.height() > sticker_rect.size().height())
                {
                    newSize = image.size().scaled(sticker_rect.size(), Qt::KeepAspectRatio);
                }

                QRect target_rect(
                    sticker_rect.left(),
                    sticker_rect.top(),
                    newSize.width(),
                    newSize.height()
                    );

                target_rect.moveCenter(sticker_rect.center());

                p.drawImage(target_rect, image, image.rect());
            }
            else
            {
                auto pixmap = QPixmap::fromImage(image);
                pixmap = pixmap.scaled(sticker_rect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

                QRect target_rect(
                    sticker_rect.left(),
                    sticker_rect.top(),
                    pixmap.width(),
                    pixmap.height()
                    );

                target_rect.moveCenter(sticker_rect.center());
                p.drawPixmap(target_rect, pixmap);
            }
        }

        return QWidget::paintEvent(_e);
    }

    bool RecentsStickersTable::add_sticker(int32_t _set_id, int32_t _sticker_id)
    {
        for (auto iter = recent_stickers_array_.begin(); iter != recent_stickers_array_.end(); ++iter)
        {
            if (iter->first == _set_id && iter->second == _sticker_id)
            {
                if (iter == recent_stickers_array_.begin())
                    return false;

                recent_stickers_array_.erase(iter);
                break;
            }
        }

        recent_stickers_array_.emplace(recent_stickers_array_.begin(), _set_id, _sticker_id);

        if ((int32_t) recent_stickers_array_.size() > max_stickers_count)
            recent_stickers_array_.pop_back();

        return true;
    }

    void RecentsStickersTable::on_sticker_updated(int32_t _set_id, int32_t _sticker_id)
    {
        for (uint32_t i = 0; i < recent_stickers_array_.size(); ++i)
        {
            if (recent_stickers_array_[i].first == _set_id && recent_stickers_array_[i].second == _sticker_id)
            {
                update(get_sticker_rect(i));
                break;
            }
        }
    }

    void RecentsStickersTable::mousePressEvent(QMouseEvent* _e)
    {
        for (uint32_t i = 0; i < recent_stickers_array_.size(); ++i)
        {
            if (get_sticker_rect(i).contains(_e->pos()))
            {
                emit sticker_selected(recent_stickers_array_[i].first, recent_stickers_array_[i].second);
                break;
            }
        }

        return QWidget::mousePressEvent(_e);
    }

    const recent_stickers_array& RecentsStickersTable::get_stckers() const
    {
        return recent_stickers_array_;
    }

    //////////////////////////////////////////////////////////////////////////
    // class StickersWidget
    //////////////////////////////////////////////////////////////////////////


    StickersWidget::StickersWidget(QWidget* _parent, Toolbar* _toolbar)
        :	QWidget(_parent), toolbar_(_toolbar), initialized_(false)
    {
    }

    StickersWidget::~StickersWidget()
    {
    }

    void StickersWidget::insert_next_set(int32_t _set_id)
    {
        if (set_tables_.find(_set_id) != set_tables_.end())
            return;

        auto stickers_view = new StickersTable(this, _set_id);
        auto button = toolbar_->addButton(stickers::get_set_icon(_set_id));
        button->AttachView(AttachedView(stickers_view, this));

        connect(button, &TabButton::clicked, [this, _set_id]()
        {
            emit scroll_to_set(set_tables_[_set_id]->geometry().top());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        QLabel* set_header = new QLabel(this);
        set_header->setObjectName("set_header");
        set_header->setText(stickers::get_set_name(_set_id));
        v_layout_->addWidget(set_header);

        v_layout_->addWidget(stickers_view);
        set_tables_[_set_id] = stickers_view;
        Utils::grabTouchWidget(stickers_view);

        connect(stickers_view, &StickersTable::sticker_selected, [this, stickers_view](int32_t _set_id, int32_t _sticker_id)
        {
            emit sticker_selected(_set_id, _sticker_id);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_picker);
        });
    }

    void StickersWidget::init()
    {
        if (!initialized_)
        {
            v_layout_ = new QVBoxLayout();
            v_layout_->setContentsMargins(0, 0, 0, 0);
        }
        
        for (auto stickers_set_id : stickers::get_stickers_sets())
        {
            insert_next_set(stickers_set_id);
        }

        if (!initialized_)
        {
            setLayout(v_layout_);
        }

        initialized_ = true;
    }

    void StickersWidget::on_sticker_updated(int32_t _set_id, int32_t _sticker_id)
    {
        auto iter_set = set_tables_.find(_set_id);
        if (iter_set == set_tables_.end())
        {
            return;
        }

        iter_set->second->on_sticker_updated(_set_id, _sticker_id);
    }

    //////////////////////////////////////////////////////////////////////////
    // Recents widget
    //////////////////////////////////////////////////////////////////////////
    RecentsWidget::RecentsWidget(QWidget* _parent)
        :	QWidget(_parent),
        v_layout_(nullptr),
        emoji_view_(nullptr),
        stickers_view_(nullptr)
    {
        init_emojis_from_settings();

        connect(Ui::GetDispatcher(), SIGNAL(on_stickers()), this, SLOT(stickers_event()));
    }

    RecentsWidget::~RecentsWidget()
    {
    }

    void RecentsWidget::stickers_event()
    {
        init_stickers_from_settings();
    }

    void RecentsWidget::init()
    {
        if (v_layout_)
            return;

        v_layout_ = new QVBoxLayout();
        v_layout_->setContentsMargins(0, 0, 0, 0);

        QLabel* set_header = new QLabel(this);
        set_header->setObjectName("set_header");
        set_header->setText(QT_TRANSLATE_NOOP("input_widget", "RECENTS"));
        v_layout_->addWidget(set_header);

         stickers_view_ = new RecentsStickersTable(this);
         stickers_view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
         stickers_view_->set_max_row_count(2);
         v_layout_->addWidget(stickers_view_);
         Utils::grabTouchWidget(stickers_view_);

        emoji_view_ = new EmojiTableView(this, new EmojiViewItemModel(this, true));
        emoji_view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emoji_view_->add_category(emoji_category("recents", emojis_));
        v_layout_->addWidget(emoji_view_);
        Utils::grabTouchWidget(emoji_view_);

        setLayout(v_layout_);

        connect(emoji_view_, &EmojiTableView::clicked, [this](const QModelIndex & _index)
        {
            auto emoji = emoji_view_->get_emoji(_index.column(), _index.row());
            if (emoji)
            {
                emit emoji_selected(emoji);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_from_recents);
            }
        });

        connect(stickers_view_, &RecentsStickersTable::sticker_selected, [this](qint32 _set_id, qint32 _sticker_id)
        {
            emit sticker_selected(_set_id, _sticker_id);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_recents);
        });
    }

    void RecentsWidget::add_sticker(int32_t _set_id, int32_t _sticker_id)
    {
        init();

        if (stickers_view_->add_sticker(_set_id, _sticker_id))
        {
            stickers_view_->on_sticker_added();

            const auto& recents_stickers = stickers_view_->get_stckers();

            std::vector<int32_t> v_stickers;
            v_stickers.reserve(recents_stickers.size());
            for (uint32_t i = 0; i < recents_stickers.size(); ++i)
            {
                v_stickers.push_back(recents_stickers[i].first);
                v_stickers.push_back(recents_stickers[i].second);
            }

            get_gui_settings()->set_value<std::vector<int32_t>>(settings_recents_stickers, v_stickers);
        }
    }

    void RecentsWidget::init_stickers_from_settings()
    {
        static int count = 0;

        ++count;

        auto sticks = get_gui_settings()->get_value<std::vector<int32_t>>(settings_recents_stickers, std::vector<int32_t>());
        if (sticks.size() == 0 || (sticks.size() % 2 != 0))
            return;

        init();

        for (uint32_t i = 0; i < sticks.size();)
        {
            int32_t set_id = sticks[i];
            int32_t sticker_id = sticks[++i];

            ++i;

            stickers_view_->add_sticker(set_id, sticker_id);
        }

        stickers_view_->on_sticker_added();
    }

    void RecentsWidget::add_emoji(Emoji::EmojiRecordSptr _emoji)
    {
        init();

        for (auto iter = emojis_.begin(); iter != emojis_.end(); iter++)
        {
            if (_emoji->Codepoint_ == (*iter)->Codepoint_ && _emoji->ExtendedCodepoint_ == (*iter)->ExtendedCodepoint_)
            {
                if (iter == emojis_.begin())
                    return;

                emojis_.erase(iter);
                break;
            }
        }

        emojis_.insert(emojis_.begin(), _emoji);
        if (emojis_.size() > 20)
            emojis_.pop_back();

        emoji_view_->on_emoji_added();

        std::vector<int32_t> v_emojis;
        v_emojis.reserve(emojis_.size() * 2);
        for (uint32_t i = 0; i < emojis_.size(); i++)
        {
            v_emojis.push_back(emojis_[i]->Codepoint_);
            v_emojis.push_back(emojis_[i]->ExtendedCodepoint_);
        }

        get_gui_settings()->set_value<std::vector<int32_t>>(settings_recents_emojis, v_emojis);
    }

    void RecentsWidget::init_emojis_from_settings()
    {
        auto emojis = get_gui_settings()->get_value<std::vector<int32_t>>(settings_recents_emojis, std::vector<int32_t>());
        if (emojis.size() == 0 || (emojis.size() % 2 != 0))
            return;

        init();

        emojis_.reserve(20);

        for (uint32_t i = 0; i < emojis.size();)
        {
            int32_t codepoint = emojis[i];
            int32_t ext_codepoint = emojis[++i];

            ++i;

            auto emoji = Emoji::GetEmojiInfoByCodepoint(codepoint, ext_codepoint);
            if (!emoji)
            {
                assert(false);
                continue;
            }

            emojis_.push_back(emoji);
        }

        emoji_view_->on_emoji_added();
    }

    void RecentsWidget::on_sticker_updated(int32_t _set_id, int32_t _sticker_id)
    {
        if (stickers_view_)
        {
            stickers_view_->on_sticker_updated(_set_id, _sticker_id);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // SmilesMenu class
    //////////////////////////////////////////////////////////////////////////

    SmilesMenu::SmilesMenu(QWidget* _parent)
        : QFrame(_parent)
        , is_visible_(false)
        , current_height_(0)
        , top_toolbar_(0)
        , bottom_toolbar_(0)
        , view_area_(0)
        , recents_view_(0)
        , emoji_view_(0)
        , stickers_view_(0)
        , anim_height_(0)
        , anim_scroll_(0)
        , block_toolbar_switch_(false)
        , sticker_meta_requested_(false)
    {
        rootVerticalLayout_ = new QVBoxLayout(this);
        rootVerticalLayout_->setContentsMargins(0, 0, 0, 0);
        setLayout(rootVerticalLayout_);

        setStyleSheet(Utils::LoadStyle(":/main_window/smiles_menu/smiles_menu.qss", Utils::get_scale_coefficient(), true));

        InitSelector();
        InitStickers();
        InitResents();

        connect(Ui::GetDispatcher(), SIGNAL(im_created()), this, SLOT(im_created()), Qt::QueuedConnection);
    }

    SmilesMenu::~SmilesMenu()
    {
    }

    void SmilesMenu::im_created()
    {
        int scale = (int) (Utils::get_scale_coefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        std::string size = "small";

        switch (scale)
        {
        case 150:
            size = "medium";
            break;
        case 200:
            size = "large";
            break;
        }

        sticker_meta_requested_ = true;

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("size", size);
        Ui::GetDispatcher()->post_message_to_core("stickers/meta/get", collection.get());
    }

    void SmilesMenu::touchScrollStateChanged(QScroller::State state)
    {
        recents_view_->blockSignals(state != QScroller::Inactive);
        emoji_view_->blockSignals(state != QScroller::Inactive);
        stickers_view_->blockSignals(state != QScroller::Inactive);
    }

    void SmilesMenu::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive (QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void SmilesMenu::focusOutEvent(QFocusEvent* _event)
    {
        Qt::FocusReason reason = _event->reason();

        if (reason != Qt::FocusReason::ActiveWindowFocusReason)
            Hide();

        QFrame::focusOutEvent(_event);
    }

    void SmilesMenu::focusInEvent(QFocusEvent* _event)
    {
        QFrame::focusInEvent(_event);
    }


    void SmilesMenu::Show()
    {
        if (is_visible_)
            return;

        ShowHide();
    }

    void SmilesMenu::Hide()
    {
        if (!is_visible_)
            return;

        ShowHide();
    }

    void SmilesMenu::ShowHide()
    {
        is_visible_ = !is_visible_;

        int start_value = is_visible_ ? 0 : Utils::scale_value(320);
        int end_value = is_visible_ ? Utils::scale_value(320) : 0;

        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 200;

        if (!anim_height_)
        {
            anim_height_ = new QPropertyAnimation(this, "current_height");

            connect(anim_height_, &QPropertyAnimation::finished, [this]()
            {
                if (!is_visible_)
                    Ui::stickers::clear_cache();
            });
        }

        if (is_visible_)
            top_toolbar_->updateArrowButtonsVisibility();

        anim_height_->stop();
        anim_height_->setDuration(duration);
        anim_height_->setStartValue(start_value);
        anim_height_->setEndValue(end_value);
        anim_height_->setEasingCurve(easing_curve);
        anim_height_->start();
    }

    bool SmilesMenu::IsHidden() const
    {
        assert(current_height_ >= 0);
        return (current_height_ == 0);
    }

    void SmilesMenu::ScrollTo(int _pos)
    {
        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 200;

        if (!anim_scroll_)
            anim_scroll_ = new QPropertyAnimation(view_area_->verticalScrollBar(), "value");

        block_toolbar_switch_ = true;
        anim_scroll_->stop();
        anim_scroll_->setDuration(duration);
        anim_scroll_->setStartValue(view_area_->verticalScrollBar()->value());
        anim_scroll_->setEndValue(_pos);
        anim_scroll_->setEasingCurve(easing_curve);
        anim_scroll_->start();

        connect(anim_scroll_, &QPropertyAnimation::finished, [this]()
        {
            block_toolbar_switch_ = false;
        });
    }

    void SmilesMenu::InitResents()
    {
        connect(recents_view_, &RecentsWidget::emoji_selected, [this](Emoji::EmojiRecordSptr _emoji)
        {
            emit emoji_selected(_emoji->Codepoint_, _emoji->ExtendedCodepoint_);
        });

        connect(recents_view_, &RecentsWidget::sticker_selected, [this](qint32 _set_id, qint32 _sticker_id)
        {
            emit sticker_selected(_set_id, _sticker_id);
        });
    }

    void SmilesMenu::stickers_meta_event()
    {
        sticker_meta_requested_ = false;
        stickers_view_->init();
    }

    void SmilesMenu::sticker_event(qint32 _set_id, qint32 _sticker_id)
    {
        stickers_view_->on_sticker_updated(_set_id, _sticker_id);
        recents_view_->on_sticker_updated(_set_id, _sticker_id);
    }

    void SmilesMenu::InitStickers()
    {
        connect(Ui::GetDispatcher(), SIGNAL(on_stickers()), this, SLOT(stickers_meta_event()));
        connect(Ui::GetDispatcher(), SIGNAL(on_sticker(qint32, qint32)), this, SLOT(sticker_event(qint32, qint32)));

        connect(stickers_view_, &StickersWidget::sticker_selected, [this](qint32 _set_id, qint32 _sticker_id)
        {
            emit sticker_selected(_set_id, _sticker_id);

            recents_view_->add_sticker(_set_id, _sticker_id);
        });

        connect(stickers_view_, &StickersWidget::scroll_to_set, [this](int _pos)
        {
            ScrollTo(stickers_view_->geometry().top() + _pos);
        });
    }


    void SmilesMenu::InitSelector()
    {
        top_toolbar_ = new Toolbar(this, buttons_align::left);
        recents_view_ = new RecentsWidget(this);
        emoji_view_ = new EmojisWidget(this);

        top_toolbar_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        top_toolbar_->setFixedHeight(Utils::scale_value(49));

        auto resents_button = top_toolbar_->addButton(":/resources/smiles_menu/picker_tab_recents_100.png");
        resents_button->AttachView(recents_view_);
        connect(resents_button, &TabButton::clicked, [this]()
        {
            ScrollTo(recents_view_->geometry().top());
            emoji_view_->select_first_button();
        });

        auto emoji_button = top_toolbar_->addButton(":/resources/smiles_menu/picker_tab_emoji_100.png");
        emoji_button->AttachView(AttachedView(emoji_view_));
        connect(emoji_button, &TabButton::clicked, [this]()
        {
            ScrollTo(emoji_view_->geometry().top());
            emoji_view_->select_first_button();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        rootVerticalLayout_->addWidget(top_toolbar_);

        view_area_ = new QScrollArea(this);
        view_area_->setFocusPolicy(Qt::NoFocus);
        Utils::grabTouchWidget(view_area_->viewport(), true);
        connect(QScroller::scroller(view_area_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

        QWidget* scroll_area_widget = new QWidget(view_area_);
        scroll_area_widget->setObjectName("scroll_area_widget");
        view_area_->setWidget(scroll_area_widget);
        view_area_->setWidgetResizable(true);
        rootVerticalLayout_->addWidget(view_area_);


        QVBoxLayout* sa_widget_layout = new QVBoxLayout();
        sa_widget_layout->setContentsMargins(Utils::scale_value(10), 0, Utils::scale_value(10), 0);
        scroll_area_widget->setLayout(sa_widget_layout);

        recents_view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sa_widget_layout->addWidget(recents_view_);
        Utils::grabTouchWidget(recents_view_);

        emoji_view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sa_widget_layout->addWidget(emoji_view_);
        Utils::grabTouchWidget(emoji_view_);

        stickers_view_ = new StickersWidget(this, top_toolbar_);
        stickers_view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sa_widget_layout->addWidget(stickers_view_);
        Utils::grabTouchWidget(stickers_view_);

        emoji_button->toggle();

        connect(emoji_view_, &EmojisWidget::emoji_selected, [this](Emoji::EmojiRecordSptr _emoji)
        {
            emit emoji_selected(_emoji->Codepoint_, _emoji->ExtendedCodepoint_);

            recents_view_->add_emoji(_emoji);
        });

        connect(emoji_view_, &EmojisWidget::scroll_to_group, [this](int _pos)
        {
            ScrollTo(emoji_view_->geometry().top() + _pos);
        });

        HookScroll();
    }

    void SmilesMenu::HookScroll()
    {
        connect(view_area_->verticalScrollBar(), &QAbstractSlider::valueChanged, [this](int _value)
        {
            QRect view_port_rect = view_area_->viewport()->geometry();
            QRect emoji_rect = emoji_view_->geometry();
            QRect view_port_rect_for_emoji(0, _value - emoji_rect.top(), view_port_rect.width(), view_port_rect.height() + 1);

            emoji_view_->on_viewport_changed(view_port_rect_for_emoji, block_toolbar_switch_);

            view_port_rect.adjust(0, _value, 0, _value);

            if (block_toolbar_switch_)
                return;

            for (TabButton* button : top_toolbar_->GetButtons())
            {
                auto view = button->GetAttachedView();
                if (view.get_view())
                {
                    QRect rc_view = view.get_view()->geometry();

                    if (view.get_view_parent())
                    {
                        QRect rc_view_parent = view.get_view_parent()->geometry();
                        rc_view.adjust(0, rc_view_parent.top(), 0, rc_view_parent.top());
                    }

                    QRect intersected_rect = rc_view.intersected(view_port_rect);

                    if (intersected_rect.height() > (view_port_rect.height() / 2))
                    {
                        button->setChecked(true);
                        top_toolbar_->scrollToButton(button);
                        break;
                    }
                }
            }
        });

    }

    void SmilesMenu::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        if (!view_area_ || !emoji_view_)
            return;

        QRect view_port_rect = view_area_->viewport()->geometry();
        QRect emoji_rect = emoji_view_->geometry();

        emoji_view_->on_viewport_changed(QRect(0, view_area_->verticalScrollBar()->value() - emoji_rect.top(), view_port_rect.width(), view_port_rect.height() + 1),
            block_toolbar_switch_);
    }

    void SmilesMenu::set_current_height(int _val)
    {
        setMaximumHeight(_val);
        setMinimumHeight(_val);

        current_height_ = _val;
    }

    int SmilesMenu::get_current_height() const
    {
        return current_height_;
    }

    namespace
    {
        qint32 getEmojiItemSize()
        {
            const auto EMOJI_ITEM_SIZE = 44;
            return Utils::scale_value(EMOJI_ITEM_SIZE);
        }

        qint32 getStickerItemSize()
        {
            const auto STICKER_ITEM_SIZE = 98;
            return Utils::scale_value(STICKER_ITEM_SIZE);
        }

        qint32 getStickerSize()
        {
            const auto STICKER_SIZE = 90;
            return Utils::scale_value(STICKER_SIZE);
        }
    }
}