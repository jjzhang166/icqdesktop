#include "stdafx.h"
#include "ContactAvatarWidget.h"

#include "AvatarPreview.h"
#include "GeneralDialog.h"
#include "ImageCropper.h"
#include "../core_dispatcher.h"
#include "../my_info.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"

namespace
{
    const auto MIN_AVATAR_SIZE = 600;
    const int ADD_PHOTO_FONTSIZE = 24;
    const QColor STROKE_COLOR(0x57, 0x9e, 0x1c);
    const QColor ADD_PHOTO_COLOR(0x28, 0x28, 0x28, 0x7f);
}

namespace Ui
{
    ContactAvatarWidget::ContactAvatarWidget(QWidget* _parent, const QString& _aimid, const QString& _displayName, int _size, bool _autoUpdate)
        :  QPushButton(_parent)
        , aimid_(_aimid)
        , displayName_(_displayName)
        , size_(_size)
        , isInMyProfile_(false)
        , isVisibleShadow_(false)
        , isVisibleSpinner_(false)
        , spinnerMovie_(nullptr)
        , connected_(false)
        , seq_(-1)
    {
        setFixedHeight(_size);
        setFixedWidth(_size);

        spinnerMovie_ = new QMovie(":/resources/gifs/r_spiner200.gif");
        connect(spinnerMovie_, &QMovie::frameChanged, this, &ContactAvatarWidget::frameChanged);

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
        if (isInMyProfile_)
        {
            return isVisibleShadow_ ? "photo enter" : "photo leave";
        }
        return "";
    }

	void ContactAvatarWidget::paintEvent(QPaintEvent* _e)
	{
		bool isDefault = false;
		const auto &avatar = Logic::GetAvatarStorage()->GetRounded(aimid_, displayName_, Utils::scale_bitmap(size_), GetState(), !Logic::getContactListModel()->isChat(aimid_), isDefault, false, false);
        
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

            QPen pen(STROKE_COLOR, Utils::scale_value(2), Qt::DashLine, Qt::RoundCap);
            p.setPen(pen);
            p.drawRoundedRect(
                pen.width(),
                pen.width(),
                size_ - (pen.width() * 2),
                size_ - (pen.width() * 2),
                (size_ / 2),
                (size_ / 2)
            );
            
            p.setFont(Fonts::appFontScaled(ADD_PHOTO_FONTSIZE, Fonts::FontStyle::LIGHT));
            p.setPen(QPen(ADD_PHOTO_COLOR));

            p.drawText(QRectF(0, 0, size_, size_), Qt::AlignCenter, QT_TRANSLATE_NOOP("avatar_upload", "Add\nphoto"));
        }
        else
        {
		    p.drawPixmap(0, 0, size_, size_, *avatar);
        }

        if (isVisibleSpinner_)
        {
            auto spinner_size = spinnerMovie_->currentPixmap().size();
            p.drawPixmap(
                size_/2 - spinner_size.width()/2,
                size_/2 - spinner_size.width()/2,
                spinnerMovie_->currentPixmap()
            );
        }

