#include "stdafx.h"
#include "InputWidget.h"

#include "../../main_window/MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../main_window/history_control/MessagesModel.h"
#include "../../utils/utils.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../utils/gui_coll_helper.h"
#include "../../gui_settings.h"

const int widget_min_height = 73;
const int widget_max_height = 230;
const int document_min_height = 32;

namespace Ui
{
    input_edit::input_edit(QWidget* _parent)
        :   TextEditEx(_parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor(0x28, 0x28, 0x28), true, false)
    {
        setUndoRedoEnabled(true);
    }

    bool input_edit::catch_enter(int _modifiers)
    {
        int key1_to_send = get_gui_settings()->get_value<int>(settings_key1_to_send_message, 0);
        if (key1_to_send != 0)
        {
            bool b1 = ((_modifiers & Qt::ControlModifier) == Qt::ControlModifier) && (key1_to_send == Qt::Key_Control);
            bool b2 = ((_modifiers & Qt::ShiftModifier) == Qt::ShiftModifier) && (key1_to_send == Qt::Key_Shift);
            bool b3 = (_modifiers == Qt::Key_Enter) && ((_modifiers & Qt::Key_Enter) == Qt::Key_Enter) && (key1_to_send == Qt::Key_Enter);
            if (!b1 && !b2 && !b3)
                return false;
        }
        else if (_modifiers != Qt::NoModifier && _modifiers != Qt::KeypadModifier)
        {
            return false;
        }

        return true;
    }

    InputWidget::InputWidget(QWidget* parent)
        : QWidget(parent)
        , text_edit_(new input_edit(this))
        , smiles_button_(new QPushButton(this))
        , send_button_(new QPushButton(this))
        , file_button_(new QPushButton(this))
        , active_height_(Utils::scale_value(widget_min_height))
        , need_height_(active_height_)
        , anim_height_(0)
    {

        setVisible(false);
        setStyleSheet(Utils::LoadStyle(":/main_window/input_widget/input_widget.qss", Utils::get_scale_coefficient(), true));

        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        QHBoxLayout* horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);

        smiles_button_->setObjectName("smiles_button");
        smiles_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        smiles_button_->setCursor(QCursor(Qt::PointingHandCursor));
        smiles_button_->setFocusPolicy(Qt::NoFocus);
        horizontalLayout->addWidget(smiles_button_);

        text_edit_->setObjectName("input_edit_control");
        text_edit_->setPlaceholderText(QT_TRANSLATE_NOOP("input_widget","Message..."));
        text_edit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        text_edit_->setAutoFillBackground(false);
        text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        text_edit_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        horizontalLayout->addWidget(text_edit_);

        connect(text_edit_, &TextEditEx::focusOut, [this]()
        {
            emit editFocusOut();
        });

        send_button_->setObjectName("send_button");
        send_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        send_button_->setCursor(QCursor(Qt::PointingHandCursor));
        Testing::setAccessibleName(send_button_, "SendMessageButton");
        horizontalLayout->addWidget(send_button_);

        setLayout(horizontalLayout);

        file_button_->setObjectName("file_button");
        file_button_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        file_button_->setFocusPolicy(Qt::NoFocus);
        file_button_->setCursor(QCursor(Qt::PointingHandCursor));
        file_button_->raise();
        Testing::setAccessibleName(file_button_, "SendFileButton");

        set_current_height(active_height_);

        connect(text_edit_->document(), SIGNAL(contentsChanged()), this, SLOT(edit_content_changed()), Qt::QueuedConnection);
        connect(text_edit_, SIGNAL(enter()), this, SLOT(send()), Qt::QueuedConnection);
        connect(text_edit_, SIGNAL(enter()), this, SLOT(stats_message_enter()), Qt::QueuedConnection);
        connect(send_button_, SIGNAL(clicked()), this, SLOT(send()), Qt::QueuedConnection);
        connect(send_button_, SIGNAL(clicked()), this, SLOT(stats_message_send()), Qt::QueuedConnection);
        connect(file_button_, SIGNAL(clicked()), this, SLOT(attach_file()), Qt::QueuedConnection);
        connect(file_button_, SIGNAL(clicked()), this, SLOT(stats_attach_file()), Qt::QueuedConnection);

        connect(smiles_button_, &QPushButton::clicked, [this]()
        {
            text_edit_->setFocus();
            emit smilesMenuSignal();
        });

        connect(text_edit_, SIGNAL(keyPressed(int)), this, SLOT(typed(int)));
        
        active_document_height_ = text_edit_->document()->size().height();

