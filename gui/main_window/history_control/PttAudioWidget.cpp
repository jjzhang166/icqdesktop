#include "stdafx.h"
#include "PttAudioWidget.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../sounds/SoundsManager.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/TextEditEx.h"
#include "MessagesModel.h"
#include "../../theme_settings.h"
#include "MessageStyle.h"

namespace
{
    const int widget_width = 320;
    const int widget_height = 56;
    const int border_radious = 8;
    const int play_button_size = 40;
    const int play_button_left_margin = 16;
    const int play_button_right_margin = 12;
    const int play_button_top_margin = 8;
    const int text_button_right_margin = 16;
    const int text_button_top_margin = 16;
    const int text_button_size = 24;
    const int text_top_margin = 11;
    const int text_bottom_margin = 12;
}

namespace HistoryControl
{
    PttAudioWidget::PttAudioWidget(QWidget *parent, const QString& contact, const bool isOutgoing, const QString& uri, int duration, qint64 id, qint64 prevId)
        : MessageContentWidget(parent, isOutgoing, contact)
        , Uri_(uri)
        , IsOutgoing_(isOutgoing)
        , Contact_(contact)
        , Duration_(duration * 1000)
        , DownloadId_(-1)
        , MetaDownloadId_(-1)
        , PlayingId_(-1)
        , PlayPressed_(false)
        , PlayHover_(false)
        , Progress_(0)
        , Download_(0)
        , ProgressValue_(0)
        , AngleValue_(90)
        , Id_(id)
        , PrevId_(prevId)
        , Played_(false)
        , HaveText_(false)
        , TextSeq_(-1)
        , PttText_(0)
        , TextPressed_(false)
        , TextHover_(false)
        , TextDownloading_(false)
        , CopyFile_(false)
        , SaveAs_(false)
        , RestartTimer_(new QTimer(this))
    {
        if (isOutgoing)
            AudioState_ = ptt_audio_done;
        else
            AudioState_ = ptt_audio_ready;
        setMouseTracking(true);

        RestartTimer_->setSingleShot(true);
        connect(RestartTimer_, SIGNAL(timeout()), this, SLOT(restartTextrecognition()), Qt::QueuedConnection);
        connect(Logic::GetMessagesModel(), SIGNAL(pttPlayed(qint64)), this, SLOT(pttPlayed(qint64)), Qt::QueuedConnection);
    }

    PttAudioWidget::~PttAudioWidget()
    {
    }