		return QWidget::paintEvent(_e);
	}

    void ContactAvatarWidget::UpdateParams(const QString& _aimid, const QString& _displayName)
    {
        aimid_ = _aimid;
        displayName_ = _displayName;
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

    void ContactAvatarWidget::enterEvent(QEvent* /*_event*/) 
    {
        emit mouseEntered();
        update();
    }

    void ContactAvatarWidget::leaveEvent(QEvent* /*_event*/)
    {
        emit mouseLeft();
        update();
    }

    void ContactAvatarWidget::SetIsInMyProfile(bool _isInMyProfile)
    {
        isInMyProfile_ = _isInMyProfile;

        if (isInMyProfile_)
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

    void ContactAvatarWidget::SetVisibleShadow(bool _isVisibleShadow)
    {
        isVisibleShadow_ = _isVisibleShadow;
    }

    void ContactAvatarWidget::frameChanged(int /*frame*/)
    {
        repaint();
    }

    void ContactAvatarWidget::SetVisibleSpinner(bool _isVisibleSpinner)
    {
        if (isVisibleSpinner_ == _isVisibleSpinner)
            return;

        isVisibleSpinner_ = _isVisibleSpinner;

        if (isVisibleSpinner_)
        {
            spinnerMovie_->start();
        }
        else
        {
            spinnerMovie_->stop();
        }
    }

    void ContactAvatarWidget::postSetAvatarToCore(const QPixmap& _avatar)
    {
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        _avatar.save(&buffer, "PNG");

        core::coll_helper helper(GetDispatcher()->create_collection(), true);

        core::ifptr<core::istream> data_stream(helper->create_stream());
        if (byteArray.size())
            data_stream->write((const uint8_t*)byteArray.data(), (uint32_t)byteArray.size());
        helper.set_value_as_stream("avatar", data_stream.get());
        if (aimid_ != MyInfo()->aimId())
            helper.set_value_as_string("aimid", aimid_.toStdString());

        seq_ = GetDispatcher()->post_message_to_core("set_avatar", helper.get());
    }

    void ContactAvatarWidget::selectFileForAvatar()
    {
        ResetInfoForSetAvatar();
        QFileDialog fileDialog(this);
        fileDialog.setDirectory(infoForSetAvatar_.currentDirectory);
        fileDialog.setFileMode(QFileDialog::ExistingFiles);
        fileDialog.setNameFilter(QT_TRANSLATE_NOOP("avatar_upload", "Images (*.jpg *.jpeg *.png *.bmp)"));

        bool isContinue = true;
        QImage newAvatar;

        while (isContinue)
        {
            isContinue = false;
            if (fileDialog.exec())
            {
                infoForSetAvatar_.currentDirectory = fileDialog.directory().path();
                auto file = fileDialog.selectedFiles()[0];
                newAvatar = QImage(file);

                if (newAvatar.height() < MIN_AVATAR_SIZE || newAvatar.width() < MIN_AVATAR_SIZE)
                {
                    if (Utils::GetErrorWithTwoButtons(
                        QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Choose file"),
                        NULL,
                        QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Image should be at least 600x600 px"),
                        NULL))
                    {
                        isContinue = true;
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
        auto mainWidget = new QWidget(this);
        auto avatarCropper = new Ui::ImageCropper(mainWidget);
        avatarCropper->setProportion(QSizeF(1.0, 1.0));
        avatarCropper->setProportionFixed(true);
        avatarCropper->setBackgroundColor(QColor(255, 255, 255, 255));
        
        if (!infoForSetAvatar_.croppingRect.isNull())
        {
            avatarCropper->setCroppingRect(infoForSetAvatar_.croppingRect);
        }

        avatarCropper->setImage(QPixmap::fromImage(infoForSetAvatar_.image));

        auto layout = new QHBoxLayout();
        layout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(12), Utils::scale_value(24), 0);
        layout->addWidget(avatarCropper);
        
        mainWidget->setLayout(layout);

        GeneralDialog imageCropDialog(mainWidget, Utils::InterConnector::instance().getMainWindow());
        imageCropDialog.addHead();
        imageCropDialog.addLabel(QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"));

        imageCropDialog.addButtonsPair(QT_TRANSLATE_NOOP("avatar_upload", "Back"), QT_TRANSLATE_NOOP("avatar_upload", "Continue"),
            Utils::scale_value(24), Utils::scale_value(12));
        
        QObject::connect(&imageCropDialog, &GeneralDialog::leftButtonClicked, this, &ContactAvatarWidget::selectFileForAvatar, Qt::QueuedConnection);

        bool isContinue = true;
        while (isContinue)
        {
            isContinue = false;

            if (imageCropDialog.showInPosition(-1, -1))
            {
                infoForSetAvatar_.croppedImage = avatarCropper->cropImage();
                infoForSetAvatar_.croppingRect = avatarCropper->getCroppingRect();

                QByteArray byteArray;
                QBuffer buffer(&byteArray);
                infoForSetAvatar_.croppedImage.save(&buffer, "PNG");

                if (byteArray.size() > 8 * 1024 * 1024)
                {
                    if (Utils::GetErrorWithTwoButtons(
                        QT_TRANSLATE_NOOP("avatar_upload", "Cancel"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Change"),
                        NULL,
                        QT_TRANSLATE_NOOP("avatar_upload", "Upload photo"),
                        QT_TRANSLATE_NOOP("avatar_upload", "Image size should be 8 Mb or less"),
                        NULL))
                    {
                        isContinue = true;
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

        auto spacerLeft = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerLeft);

        auto croppedAvatar = infoForSetAvatar_.croppedImage;
        auto previewAvatarWidget = new AvatarPreview(croppedAvatar, nullptr);
        previewAvatarWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        layout->addWidget(previewAvatarWidget);

        auto spacerRight = new QSpacerItem(Utils::scale_value(12), 1, QSizePolicy::Expanding);
        layout->addSpacerItem(spacerRight);

        auto avatarPreviewHost = new QWidget();
        avatarPreviewHost->setLayout(layout);

        Ui::GeneralDialog previewDialog(avatarPreviewHost, Utils::InterConnector::instance().getMainWindow());
        previewDialog.addHead();
        previewDialog.addLabel(QT_TRANSLATE_NOOP("avatar_upload", "Preview"));

        previewDialog.addButtonsPair(QT_TRANSLATE_NOOP("avatar_upload", "Back"), QT_TRANSLATE_NOOP("avatar_upload", "Save"),
            Utils::scale_value(24), Utils::scale_value(12));

        QObject::connect(&previewDialog, &GeneralDialog::leftButtonClicked, this, &ContactAvatarWidget::cropAvatar, Qt::QueuedConnection);

        if (previewDialog.showInPosition(-1, -1))
        {
            SetVisibleSpinner(true);
            postSetAvatarToCore(croppedAvatar);
        }
    }

    void ContactAvatarWidget::setAvatar(qint64 _seq, int _error)
    {
        if (_seq != seq_)
            return;

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