        Testing::setAccessibleName(text_edit_, "InputTextEdit");
    }

    InputWidget::~InputWidget()
    {

    }


    void InputWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void InputWidget::resizeEvent(QResizeEvent * _e)
    {
        edit_content_changed();

        QRect edit_rect = text_edit_->geometry();
        file_button_->setGeometry(edit_rect.right() - Utils::scale_value(24) - Utils::scale_value(16) - Utils::scale_value(16)/*margin*/,
            edit_rect.top() + ((edit_rect.height() - Utils::scale_value(24))/2),
            Utils::scale_value(24),
            Utils::scale_value(24));

        return QWidget::resizeEvent(_e);
    }
    
    void InputWidget::keyReleaseEvent(QKeyEvent *_e)
    {
        if (platform::is_apple())
        {
            if (_e->modifiers() & Qt::AltModifier)
            {
                QTextCursor cursor = text_edit_->textCursor();
                QTextCursor::MoveMode selection = (_e->modifiers() & Qt::ShiftModifier)?
                    QTextCursor::MoveMode::KeepAnchor:
                    QTextCursor::MoveMode::MoveAnchor;
                
                if (_e->key() == Qt::Key_Left)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::PreviousWord, selection);
                    text_edit_->setTextCursor(cursor);
                }
                else if (_e->key() == Qt::Key_Right)
                {
                    cursor.movePosition(QTextCursor::MoveOperation::NextWord, selection);
                    text_edit_->setTextCursor(cursor);
                }
            }
        }
        
        QWidget::keyReleaseEvent(_e);
    }

    void InputWidget::resize_to(int _height)
    {
        if (get_current_height() == _height)
            return;

        QEasingCurve easing_curve = QEasingCurve::Linear;
        int duration = 100;

        int start_value = get_current_height();
        int end_value = _height;

        if (!anim_height_)
            anim_height_ = new QPropertyAnimation(this, "current_height");

        anim_height_->stop();
        anim_height_->setDuration(duration);
        anim_height_->setStartValue(start_value);
        anim_height_->setEndValue(end_value);
        anim_height_->setEasingCurve(easing_curve);
        anim_height_->start();
    }

    void InputWidget::edit_content_changed()
    {
        QString input_text = text_edit_->getPlainText();

        Logic::GetContactListModel()->setInputText(contact_, input_text);

        send_button_->setEnabled(!input_text.trimmed().isEmpty());
        file_button_->setVisible(input_text.isEmpty());

        int doc_height = text_edit_->document()->size().height();

        if (doc_height < Utils::scale_value(document_min_height) && active_document_height_ < Utils::scale_value(document_min_height))
            return;

        int diff = doc_height - active_document_height_;

        active_document_height_ = doc_height;

        need_height_ = need_height_ + diff;

        int new_height = need_height_;

        if (need_height_ <= Utils::scale_value(widget_min_height) || doc_height <= Utils::scale_value(document_min_height))
        {
            new_height = Utils::scale_value(widget_min_height);
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
        else if (need_height_ > Utils::scale_value(widget_max_height))
        {
            new_height = Utils::scale_value(widget_max_height);
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
        else
        {
            text_edit_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }

        resize_to(new_height);
    }

    void InputWidget::quote(QString _text)
    {
        text_edit_->insertPlainText_(_text);
        text_edit_->setFocus();
    }

    void InputWidget::insert_emoji(int32_t _main, int32_t _ext)
    {
        text_edit_->insert_emoji(_main, _ext);

        text_edit_->setFocus();
    }

    void InputWidget::send_sticker(int32_t _set_id, int32_t _sticker_id)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        //ext:1:sticker:5
        collection.set_value_as_bool("sticker", true);
        collection.set_value_as_int("sticker/set_id", _set_id);
        collection.set_value_as_int("sticker/id", _sticker_id);

        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

        emit sendMessage(contact_);
    }

    void InputWidget::send()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", contact_);
        QString text = text_edit_->getPlainText().trimmed();
        
        if (text.length() == 0)
        {
            return;
        }
        
        if ((unsigned)text.length() > Utils::get_input_maximum_chars())
            text.resize(Utils::get_input_maximum_chars());

        while (text.endsWith("\n"))
            text = text.left(text.length() - 1);
        if (text.isEmpty())
            return;

        collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());

        text_edit_->clear();

        emit sendMessage(contact_);
    }
    
    void InputWidget::attach_file()
    {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::ExistingFiles);
        dialog.setViewMode(QFileDialog::Detail);
        dialog.setDirectory(get_gui_settings()->get_value<QString>(settings_upload_directory, ""));
        if (platform::is_apple())
            dialog.setFilter(QDir::Files); // probably it'll also be useful for other platforms
        if (!dialog.exec())
            return;

        get_gui_settings()->set_value<QString>(settings_upload_directory, dialog.directory().absolutePath());

        QStringList selectedFiles = dialog.selectedFiles();
        if (selectedFiles.isEmpty())
            return;
        
        core::stats::event_props_type props;
        props.push_back(std::make_pair("Filesharing_Count", std::to_string(selectedFiles.size())));
        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent_count, props);

        for (const auto &filename : selectedFiles)
        {
            QFileInfo fileInfo(filename);
            if (fileInfo.size() == 0)
                continue;

            Ui::GetDispatcher()->uploadSharedFile(contact_, filename);
         
            core::stats::event_props_type props_file;
            props_file.push_back(std::make_pair("Filesharing_Filesize", std::to_string(1.0 * fileInfo.size() / 1024 / 1024)));
            props_file.push_back(std::make_pair("Filesharing_Is_Image", std::to_string(Utils::is_image_extension(fileInfo.suffix()))));
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_sent, props_file);
        }
    }

    void InputWidget::contactSelected(QString contact)
    {
        contact_ = contact;

        setVisible(true);
        activateWindow();

        text_edit_->setPlainText(Logic::GetContactListModel()->getInputText(contact_));
        text_edit_->setFocus(Qt::FocusReason::MouseFocusReason);
    }
    
    void InputWidget::hide()
    {
//        contact_ = nullptr;
        setVisible(false);
        text_edit_->setPlainText("");
    }

    void InputWidget::set_current_height(int _val)
    {
        setMaximumHeight(_val);
        setMinimumHeight(_val);

        active_height_ = _val;
    }

    int InputWidget::get_current_height() const
    {
        return active_height_;
    }
    
    void InputWidget::typed(int /*key*/)
    {
        static uint prevTypedTime = 0;
        uint currTypedTime = QDateTime::currentDateTime().toTime_t();
        if ((currTypedTime - prevTypedTime) > 2)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", contact_);
            Ui::GetDispatcher()->post_message_to_core("message/typing", collection.get());
            prevTypedTime = currTypedTime;
        }
    }

    void InputWidget::stats_message_enter()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_enter_button);
    }


    void InputWidget::stats_message_send()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::message_send_button);
    }

    void InputWidget::stats_attach_file()
    {
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dialog);
    }
}