    void PttAudioWidget::initializeInternal()
    {
        setFixedHeight(Utils::scale_value(widget_height));
        QTime time;
        int duration = Duration_ / 1000;

        if (duration < 60)
            time.setHMS(0, 0, duration);
        else
            time.setHMS(0, duration / 60, duration % 60);
        DurationText_ = time.toString("mm:ss");

        connect(Ui::GetDispatcher(), SIGNAL(fileSharingMetadataDownloaded(qint64, QString, QString, QString, QString, QString, qint64, bool)),
            SLOT(setFileInfo(qint64, QString, QString, QString, QString, QString, qint64, bool)), Qt::QueuedConnection);

        connect(Ui::GetDispatcher(), SIGNAL(fileSharingDownloadError(qint64, QString, qint32)), this, SLOT(messageDownloadError(qint64, QString, qint32)), Qt::QueuedConnection);

        MetaDownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(), core::file_sharing_function::download_meta);
    }

    int32_t PttAudioWidget::actualContentWidth()
    {
        return width();
    }

    bool PttAudioWidget::isBlockElement() const
    {
        return false;
    }

    bool PttAudioWidget::canUnload() const
    {
        return true;
    }

    void PttAudioWidget::render(QPainter &p)
    {
        p.save();

        updateHeight();

        int theme_id = Ui::get_qt_theme_settings()->themeIdForContact(aimId_);

        p.fillPath(getBodyPath(QRect(0, 0, width(), height()), Utils::scale_value(border_radious), IsOutgoing_, false), Ui::MessageStyle::getBodyBrush(IsOutgoing_, Selected_, theme_id));
        auto progressPath = getProgressPath(QRect(0, 0, width(), Utils::scale_value(widget_height)), Utils::scale_value(border_radious), IsOutgoing_, HaveText_);
        QBrush progressBrush(IsOutgoing_ ?
            QColor(0x0, 0x0, 0x0, (int32_t)(0.12 * 255)) :
            QColor(0x0, 0x0, 0x0, (int32_t)(0.2 * 255)));

        p.fillPath(progressPath, progressBrush);

        QString playImage;
        switch (AudioState_)
        {
        case ptt_audio_ready:
            playImage = ":/resources/ptt/content_playptt_100";
            break;

        case ptt_audio_downloading:
            playImage = ":/resources/ptt/content_pauseptt_100_download";
            break;

        case ptt_audio_playing:
            playImage = ":/resources/ptt/content_pauseptt_100";
            break;

        case ptt_audio_pause:
            playImage = ":/resources/ptt/content_playptt_done_100";
            break;

        case ptt_audio_done:
            playImage = ":/resources/ptt/content_playptt_done_100";
            break;
        }

        if (AudioState_ != ptt_audio_downloading)
        {
            if (PlayPressed_)
                playImage += "_active";
            else if (PlayHover_)
                playImage += "_hover";
        }

        playImage += ".png";

        double ratio = Utils::scale_bitmap(1);

        QPixmap pixmap = Utils::parse_image_name(playImage);
        Utils::check_pixel_ratio(pixmap);
        p.drawPixmap(Utils::scale_value(play_button_left_margin), Utils::scale_value(play_button_top_margin), pixmap);

        if (AudioState_ == ptt_audio_downloading)
        {
            auto percents = 0.25;
            const auto angle = (int)std::ceil(percents * 360 * 16);
            const auto penWidth = Utils::scale_value(2);
            QPen pen(QBrush(QColor(0x0, 0x0, 0x0, (int32_t)(0.6 * 255))), penWidth);
            p.setPen(pen);

            const auto baseAngle = (AngleValue_ * 16);
            p.drawArc(QRect(Utils::scale_value(play_button_left_margin) + penWidth / 2, Utils::scale_value(play_button_top_margin) + penWidth / 2, pixmap.width() / ratio - penWidth, pixmap.height() / ratio - penWidth), -baseAngle, -angle);
        }

        if (TextDownloading_)
        {
            auto percents = 0.25;
            const auto angle = (int)std::ceil(percents * 360 * 16);
            const auto penWidth = Utils::scale_value(2);
            QPen pen(QBrush(QColor(0x0, 0x0, 0x0, (int32_t)(0.6 * 255))), penWidth);
            p.setPen(pen);

            const auto baseAngle = (AngleValue_ * 16);
            p.drawArc(QRect(width() - Utils::scale_value(text_button_right_margin) - Utils::scale_value(text_button_size) + penWidth / 2, Utils::scale_value(text_button_top_margin) + penWidth / 2,
                      Utils::scale_value(text_button_size) - penWidth, Utils::scale_value(text_button_size) - penWidth), -baseAngle, -angle);
        }
        else
        {
            QString textImage;
            if (AudioState_ == ptt_audio_ready)
                textImage = ":/resources/ptt/content_textptt_100";
            else
                textImage = ":/resources/ptt/content_textptt_done_100";

            if (TextPressed_)
                textImage += "_active";
            else if (TextHover_)
                textImage += "_hover";

            textImage += ".png";

            pixmap = QPixmap(Utils::parse_image_name(textImage));

            Utils::check_pixel_ratio(pixmap);
            p.drawPixmap(width() - Utils::scale_value(text_button_right_margin) - pixmap.width() / ratio, Utils::scale_value(text_button_top_margin), pixmap);
        }

        QFont f = Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15));
        p.setFont(f);
        QPen penText(QColor("#282828"));
        int textWidth = QFontMetrics(f).width(DurationText_);
        Utils::drawText(p, QPointF(Utils::scale_value(play_button_left_margin) + Utils::scale_value(play_button_right_margin) + Utils::scale_value(play_button_size) / 2 + textWidth, Utils::scale_value(widget_height) / 2), Qt::AlignHCenter | Qt::AlignVCenter, DurationText_);

        if (HaveText_)
        {
            QPen pen(QBrush(QColor(0x0, 0x0, 0x0, (int32_t)(0.15 * 255))), Utils::scale_value(1), Qt::SolidLine, Qt::FlatCap);
            p.setPen(pen);
            p.drawLine(QPoint(0, Utils::scale_value(widget_height)), QPoint(width(), Utils::scale_value(widget_height)));
            PttText_->render(&p, QPoint(Utils::scale_value(play_button_left_margin), Utils::scale_value(widget_height) + Utils::scale_value(text_top_margin)));
        }

        p.restore();
    }

    QString PttAudioWidget::toLogString() const
    {
        return Uri_;
    }

    QString PttAudioWidget::toString() const
    {
        if (HaveText_ && !PttText_->selection().isEmpty())
            return PttText_->selection();

        QString result = Uri_;
        if (HaveText_)
        {
            result += "\n";
            result += PttText_->toPlainText();
        }

        return result;
    }

    QString PttAudioWidget::toLink() const
    {
        return Uri_;
    }

    bool PttAudioWidget::hasTextBubble() const
    {
        return HaveText_;
    }

    bool PttAudioWidget::selectByPos(const QPoint &p)
    {
        if (!HaveText_)
            assert(false);

       int y = mapFromGlobal(p).y();
       if (y >=0 && (y < Utils::scale_value(widget_height) || y > height()))
       {
           if (isSelected())
                PttText_->clearSelection();
           return false;
       }

       if (!isSelected())
       {
           QPoint pos = p;
           pos.setY(pos.y() - Utils::scale_value(widget_height) - Utils::scale_value(text_top_margin));
           pos.setX(pos.x() - Utils::scale_value(play_button_left_margin));
           PttText_->selectByPos(pos);
       }
       else
       {
           PttText_->clearSelection();
           return false;
       }
       update();
       return true;
    }

    void PttAudioWidget::clearSelection()
    {
        if (HaveText_)
            PttText_->clearSelection();
        else
            assert(false);
    }

    QString PttAudioWidget::selectedText() const
    {
        if (HaveText_ && !PttText_->selection().isEmpty())
            return PttText_->selection();

        return QString();
    }

    int PttAudioWidget::maxWidgetWidth() const
    {
        return Utils::scale_value(widget_width);
    }

    void PttAudioWidget::copyFile()
    {
        if (!LocalPath_.isEmpty())
        {
            Utils::copyFileToClipboard(LocalPath_);
        }
        else
        {
            CopyFile_ = true;
            connect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)), this, SLOT(messageDownloaded(qint64, QString, QString)), Qt::QueuedConnection);
            DownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(), core::file_sharing_function::download_file);
            AudioState_ = ptt_audio_downloading;
            startDownloadAnimation();
        }
    }

    void PttAudioWidget::saveAs()
    {
        QString dir, file;
        if (Utils::saveAs(FileName_, file, dir))
        {
            SaveAs_ = true;
            connect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)), this, SLOT(messageDownloaded(qint64, QString, QString)), Qt::QueuedConnection);
            DownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, dir, file, core::file_sharing_function::download_file);
            AudioState_ = ptt_audio_downloading;
            startDownloadAnimation();
        }
    }

    bool PttAudioWidget::haveContentMenu(QPoint) const
    {
        return true;
    }

    QPoint PttAudioWidget::deliveryStatusOffsetHint(const int32_t) const
    {
        return QPoint(width(), 0);
    }

    void PttAudioWidget::mouseMoveEvent(QMouseEvent *e)
    {
        bool prevPlayValue = PlayHover_;
        bool prevTextValue = TextHover_;
        PlayHover_ = isMouseOnPlayButton(e->pos());
        TextHover_ = isMouseOnTextButton(e->pos());
        if (prevPlayValue != PlayHover_ || prevTextValue != TextHover_)
            update();

        if (PlayHover_ || TextHover_)
            setCursor(Qt::PointingHandCursor);
        else
            setCursor(QCursor());


        return MessageContentWidget::mouseMoveEvent(e);
    }

    void PttAudioWidget::mousePressEvent(QMouseEvent *e)
    {
        if (e->button() == Qt::LeftButton)
        {
            bool prevPlayValue = PlayPressed_;
            bool prevTextValue = TextPressed_;
            PlayPressed_ = isMouseOnPlayButton(e->pos());
            TextPressed_ = isMouseOnTextButton(e->pos());
            if (prevPlayValue != PlayPressed_ || prevTextValue != TextPressed_)
                update();
        }

        return MessageContentWidget::mousePressEvent(e);
    }

    void PttAudioWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        if (e->button() == Qt::LeftButton)
        {
            if (isMouseOnPlayButton(e->pos()))
            {
                if (AudioState_ == ptt_audio_playing)
                {
                    pause();
                }
                else
                {
                    if (LocalPath_.isEmpty())
                    {
                        connect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)), this, SLOT(messageDownloaded(qint64, QString, QString)), Qt::QueuedConnection);
                        DownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(), core::file_sharing_function::download_file);
                        AudioState_ = ptt_audio_downloading;
                        startDownloadAnimation();
                    }
                    else
                    {
                        play();
                    }
                }
            }
            else if (isMouseOnTextButton(e->pos()))
            {
                showText();
            }
            PlayPressed_ = false;
            TextPressed_ = false;
            update();
        }
        return MessageContentWidget::mouseReleaseEvent(e);
    }

    void PttAudioWidget::resizeEvent(QResizeEvent* e)
    {
        if (HaveText_ && PttText_)
        {
            PttText_->setFixedWidth(width() - Utils::scale_value(play_button_left_margin) - Utils::scale_value(text_button_right_margin));
            updateHeight();
        }

        return MessageContentWidget::resizeEvent(e);
    }

    void PttAudioWidget::setProgress(int value)
    {
        ProgressValue_ = value;
        if (ProgressValue_ == Duration_ && AudioState_ == ptt_audio_playing)
        {
            AudioState_ = ptt_audio_done;
            stopProgressAnimation();
            ProgressValue_ = 0;

            Ui::SoundsManager* manager = Ui::GetSoundsManager();
            disconnect(manager, SIGNAL(pttPaused(int)), this, SLOT(pttPaused(int)));
            disconnect(manager, SIGNAL(pttFinished(int)), this, SLOT(pttFinished(int)));

            emit Logic::GetMessagesModel()->pttPlayed(Id_);
        }
        update();
    }

    int PttAudioWidget::getProgress() const
    {
        return ProgressValue_;
    }

    void PttAudioWidget::setAngle(int value)
    {
        AngleValue_ = value;
        update();
    }

    int PttAudioWidget::getAngle() const
    {
        return AngleValue_;
    }

    void PttAudioWidget::pttPlayed(qint64 id)
    {
        if (id == PrevId_)
        {
            if (!Played_)
            {
                if (AudioState_ == ptt_audio_ready || AudioState_ == ptt_audio_done)
                {
                    Played_ = true;
                    if (LocalPath_.isEmpty())
                    {
                        connect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)), this, SLOT(messageDownloaded(qint64, QString, QString)), Qt::QueuedConnection);
                        DownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(), core::file_sharing_function::download_file);
                        AudioState_ = ptt_audio_downloading;
                        startDownloadAnimation();
                    }
                    else
                    {
                        play();
                    }
                }
                else
                {
                    emit Logic::GetMessagesModel()->pttPlayed(Id_);
                }
            }

            disconnect(Logic::GetMessagesModel(), SIGNAL(pttPlayed(qint64)), this, SLOT(pttPlayed(qint64)));
        }
    }

    void PttAudioWidget::messageDownloaded(qint64 id, QString, QString localPath)
    {
        if (id == DownloadId_)
        {
            disconnect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)), this, SLOT(messageDownloaded(qint64, QString, QString)));
            disconnect(Ui::GetDispatcher(), SIGNAL(fileSharingDownloadError(qint64, QString, qint32)), this, SLOT(messageDownloadError(qint64, QString, qint32)));
            DownloadId_ = -1;

            LocalPath_ = localPath;
            if (Played_ || IsOutgoing_)
                AudioState_ = ptt_audio_done;
            else
                AudioState_ = ptt_audio_ready;
            update();

            if (CopyFile_)
            {
                CopyFile_ = false;
                copyFile();
                update();
            }
            else if (SaveAs_)
            {
                SaveAs_ = false;
            }
            else
            {
                play();
            }
        }
    }

    void PttAudioWidget::messageDownloadError(qint64 id, QString, qint32)
    {
        if (id == DownloadId_)
            DownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(), core::file_sharing_function::download_file);
        else if (id == MetaDownloadId_)
            MetaDownloadId_ = Ui::GetDispatcher()->downloadSharedFile(Contact_, Uri_, Ui::get_gui_settings()->get_value<QString>(settings_download_directory, Utils::DefaultDownloadsPath()), QString(), core::file_sharing_function::download_meta);
    }

    void PttAudioWidget::setFileInfo(qint64 id, QString, QString, QString, QString, QString filename, qint64 size, bool played)
    {
        if (id != MetaDownloadId_)
            return;

        if (played && AudioState_ == ptt_audio_ready)
        {
            AudioState_ = ptt_audio_done;
            update();
        }

        FileName_ = filename;

        Played_ = played;

        FileSize_ = size;
    }

    void PttAudioWidget::speechToText(qint64 seq, int error, QString text, int comeback)
    {
        if (seq == TextSeq_)
        {
            if (error && comeback)
            {
                RestartTimer_->setInterval(comeback * 1000);
                RestartTimer_->start();
                return;
            }
            TextDownloading_ = false;
            stopDownloadAnimation();
            if (!PttText_)
            {
                PttText_ = new Ui::TextEditEx(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor("#282828"), false, true);
                PttText_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
                PttText_->setFixedWidth(width() - Utils::scale_value(play_button_left_margin) - Utils::scale_value(text_button_right_margin));
                PttText_->setFrameStyle(QFrame::NoFrame);
                PttText_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                PttText_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                PttText_->setWordWrapMode(QTextOption::WordWrap);
                PttText_->setStyleSheet("background: transparent");
                PttText_->setFocusPolicy(Qt::NoFocus);
                PttText_->hide();
            }
            disconnect(Ui::GetDispatcher(), SIGNAL(speechToText(qint64, int, QString, int)), this, SLOT(speechToText(qint64, int, QString, int)));
            if (error != 0 || text.isEmpty())
                text = QT_TRANSLATE_NOOP("ptt_widget", "unclear message");

            PttText_->setText(text);
            HaveText_ = true;

            if (AudioState_ == ptt_audio_ready)
                AudioState_ = ptt_audio_done;

            if (error == 0)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("url", Uri_);
                collection.set_value_as_bool("played", true);
                Ui::GetDispatcher()->post_message_to_core("files/set_url_played", collection.get());
            }

            update();
        }
    }

    void PttAudioWidget::pttFinished(int id)
    {
        if (id != PlayingId_)
            return;

        if (AudioState_ != ptt_audio_playing)
        {
            AudioState_ = ptt_audio_done;
            stopProgressAnimation();
            ProgressValue_ = 0;
            update();

            Ui::SoundsManager* manager = Ui::GetSoundsManager();
            disconnect(manager, SIGNAL(pttPaused(int)), this, SLOT(pttPaused(int)));
            disconnect(manager, SIGNAL(pttFinished(int)), this, SLOT(pttFinished(int)));
        }
    }

    void PttAudioWidget::pttPaused(int id)
    {
        if (id != PlayingId_)
            return;

        pauseProgressAnimation();

        AudioState_ = ptt_audio_pause;
        update();
    }

    void PttAudioWidget::restartTextrecognition()
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("url", Uri_);
        collection.set_value_as_qstring("locale", Utils::GetTranslator()->getCurrentLang());
        TextSeq_ = Ui::GetDispatcher()->post_message_to_core("files/speech_to_text", collection.get());
    }

    void PttAudioWidget::play()
    {
        Played_ = true;
        Ui::SoundsManager* manager = Ui::GetSoundsManager();
        PlayingId_ = manager->playPtt(LocalPath_, PlayingId_);
        if (PlayingId_ != -1)
        {
            AudioState_ = ptt_audio_playing;
            update();
            connect(manager, SIGNAL(pttPaused(int)), this, SLOT(pttPaused(int)), Qt::QueuedConnection);
            connect(manager, SIGNAL(pttFinished(int)), this, SLOT(pttFinished(int)), Qt::QueuedConnection);
            startProgressAnimation();
        }

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("url", Uri_);
        collection.set_value_as_bool("played", true);
        Ui::GetDispatcher()->post_message_to_core("files/set_url_played", collection.get());
    }

    void PttAudioWidget::pause()
    {
        Ui::GetSoundsManager()->pausePtt(PlayingId_);
        AudioState_ = ptt_audio_pause;
        update();

        pauseProgressAnimation();
    }

    bool PttAudioWidget::isMouseOnPlayButton(const QPoint& pos)
    {
        return pos.x() >= Utils::scale_value(play_button_left_margin) && pos.x() <= Utils::scale_value(play_button_size) + Utils::scale_value(play_button_left_margin)
            && pos.y() >= Utils::scale_value(play_button_top_margin) && pos.y() <= Utils::scale_value(play_button_size) + Utils::scale_value(play_button_top_margin);
    }

    bool PttAudioWidget::isMouseOnTextButton(const QPoint& pos)
    {
        return pos.x() >= width() - Utils::scale_value(text_button_right_margin) - Utils::scale_value(text_button_size)
               && pos.x() <= width() - Utils::scale_value(text_button_right_margin)
               && pos.y() >= Utils::scale_value(text_button_top_margin) && pos.y() <= Utils::scale_value(text_button_size) + Utils::scale_value(text_button_top_margin);
    }

    void PttAudioWidget::startProgressAnimation()
    {
        if (!Progress_)
        {
            Progress_ = new QPropertyAnimation(this, "Progress");
            Progress_->setDuration(Duration_);
            Progress_->setStartValue(0);
            Progress_->setEndValue(Duration_);
        }

        if (Progress_->state() == QAbstractAnimation::Paused)
            Progress_->resume();
        else
            Progress_->start();
    }

    void PttAudioWidget::stopProgressAnimation()
    {
        Progress_->stop();
    }

    void PttAudioWidget::pauseProgressAnimation()
    {
        if (Progress_->state() == QAbstractAnimation::Running)
            Progress_->pause();
    }

    void PttAudioWidget::startDownloadAnimation()
    {
        if (!Download_)
        {
            Download_ = new QPropertyAnimation(this, "Angle");
            Download_->setDuration(700);
            Download_->setStartValue(90);
            Download_->setEndValue(450);
            Download_->setLoopCount(-1);
        }

        stopDownloadAnimation();

        Download_->start();
    }

    void PttAudioWidget::stopDownloadAnimation()
    {
        Download_->stop();
    }

    void PttAudioWidget::showText()
    {
        if (!PttText_)
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("url", Uri_);
            collection.set_value_as_qstring("locale", Utils::GetTranslator()->getCurrentLang());
            TextSeq_ = Ui::GetDispatcher()->post_message_to_core("files/speech_to_text", collection.get());
            connect(Ui::GetDispatcher(), SIGNAL(speechToText(qint64, int, QString, int)), this, SLOT(speechToText(qint64, int, QString, int)), Qt::QueuedConnection);
            TextDownloading_ = true;
            startDownloadAnimation();
            update();
        }
        else
        {
            HaveText_ = !HaveText_;
            update();
        }
    }

    void PttAudioWidget::updateHeight()
    {
        if (HaveText_)
        {
            PttText_->setFixedHeight(PttText_->getTextSize().height());
            setFixedHeight(Utils::scale_value(widget_height) + PttText_->height() + Utils::scale_value(text_top_margin) + Utils::scale_value(text_bottom_margin));
        }
        else
        {
            setFixedHeight(Utils::scale_value(widget_height));
        }
    }

    QPainterPath PttAudioWidget::getBodyPath(
        const QRect &rect,
        const int32_t borderRadius,
        const bool isOutgoing, const bool skipArc)
    {
        const auto borderDiameter = (borderRadius * 2);

        const auto heightMinusBorder = (rect.height() - borderDiameter);
        const auto widthMinusBorder = (rect.width() - borderDiameter);

        QPainterPath clipPath;

        auto x = 0;
        auto y = 0;

        if (isOutgoing)
        {
            x += borderRadius;
        }

        clipPath.moveTo(x, y);

        if (isOutgoing)
        {
            clipPath.arcTo(0, 0, borderDiameter, borderDiameter, 90, 90);
        }

        x = 0;
        y = skipArc ? rect.height() : heightMinusBorder;
        clipPath.lineTo(x, y);

        if (!skipArc)
            clipPath.arcTo(x, y, borderDiameter, borderDiameter, 180, 90);

        x = ((isOutgoing || skipArc) ? rect.width() : widthMinusBorder);
        y = rect.height();
        clipPath.lineTo(x, y);

        if (!isOutgoing && !skipArc)
        {
            y -= borderDiameter;
            clipPath.arcTo(x, y, borderDiameter, borderDiameter, 270, 90);
        }

        x = rect.width();
        y = borderDiameter;
        clipPath.lineTo(x, y);

        x -= borderDiameter;
        y -= borderDiameter;
        clipPath.arcTo(x, y, borderDiameter, borderDiameter, 0, 90);

        x = (isOutgoing ? borderRadius : 0);
        y = 0;

        clipPath.lineTo(x, y);
        clipPath = clipPath.translated(rect.left(), rect.top());
        return clipPath;
    }

    QPainterPath PttAudioWidget::getProgressPath(const QRect &rect, const int32_t borderRadius, const bool isOutgoing, const bool haveText)
    {
        QPainterPath clipPath;

        if (ProgressValue_ == 0)
            return clipPath;

        clipPath = getBodyPath(rect, borderRadius, isOutgoing, haveText);

        double widthPercent = (double)rect.width() * ((double)ProgressValue_ / (double)Duration_);

        QRect r(0, 0, widthPercent, rect.height());
        QPainterPath p;
        p.addRect(r);

        clipPath = clipPath.intersected(p);

        return clipPath;
    }
}