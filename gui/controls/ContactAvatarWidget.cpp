#include "stdafx.h"
#include "ContactAvatarWidget.h"
#include "../utils/utils.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../utils/InterConnector.h"
#include "../core_dispatcher.h"
#include "ImageCropper.h"
#include "AvatarPreview.h"
#include "GeneralDialog.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../my_info.h"

namespace Ui
{
    ContactAvatarWidget::ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _display_name, int _size, bool _autoUpdate)
        :  QPushButton(_parent)
        , aimid_(_aimid)
        , display_name_(_display_name)
        , size_(_size)
        , is_in_my_profile_(false)
        , is_visible_shadow_(false)
        , is_visible_spinner_(false)
        , spinner_movie_(nullptr)
        , connected_(false)
    {
        setFixedHeight(_size);
        setFixedWidth(_size);

        spinner_movie_ = new QMovie(":/resources/gifs/r_spiner200.gif");
        connect(spinner_movie_, &QMovie::frameChanged, this, &ContactAvatarWidget::frameChanged);

        if (_autoUpdate)
            connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)), Qt::QueuedConnection);
        
        infoForSetAvatar_.currentDirectory = QDir::homePath();
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::setAvatar, this, &ContactAvatarWidget::setAvatar, Qt::QueuedConnection);
	}

    ContactAvatarWidget::~ContactAvatarWidget()
    {
    }
    
    QString ContactAvatarWidget::GetState()
    {
        if (is_in_my_profile_)
        {
            return is_visible_shadow_ ? "photo enter" : "photo leave";
        }
        return "";
    }

	void ContactAvatarWidget::paintEvent(QPaintEvent* _e)
	{
		bool isDefault = false;
		const auto &avatar = Logic::GetAvatarStorage()->GetRounded(aimid_, display_name_, Utils::scale_bitmap(size_), GetState(), !Logic::GetContactListModel()->isChat(aimid_), isDefault, false);
        
		if (avatar->isNull())
			return;

		QPainter p(this);
        if (aimid_ == MyInfo()->aimId() && isDefault)
        {
            p.setPen(Qt::NoPen);
            p.setRenderHint(QPainter::Antialiasing);
            p.setRenderHint(QPainter::TextAntialiasing);
            p.setRenderHint(QPainter::SmoothPixmapTransform);
            p.setBrush(QBrush(QColor(Qt::transparent)));
            p.drawEllipse(0, 0, size_, size_);

            QPen pen(QColor(0x57, 0x9e, 0x1c), Utils::scale_value(2), Qt::DashLine, Qt::RoundCap);
            p.setPen(pen);
            p.drawRoundedRect(pen.width(), pen.width(), size_ - (pen.width() * 2), size_ - (pen.width() * 2), (size_ / 2), (size_ / 2));
            

            p.setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI_LIGHT, Utils::scale_value(24)));
            p.setPen(QPen(QColor(0x28, 0x28, 0x28, 0x7f)));

            p.drawText(QRectF(0, 0, size_, size_), Qt::AlignCenter, QT_TRANSLATE_NOOP("avatar_upload", "Add\nphoto"));
        }
        else
        {
		    p.drawPixmap(0, 0, size_, size_, *avatar);
        }

        if (is_visible_spinner_)
        {
            auto spinner_size = spinner_movie_->currentPixmap().size();
            p.drawPixmap(size_/2 - spinner_size.width()/2, size_/2 - spinner_size.width()/2, spinner_movie_->currentPixmap());
        }

		return QWidget::paintEvent(_e);
	}

    void ContactAvatarWidget::UpdateParams(const QString& _aimid, const QString& _display_name)
    {
        aimid_ = _aimid;
        display_name_ = _display_name;
    }

    void ContactAvatarWidget::avatarChanged(QString _aimId)
    {
        if (_aimId == aimid_)
            update();
    }

    void ContactAvatarWidget::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->source() == Qt::MouseEventNotSynthesized)
        {
            emit clicked();
            _event->accept();
        }
    }

    void ContactAvatarWidget::enterEvent(QEvent* _event) 
    {
        emit mouseEntered();
        update();
    }

    void ContactAvatarWidget::leaveEvent(QEvent* _event)
    {
        emit mouseLeft();
        update();
    }

    void ContactAvatarWidget::SetIsInMyProfile(bool _is_in_my_profile)
    {
        is_in_my_profile_ = _is_in_my_profile;

        if (is_in_my_profile_)
        {
            if (!connected_)
            {
                connected_ = true;
                connect(this, &ContactAvatarWidget::clicked, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);
                connect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter, Qt::QueuedConnection);
                connect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave, Qt::QueuedConnection);
                this->setCursor(Qt::CursorShape::PointingHandCursor);
            }
        }
        else
        {
            connected_ = false;
            disconnect(this, &ContactAvatarWidget::clicked, this, &ContactAvatarWidget::selectFileForAvatar);
            disconnect(this, &ContactAvatarWidget::mouseEntered, this, &ContactAvatarWidget::avatarEnter);
            disconnect(this, &ContactAvatarWidget::mouseLeft, this, &ContactAvatarWidget::avatarLeave);
            this->setCursor(Qt::CursorShape::ArrowCursor);
        }

    }

    void ContactAvatarWidget::SetVisibleShadow(bool _is_visible_shadow)
    {
        is_visible_shadow_ = _is_visible_shadow;
    }

    void ContactAvatarWidget::frameChanged(int frame)
    {
        repaint();
    }

    void ContactAvatarWidget::SetVisibleSpinner(bool _is_visible_spinner)
    {
        is_visible_spinner_ = _is_visible_spinner;

        if (is_visible_spinner_)
        {
            spinner_movie_->start();
        }
        else
        {
            spinner_movie_->stop();
        }
    }

    void ContactAvatarWidget::postSetAvatarToCore(const QPixmap& _avatar)
    {
        QByteArray byte_array;
        QBuffer buffer(&byte_array);
        _avatar.save(&buffer, "PNG");

        core::coll_helper helper(GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(helper->create_stream());
        if (byte_array.size())
            data_stream->write((const uint8_t*) byte_array.data(), (uint32_t)byte_array.size());
        helper.set_value_as_stream("avatar", data_stream.get());
        if (aimid_ != MyInfo()->aimId())
            helper.set_value_as_string("aimid", aimid_.toStdString());

        GetDispatcher()->post_message_to_core("set_avatar", helper.get());
    }

    const auto min_avatar_size_px = 600;

    void ContactAvatarWidget::selectFileForAvatar()
    {
        ResetInfoForSetAvatar();
        QFileDialog fileDialog(this);
        fileDialog.setDirectory(infoForSetAvatar_.currentDirectory);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setNameFilter(QT_TRANSLATE_NOOP("avatar_upload", "Images (*.jpg *.jpeg *.png *.bmp)"));

        bool is_continue = true;
        QImage newAvatar;

        while (is_continue)
        {
            is_continue = false;
            if (fileDialog.exec())
            {
                infoForSetAvatar_.currentDirectory = fileDialog.directory().path();
                auto file = fileDialog.selectedFiles()[0];
                newAvatar = QImage(file);

                if (newAvatar.height() < min_avatar_size_px || newAvatar.width() < min_avatar_size_px)
                {
                    if (Utils::GetErrorWithTwoButtons(
                        QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Choose file"),
                        NULL,
                        QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Image should be at least 600x600 px"),
                        NULL))
                    {
                        is_continue = true;
                    }
                    else
                    {
                        return;
                    }
                }
            }
            else
            {
                return;
            }
        }
                
        infoForSetAvatar_.image = newAvatar;
        cropAvatar();
    }

    void ContactAvatarWidget::ResetInfoForSetAvatar()
    {
        infoForSetAvatar_.image = QImage();
        infoForSetAvatar_.croppingRect = QRectF();
        infoForSetAvatar_.croppedImage = QPixmap();
    }

    void ContactAvatarWidget::cropAvatar()
    {
        auto main_widget = new QWidget(this);
        auto avatar_cropper = new Ui::ImageCropper(main_widget);
        avatar_cropper->setProportion(QSizeF(1.0, 1.0));
        avatar_cropper->setProportionFixed(true);
        avatar_cropper->setBackgroundColor(QColor(255, 255, 255, 255));
        
        if (!infoForSetAvatar_.croppingRect.isNull())
        {
            avatar_cropper->setCroppingRect(infoForSetAvatar_.croppingRect);
        }

        avatar_cropper->setImage(QPixmap::fromImage(infoForSetAvatar_.image));

        auto layout = new QHBoxLayout();
        layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(12), Utils::scale_value(24), 0);
        layout->addWidget(avatar_cropper);
        
        main_widget->setLayout(layout);

        GeneralDialog image_crop_dialog(main_widget, Utils::InterConnector::instance().getMainWindow());
        image_crop_dialog.addHead();
        image_crop_dialog.addLabel(QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"));

        image_crop_dialog.addButtonsPair(QT_TRANSLATE_NOOP("avatar_upload", "Back"), QT_TRANSLATE_NOOP("avatar_upload", "Continue"),
            Utils::scale_value(24), Utils::scale_value(12));
        
        QObject::connect(&image_crop_dialog, &GeneralDialog::leftButtonClicked, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);

        bool is_continue = true;
        while (is_continue)
        {
            is_continue = false;

            if (image_crop_dialog.showInPosition(-1, -1))
            {
                infoForSetAvatar_.croppedImage = avatar_cropper->cropImage();
                infoForSetAvatar_.croppingRect = avatar_cropper->getCroppingRect();

                QByteArray byte_array;
                QBuffer buffer(&byte_array);
                infoForSetAvatar_.croppedImage.save(&buffer, "PNG");

                if (byte_array.size() > 8 * 1024 * 1024)
                {
                    if (Utils::GetErrorWithTwoButtons(
                        QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Change"),
                        NULL,
                        QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Image size should be 8 Mb or less"),
                        NULL))
                    {
                        is_continue = true;
                    }
                    else
                    {
                        return;
                    }
                }

            }
            else
            {
                return;
            }
        }

        openAvatarPreview();
    }

    void ContactAvatarWidget::openAvatarPreview()
    {
        auto layout = new QHBoxLayout();

        auto spacer_left = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacer_left);

        auto cropped_avatar = infoForSetAvatar_.croppedImage;
        auto preview_avatar_widget = new AvatarPreview(cropped_avatar, nullptr);
        preview_avatar_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        layout->addWidget(preview_avatar_widget);

        auto spacerRight = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerRight);

        auto avatarPreviewHost = new QWidget();
        avatarPreviewHost->setLayout(layout);

        Ui::GeneralDialog preview_dialog(avatarPreviewHost, Utils::InterConnector::instance().getMainWindow());
        preview_dialog.addHead();
        preview_dialog.addLabel(QT_TRANSLATE_NOOP("avatar_upload", "Preview"));

        preview_dialog.addButtonsPair(QT_TRANSLATE_NOOP("avatar_upload", "Back"), QT_TRANSLATE_NOOP("avatar_upload", "Save"),
            Utils::scale_value(24), Utils::scale_value(12));

        QObject::connect(&preview_dialog, &GeneralDialog::leftButtonClicked, this, &ContactAvatarWidget::cropAvatar, Qt::QueuedConnection);

        if (preview_dialog.showInPosition(-1, -1))
        {
            SetVisibleSpinner(true);
            postSetAvatarToCore(cropped_avatar);
        }
    }

    void ContactAvatarWidget::setAvatar(int _error)
    {
        SetVisibleSpinner(false);

        if (_error != 0)
        {
             if (Utils::GetErrorWithTwoButtons(
                QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                QT_TRANSLATE_NOOP("avatar_upload", "Choose file"),
                NULL,
                QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                QT_TRANSLATE_NOOP("avatar_upload", "Avatar was not uploaded due to server error"),
                NULL))
             {
                selectFileForAvatar();
             }
        }
        else
        {
            emit afterAvatarChanged();
        }
    }

    void ContactAvatarWidget::avatarEnter()
    {
        this->SetVisibleShadow(true);
    }

    void ContactAvatarWidget::avatarLeave()
    {
        this->SetVisibleShadow(false);
    }
}
