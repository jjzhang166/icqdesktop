#include "stdafx.h"
#include "SnapsPage.h"
#include "../cache/snaps/SnapStorage.h"
#include "../cache/emoji/Emoji.h"
#include "../cache/emoji/EmojiDb.h"
#include "mplayer/FFMpegPlayer.h"
#include "../utils/utils.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/Text2DocConverter.h"
#include "../utils/InterConnector.h"
#include "../core_dispatcher.h"
#include "../main_window/input_widget/InputWidget.h"
#include "../main_window/smiles_menu/SmilesMenu.h"
#include "../main_window/contact_list/ContactList.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/contact_list/UnknownsModel.h"
#include "../main_window/contact_list/SelectionContactsForGroupChat.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "../main_window/history_control/MessageStyle.h"
#include "../fonts.h"
#include "../controls/CustomButton.h"
#include "../controls/TextEditEx.h"
#include "../controls/ContactAvatarWidget.h"
#include "../my_info.h"
#include "../main_window/sidebar/Sidebar.h"

namespace
{
    const int IMAGE_DURATION = 4000;
    const int PROGRESS_HEIGHT = 70;
    const int PROGRESS_TEXT_PADDING = 16;
    const int PROGRESS_VIEWS_PADDING = 54;
    const int PROGRESS_VIEWS_TEXT_PADDING = 66;
    const int PROGRESS_TOP_PADDING = 8;
    const int PROGRESS_PEN_WIDTH = 2;
    const int PROGRESS_SPACING = 6;
    const int MY_CONTROL_PANEL_WIDTH = 188;
    const int CONTROL_PANEL_HEIGHT = 36;
    const int MESSAGE_HEIGHT = 56;
    const int BUTTON_OFFSET = 2;
    const int CONTROL_PANEL_ELEMENT_WIDTH = 36;
    const int CONTROL_PANEL_ELEMENT_SPACING = 8;
    const int CONTROL_PANEL_SPACING = 16;
    const int CONTROL_PANEL_SMILES_SPACING = 6;
    const int MESSAGE_SIZE = 56;
    const int BORDER_RADIUS = 8;
    const int SMILES_SIZE = 120;
    const int BUTTON_SIZE = 32;
    const int SMILE_SIZE = 32;
    const int SPACE_TO_SMILES_OPEN = 16;
    const int CLOSE_BUTTON_HEIGHT = 24;
    const int CLOSE_BUTTON_WIDTH = 28;
    const int OPEN_SMILES_WIDTH = 12;
    const int OPEN_SMILES_HEIGTH = 8;
    const int EMOJI_VIEW_HEIGHT = 186;
    const int EMOJI_ITEM_SIZE = 44;
    const int PREVIEW_OFFSET = 40;
    const int SMILES_TOP_PADDING = 8;
    const int SEND_TIMEOUT = 2000;

    const auto FIRST_EMOJI_MAIN = 0x1f602;
    const auto SECOND_EMOJI_MAIN = 0x1f525;
    const auto THIRD_EMOJI_MAIN = 0x1f618;
    const auto FIRST_EMOJI_EXT = 0x0;
    const auto SECOND_EMOJI_EXT = 0x0;
    const auto THIRD_EMOJI_EXT = 0x0;

    const int ANIMATION_START = 0;
    const int ANIMATION_END = 1000;
    const int ANIMATION_DURATION = 1000;

    const int SMILE_ANIMATION_START = 0;
    const int SMILE_ANIMATION_END = 100;
    const int SMILE_ANIMATION_DURATION = 100;

    const int MAX_ANIMATIONS_COUNT = 5;
    const int MAX_PLAYER_QUEUE_SIZE = 3;
    const int PROGRESS_SIZE = 40;

    const int AVATAR_SIZE = 24;
    const int AVATAR_SPACING = 8;
    const int NAME_TOP_PADDING = 6;

    const int SNAP_TOP_PADDING = 16;
    const int FRIENDLY_HEIGHT = 24;

    Emoji::EmojiSizePx getEmojiSize()
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_32;
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
        case 100:
            emojiSize = Emoji::EmojiSizePx::_32;
            break;
        case 125:
            emojiSize = Emoji::EmojiSizePx::_40;
            break;
        case 150:
            emojiSize = Emoji::EmojiSizePx::_48;
            break;
        case 200:
            emojiSize = Emoji::EmojiSizePx::_64;
            break;
        default:
            assert(!"invalid scale");
        }

        return emojiSize;
    }

    QString formatDate(QDateTime date)
    {
        const auto current = QDateTime::currentDateTime();
        const auto days = date.daysTo(current);
        QString result;
        if (days == 0)
            result += QT_TRANSLATE_NOOP("contact_list", "today");
        else if (days == 1)
            result += QT_TRANSLATE_NOOP("contact_list", "yesterday");
        else
            result += Utils::GetTranslator()->formatDate(date.date(), date.date().year() == current.date().year());
        if (date.date().year() == current.date().year())
        {
            result += QT_TRANSLATE_NOOP("contact_list", " at ");
            result += date.time().toString(Qt::SystemLocaleShortDate);
        }

        return result;
    }
}

namespace Ui
{
PreviewWidget::PreviewWidget(QWidget* parent)
    : QWidget(parent)
    , Avatar_(new ContactAvatarWidget(this, QString(), QString(), Utils::scale_value(AVATAR_SIZE), true))
{
    Avatar_->setFixedSize(QSize(Utils::scale_value(AVATAR_SIZE), Utils::scale_value(AVATAR_SIZE)));
    Avatar_->move(Utils::scale_value(AVATAR_SPACING), Utils::scale_value(AVATAR_SPACING));

    Friendly_ = new TextEditEx(this, Fonts::appFontScaled(14, Fonts::FontWeight::Medium), QColor("#ffffff"), false, false);
    Friendly_->setFrameStyle(QFrame::NoFrame);
    Friendly_->setStyleSheet("background: transparent;");
    Friendly_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    Friendly_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    Friendly_->move(Utils::scale_value(AVATAR_SPACING) + Avatar_->width(), Utils::scale_value(NAME_TOP_PADDING));
    Friendly_->setTextInteractionFlags(Qt::NoTextInteraction);
    Friendly_->setFixedHeight(Utils::scale_value(FRIENDLY_HEIGHT));
}
    
PreviewWidget::~PreviewWidget()
{
}
    
void PreviewWidget::setPreview(const QPixmap& _preview)
{
    QGraphicsBlurEffect* blur = new QGraphicsBlurEffect(this);
    blur->setBlurRadius(10.0);
    blur->setBlurHints(QGraphicsBlurEffect::QualityHint);

    QGraphicsScene scene;
    QGraphicsPixmapItem item;

    item.setPixmap(_preview);
    item.setGraphicsEffect(blur);
    scene.addItem(&item);
    QPixmap res(_preview.size());
    res.fill(Qt::transparent);
    QPainter ptr(&res);
    ptr.setRenderHints(QPainter::HighQualityAntialiasing);
    scene.render(&ptr, QRect(), QRect(0, 0, _preview.width(), _preview.height()));
    ptr.setOpacity(0.1);
    ptr.fillRect(res.rect(), Qt::white);

    Preview_ = res;
    update();
}

void PreviewWidget::setAimId(const QString& _aimid)
{
    AimId_ = _aimid;
    FriendlyName_ = Logic::GetSnapStorage()->getFriednly(_aimid);
    updateFriendly();
    Avatar_->UpdateParams(AimId_, FriendlyName_);
    update();
}

QString PreviewWidget::getAimId() const
{
    return AimId_;
}
    
void PreviewWidget::paintEvent(QPaintEvent* e)
{
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing);
    p.fillRect(rect(), Preview_.isNull() ? QColor("#767676") : Qt::black);
    if (!Preview_.isNull())
    {
        auto s = Preview_.size();
        s = s.scaled(size(), Qt::KeepAspectRatio);
        auto r = QRect((width() - s.width()) / 2, (height() - s.height()) / 2, s.width(), s.height());
        p.drawPixmap(r, Preview_, Preview_.rect());
    }
    Friendly_->render(&p);
}

void PreviewWidget::resizeEvent(QResizeEvent* e)
{
    Avatar_->move(Utils::scale_value(AVATAR_SPACING), Utils::scale_value(AVATAR_SPACING));
    Friendly_->move(Utils::scale_value(AVATAR_SPACING * 2) + Avatar_->width(), Utils::scale_value(NAME_TOP_PADDING));
    updateFriendly();
    return QWidget::resizeEvent(e);
}

void PreviewWidget::updateFriendly()
{
    QFontMetrics m(Friendly_->font());
    auto w = width() - Utils::scale_value(AVATAR_SPACING * 2 + AVATAR_SIZE);
    auto friendly = m.elidedText(FriendlyName_, Qt::ElideRight, w);
    Friendly_->setFixedWidth(w);
    auto &doc = *Friendly_->document();
    doc.clear();
    QTextCursor cursor = Friendly_->textCursor();
    Logic::Text2Doc(friendly, cursor, Logic::Text2DocHtmlMode::Pass, false, nullptr, Emoji::EmojiSizePx::_16);
    Logic::FormatDocument(doc, Utils::scale_value(FRIENDLY_HEIGHT));
}
    
ProgressBar::ProgressBar(QWidget* parent)
    : QWidget(parent)
    , Pos_(0)
    , Duration_(0)
    , Current_(0)
    , Count_(0)
    , Inverted_(false)
    , Avatar_(new ContactAvatarWidget(this, QString(), QString(), Utils::scale_value(AVATAR_SIZE), true))
    , Timestamp_(new QLabel(this))
    , Views_(new QLabel(this))
{
    Avatar_->setFixedSize(QSize(Utils::scale_value(AVATAR_SIZE), Utils::scale_value(AVATAR_SIZE)));
    Avatar_->move(Utils::scale_value(AVATAR_SPACING), Utils::scale_value(AVATAR_SPACING + SNAP_TOP_PADDING));

    auto f = Fonts::appFontScaled(14, Fonts::FontWeight::Medium);
    Friendly_ = new TextEditEx(this, f, QColor("#ffffff"), false, false);
    Friendly_->setFrameStyle(QFrame::NoFrame);
    Friendly_->setStyleSheet("background: transparent;");
    Friendly_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    Friendly_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    Friendly_->setFixedWidth(width());
    Friendly_->setFixedHeight(Utils::scale_value(FRIENDLY_HEIGHT));
    Friendly_->hide();

    Friendly_->setCursor(Qt::PointingHandCursor);

    QPalette p;
    p.setColor(Timestamp_->foregroundRole(), QColor("#ffffff"));

    Timestamp_->setStyleSheet("background: transparent;");
    Timestamp_->setPalette(p);
    Timestamp_->setFont(f);

    Views_->setStyleSheet("background: transparent;");
    Views_->setPalette(p);
    Views_->setFont(f);

    setMouseTracking(true);
}

ProgressBar::~ProgressBar()
{

}

void ProgressBar::durationChanged(qint64 _duration)
{
    if (_duration < 0)
        return;

    Duration_ = _duration;
    update();
}

void ProgressBar::positionChanged(qint64 _position)
{
    Pos_ = _position;
    update();
}

void ProgressBar::paintEvent(QPaintEvent* e)
{
    if (Count_ == 0 || Duration_ == 0)
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    QPen pen;
    pen.setWidth(Utils::scale_value(PROGRESS_PEN_WIDTH));
    pen.setColor(QColor("#ffffff"));
    p.setPen(pen);
    int from = 0;
    int cur_width = (width() - ((Count_ - 1) * Utils::scale_value(PROGRESS_SPACING))) / Count_;
    for (int i = 0; i < Count_; ++i)
    {
        auto to = (i + 1) * cur_width + i * Utils::scale_value(PROGRESS_SPACING);
        if (i != Current_)
        {
            p.drawLine(QPoint(from, Utils::scale_value(PROGRESS_TOP_PADDING)), QPoint(to, Utils::scale_value(PROGRESS_TOP_PADDING)));
            from = to + Utils::scale_value(PROGRESS_SPACING);
        }
        else
        {
            auto cur_from = from;
            auto b = QPoint(from, Utils::scale_value(PROGRESS_TOP_PADDING));
            auto e = QPoint(std::max(from + ((to - from) * Pos_ / Duration_), cur_from), Utils::scale_value(PROGRESS_TOP_PADDING));
            if (e != b)
                p.drawLine(b, e);
            
            from = std::max(from + ((to - from) * Pos_ / Duration_), cur_from);
            QColor color("#999999");
            color.setAlphaF(0.43);
            pen.setColor(color);
            p.setPen(pen);
            p.drawLine(QPoint(from, Utils::scale_value(PROGRESS_TOP_PADDING)), QPoint(to, Utils::scale_value(PROGRESS_TOP_PADDING)));
            from = to + Utils::scale_value(PROGRESS_SPACING);
        }
    }

    p.translate(0, 0);
    Friendly_->render(&p, QPoint(Utils::scale_value(AVATAR_SPACING * 2) + Avatar_->width(), Utils::scale_value(NAME_TOP_PADDING + SNAP_TOP_PADDING)));
}

void ProgressBar::resizeEvent(QResizeEvent * e)
{
    updateFriednly();
    Timestamp_->move(width() - Utils::scale_value(AVATAR_SPACING) -Timestamp_->width(), Utils::scale_value(AVATAR_SPACING + SNAP_TOP_PADDING));
    Views_->move(width() - Utils::scale_value(AVATAR_SPACING) - Views_->width(), Utils::scale_value(AVATAR_SPACING + SNAP_TOP_PADDING) + Timestamp_->height());
    return QWidget::resizeEvent(e);
}

void ProgressBar::mouseReleaseEvent(QMouseEvent *e)
{
    QFontMetrics m(Friendly_->font());
    auto text = Friendly_->getPlainText();
    auto w = m.width(text);
    auto h = m.height();
    QRect r (width() / 2 - w / 2, Utils::scale_value(PROGRESS_TEXT_PADDING), w, h);
    if (r.contains(e->pos()))
    {
        if (Logic::getContactListModel()->getContactItem(AimId_))
        {
            Logic::getContactListModel()->setCurrent(AimId_, -1, true, true, [this](HistoryControlPage *page)
            {
                Utils::InterConnector::instance().showSidebar(AimId_, Ui::profile_page);
            });
            return;
        }

        Logic::getContactListModel()->add(AimId_, Friendly_->getPlainText());
        Logic::getUnknownsModel()->add(AimId_);
        Logic::getContactListModel()->setCurrent(AimId_, -1, true, true, [this](HistoryControlPage *page)
        {
            Utils::InterConnector::instance().showSidebar(AimId_, Ui::profile_page);
        });
    }
}

void ProgressBar::mouseMoveEvent(QMouseEvent * e)
{
    QFontMetrics m(Friendly_->font());
    auto text = Friendly_->getPlainText();
    auto w = m.width(text);
    auto h = m.height();
    QRect r (width() / 2 - w / 2, Utils::scale_value(PROGRESS_TEXT_PADDING), w, h);
    bool theSame = (QApplication::overrideCursor() && QApplication::overrideCursor()->shape() == Qt::PointingHandCursor);
    if (r.contains(e->pos()))
    {
        if (!theSame)
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
    }
    else
    {
       QApplication::restoreOverrideCursor();
    }

    return QWidget::mouseMoveEvent(e);
}

void ProgressBar::leaveEvent(QEvent * e)
{
    QApplication::restoreOverrideCursor();
    return QWidget::leaveEvent(e);
}

void ProgressBar::updateFriednly()
{
    QFontMetrics m(Friendly_->font());
    auto w = width() - Utils::scale_value(AVATAR_SPACING * 2 + AVATAR_SIZE) - Timestamp_->width();
    auto friendly = m.elidedText(FriendlyName_, Qt::ElideRight, w);
    Friendly_->setFixedWidth(w);
    auto &doc = *Friendly_->document();
    doc.clear();
    QTextCursor cursor = Friendly_->textCursor();
    Logic::Text2Doc(friendly, cursor, Logic::Text2DocHtmlMode::Pass, false, nullptr, Emoji::EmojiSizePx::_16);
    Logic::FormatDocument(doc, Utils::scale_value(FRIENDLY_HEIGHT));
}

void ProgressBar::setCount(int count)
{
    Count_ = count;
    update();
}

void ProgressBar::setCurrent(int cur)
{
    Current_ = cur;
    update();
}

void ProgressBar::resetCurrent()
{
    Current_ = 0;
    update();
}

void ProgressBar::next()
{
    ++Current_;
    update();
}

void ProgressBar::prev()
{
    --Current_;
    update();
}

void ProgressBar::setFriednly(const QString& _friednly)
{
    FriendlyName_ = _friednly;
    updateFriednly();
    Avatar_->UpdateParams(AimId_, _friednly);
    update();
}

void ProgressBar::setAimId(const QString& _aimId)
{
    AimId_ = _aimId;
    Avatar_->UpdateParams(_aimId, Friendly_->getPlainText());
    update();
}

void ProgressBar::setViews(int _views)
{
    Views_->setText(QVariant(_views).toString() + " " + QT_TRANSLATE_NOOP("snaps", "views"));
    Views_->adjustSize();
    Views_->move(width() - Utils::scale_value(AVATAR_SPACING) - Views_->width(), Utils::scale_value(AVATAR_SPACING + SNAP_TOP_PADDING) + Timestamp_->height());
    update();
}

void ProgressBar::setTimestamp(int32_t _timestamp)
{
    Timestamp_->setText(formatDate(QDateTime::fromTime_t(_timestamp)));
    Timestamp_->adjustSize();
    Timestamp_->move(width() - Utils::scale_value(AVATAR_SPACING) -Timestamp_->width(), Utils::scale_value(AVATAR_SPACING + SNAP_TOP_PADDING));
    updateFriednly();
    update();
}

ControlPanel::ControlPanel(QWidget* parent)
    : QWidget(parent)
    , MessageHovered_(false)
    , ForwardHovered_(false)
    , SmilesOpenHovered_(false)
    , LeftPadding_(0)
    , Prop_(0)
    , curSmile_(0)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    Text_ = new TextEditEx(this, QFont(), QColor(), false, false);
    Text_->hide();

    Timer_ = new QTimer(this);
    Timer_->setInterval(SEND_TIMEOUT);
    Timer_->setSingleShot(true);
    connect(Timer_, SIGNAL(timeout()), this, SLOT(send()), Qt::QueuedConnection);

    Animation_ = new QPropertyAnimation(this, "prop");
    Animation_->setStartValue(SMILE_ANIMATION_START);
    Animation_->setEndValue(SMILE_ANIMATION_END);
    Animation_->setDuration(SMILE_ANIMATION_DURATION);
}

ControlPanel::~ControlPanel()
{
}

void ControlPanel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::black);
    QPen pen;
    pen.setColor(Qt::transparent);
    p.setPen(pen);
    p.setOpacity(0.6);

    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    p.drawRoundedRect(QRect(0, y, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT)), Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));
    p.drawRoundedRect(QRect(width() / 2 - Utils::scale_value(SMILES_SIZE / 2), y, Utils::scale_value(SMILES_SIZE), Utils::scale_value(CONTROL_PANEL_HEIGHT)), 
        Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));
    p.drawRoundedRect(QRect(width() - Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), y, 
        Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT)), Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));

    p.setOpacity(1.0);
    QPixmap message(Utils::parse_image_name(MessageHovered_ ? ":/resources/videoplayer/reply_snap_button_100_hover.png" : ":/resources/videoplayer/reply_snap_button_100.png"));
    Utils::check_pixel_ratio(message);
    p.drawPixmap(Utils::scale_value(BUTTON_OFFSET), y + Utils::scale_value(BUTTON_OFFSET), message);


    QPixmap forward(Utils::parse_image_name(ForwardHovered_ ? ":/resources/videoplayer/player_share_big_100_hover.png" : ":/resources/videoplayer/player_share_big_100.png"));
    Utils::check_pixel_ratio(forward);
    p.drawPixmap(width() - Utils::scale_value(BUTTON_SIZE + BUTTON_OFFSET), y + Utils::scale_value(BUTTON_OFFSET), forward);

    QImage firstSmile = Emoji::GetEmoji(FIRST_EMOJI_MAIN, FIRST_EMOJI_EXT, getEmojiSize());
    QImage secondSmile = Emoji::GetEmoji(SECOND_EMOJI_MAIN, SECOND_EMOJI_EXT, getEmojiSize());
    QImage thirdSmile = Emoji::GetEmoji(THIRD_EMOJI_MAIN, THIRD_EMOJI_EXT, getEmojiSize());
    
    Utils::check_pixel_ratio(firstSmile);
    Utils::check_pixel_ratio(secondSmile);
    Utils::check_pixel_ratio(thirdSmile);

    int addSpaceFirst = 0;
    int addSpaceSecond = 0;
    int addSpaceThird = 0;
    if (Animation_->state() != QAbstractAnimation::Stopped)
    {
        auto size = SMILE_SIZE * ((float)Prop_ / SMILE_ANIMATION_END);
        switch (curSmile_)
        {
        case 1:
            firstSmile = firstSmile.scaledToHeight(size);
            addSpaceFirst = (SMILE_SIZE - size) / 2;
            break;
        case 2:
            secondSmile = secondSmile.scaledToHeight(size);
            addSpaceSecond = (SMILE_SIZE - size) / 2;
            break;
        case 3:
            thirdSmile = thirdSmile.scaledToHeight(size);
            addSpaceThird = (SMILE_SIZE - size) / 2;
            break;
        }
    }

    p.drawImage(QRect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) + addSpaceFirst, y + addSpaceFirst + Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE)), firstSmile, firstSmile.rect());
    p.drawImage(QRect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) * 2 + Utils::scale_value(SMILE_SIZE) + addSpaceSecond, y + addSpaceSecond + Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE)), secondSmile, secondSmile.rect());
    p.drawImage(QRect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) * 3 + Utils::scale_value(SMILE_SIZE) * 2 + addSpaceThird, y + addSpaceThird + Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE)), thirdSmile, thirdSmile.rect());

    QPixmap smilesOpen(Utils::parse_image_name(SmilesOpenHovered_ ? ":/resources/basic_elements/arrow_small_b_100.png" : ":/resources/basic_elements/arrow_small_c_100.png"));
    Utils::check_pixel_ratio(smilesOpen);
    p.drawPixmap(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) * 4 + Utils::scale_value(SMILE_SIZE) * 3, y + Utils::scale_value(SPACE_TO_SMILES_OPEN), smilesOpen);
}

void ControlPanel::mouseMoveEvent(QMouseEvent *e)
{
    MessageHovered_ = isMessage(e->pos());
    ForwardHovered_ = isForward(e->pos());
    SmilesOpenHovered_ = isSmilesOpen(e->pos());

    update();

    return QWidget::mouseMoveEvent(e);
}

void ControlPanel::leaveEvent(QEvent *e)
{
    MessageHovered_ = false;
    ForwardHovered_ = false;
    SmilesOpenHovered_ = false;

    update();

    return QWidget::leaveEvent(e);
}

void ControlPanel::mouseReleaseEvent(QMouseEvent *e)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    if (isMessage(e->pos()))
    {
        emit messageClicked();
    }
    else if (isSmilesOpen(e->pos()))
    {
        emit openSmilesClicked();
    }
    else if (isFirstSmile(e->pos()))
    {
        emit animation(Emoji::GetEmoji(FIRST_EMOJI_MAIN, FIRST_EMOJI_EXT, getEmojiSize()), 
                       LeftPadding_ + Utils::scale_value(MESSAGE_SIZE + CONTROL_PANEL_SPACING), LeftPadding_ + width() / 2,
                       y, 0);
        Animation_->start();
        curSmile_ = 1;
        addEmoji(FIRST_EMOJI_MAIN, FIRST_EMOJI_EXT);
    }
    else if (isSecondSmile(e->pos()))
    {
        emit animation(Emoji::GetEmoji(SECOND_EMOJI_MAIN, SECOND_EMOJI_EXT, getEmojiSize()), 
            LeftPadding_ + Utils::scale_value(MESSAGE_SIZE + CONTROL_PANEL_SPACING + BUTTON_SIZE), LeftPadding_ + width() / 2,
            y, 0);
        Animation_->start();
        curSmile_ = 2;
        addEmoji(SECOND_EMOJI_MAIN, SECOND_EMOJI_EXT);
    }
    else if (isThirdSmile(e->pos()))
    {
        emit animation(Emoji::GetEmoji(THIRD_EMOJI_MAIN, THIRD_EMOJI_EXT, getEmojiSize()), 
            LeftPadding_ + Utils::scale_value(MESSAGE_SIZE + CONTROL_PANEL_SPACING + BUTTON_SIZE * 2), LeftPadding_ + width() / 2,
            y, 0);
        Animation_->start();
        curSmile_ = 3;
        addEmoji(THIRD_EMOJI_MAIN, THIRD_EMOJI_EXT);
    }
    else if (isForward(e->pos()))
    {
        emit forwardClicked();
    }
    else
    {
        return QWidget::mouseReleaseEvent(e);
    }
}

bool ControlPanel::isMessage(const QPoint& p)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    QRect rect(0, y, Utils::scale_value(BUTTON_SIZE), Utils::scale_value(BUTTON_SIZE));

    return rect.contains(p);
}

bool ControlPanel::isForward(const QPoint& p)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    QRect rect(width() - Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), y, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT));

    return rect.contains(p);
}

bool ControlPanel::isSmilesOpen(const QPoint& p)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    QRect rect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) * 4 + Utils::scale_value(SMILE_SIZE) * 3, y + Utils::scale_value(SPACE_TO_SMILES_OPEN), Utils::scale_value(OPEN_SMILES_WIDTH), Utils::scale_value(OPEN_SMILES_HEIGTH));

    return rect.contains(p);
}

bool ControlPanel::isFirstSmile(const QPoint& p)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    QRect rect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET), y + Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE));

    return rect.contains(p);
}

bool ControlPanel::isSecondSmile(const QPoint& p)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    QRect rect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) * 2 + Utils::scale_value(SMILE_SIZE), y + Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE));

    return rect.contains(p);
}

bool ControlPanel::isThirdSmile(const QPoint& p)
{
    int y = height() - Utils::scale_value(CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING);

    QRect rect(width() / 2 - Utils::scale_value(SMILES_SIZE) / 2 + Utils::scale_value(BUTTON_OFFSET) * 3 + Utils::scale_value(SMILE_SIZE) * 2, y + Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE));

    return rect.contains(p);
}

void ControlPanel::addEmoji(int32_t main, int32_t ext)
{
    Text_->insertEmoji(main, ext);
    Timer_->start();
}

void ControlPanel::predefined(const QString& aimId, const QString& url)
{
    Timer_->stop();
    send();
    AimId_ = aimId;
    Url_ = url;
}

void ControlPanel::send()
{
    QString text = Text_->getPlainText();
    Text_->clear();
    if (!text.isEmpty())
    {
        text = Url_ + " " + text;
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set<QString>("contact", AimId_);
        collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
    }
}

MyControlPanel::MyControlPanel(QWidget* parent)
    : QWidget(parent)
    , SaveHovered_(false)
    , DeleteHovered_(false)
    , ForwardHovered_(false)
{
    setMouseTracking(true);
}

MyControlPanel::~MyControlPanel()
{

}

void MyControlPanel::paintEvent(QPaintEvent* e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::black);
    QPen pen;
    pen.setColor(Qt::transparent);
    p.setPen(pen);
    p.setOpacity(0.6);
    p.drawRoundedRect(QRect(0, 0, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT)), Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));
    p.drawRoundedRect(QRect(Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH + CONTROL_PANEL_ELEMENT_SPACING), 0, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT)), Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));
    p.drawRoundedRect(QRect(width() - Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), 0, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT)), Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));

    p.setOpacity(1.0);

    QPixmap save(Utils::parse_image_name(SaveHovered_ ? ":/resources/videoplayer/player_save_big_100_hover.png" : ":/resources/videoplayer/player_save_big_100.png"));
    Utils::check_pixel_ratio(save);
    p.drawPixmap(Utils::scale_value(BUTTON_OFFSET), Utils::scale_value(BUTTON_OFFSET), save);

    QPixmap deleteSnap(Utils::parse_image_name(DeleteHovered_ ? ":/resources/videoplayer/player_delete_big_100_hover.png" : ":/resources/videoplayer/player_delete_big_100.png"));
    Utils::check_pixel_ratio(deleteSnap);
    p.drawPixmap(Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH + CONTROL_PANEL_ELEMENT_SPACING + BUTTON_OFFSET), Utils::scale_value(BUTTON_OFFSET), deleteSnap);

    QPixmap forward(Utils::parse_image_name(ForwardHovered_ ? ":/resources/videoplayer/player_share_big_100_hover.png" : ":/resources/videoplayer/player_share_big_100.png"));
    Utils::check_pixel_ratio(forward);
    p.drawPixmap(width() - Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH - BUTTON_OFFSET), Utils::scale_value(BUTTON_OFFSET), forward);
}

bool MyControlPanel::isSave(const QPoint& p)
{
    QRect r(0, 0, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT));
    return r.contains(p);
}

bool MyControlPanel::isDelete(const QPoint& p)
{
    QRect r(Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH + CONTROL_PANEL_ELEMENT_SPACING), 0, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT));
    return r.contains(p);
}

bool MyControlPanel::isForward(const QPoint& p)
{
    QRect r(width() - Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), 0, Utils::scale_value(CONTROL_PANEL_ELEMENT_WIDTH), Utils::scale_value(CONTROL_PANEL_HEIGHT));
    return r.contains(p);
}

void MyControlPanel::mouseMoveEvent(QMouseEvent* e)
{
    SaveHovered_ = isSave(e->pos());
    DeleteHovered_ = isDelete(e->pos());
    ForwardHovered_ = isForward(e->pos());

    update();

    return QWidget::mouseMoveEvent(e);
}

void MyControlPanel::mouseReleaseEvent(QMouseEvent* e)
{
    if (isSave(e->pos()))
    {
        emit save();
    }
    else if (isDelete(e->pos()))
    {
        emit deleteSnap();
    }
    else if(isForward(e->pos()))
    {
        emit forwardClicked();
    }
}

MessagePanel::MessagePanel(QWidget* parent)
    : QWidget(parent)
{
    auto mainLayout = Utils::emptyVLayout(this);
    auto hLayout = Utils::emptyHLayout();
    hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

    Close_ = new QPushButton(this);
    Close_->setObjectName("close_button");
    Close_->setFixedSize(Utils::scale_value(CLOSE_BUTTON_WIDTH), Utils::scale_value(CLOSE_BUTTON_HEIGHT));
    hLayout->addWidget(Close_);
    mainLayout->addLayout(hLayout);

    QString styleSheet = ":/main_window/input_widget/input_widget_snaps.qss";
    setStyleSheet(Utils::LoadStyle(styleSheet));

    Input_ = new Ui::InputWidget(this, Utils::scale_value(MESSAGE_HEIGHT), styleSheet, true);
    Input_->setFontColor(Qt::white);
    Input_->show();
    mainLayout->addWidget(Input_);

    connect(Input_, SIGNAL(sizeChanged()), this, SLOT(sizeChanged()), Qt::QueuedConnection);
    connect(Close_, SIGNAL(clicked()), this, SIGNAL(closeClicked()), Qt::QueuedConnection);
    connect(Input_, SIGNAL(sendMessage(QString)), this, SIGNAL(closeClicked()), Qt::QueuedConnection);
}

MessagePanel::~MessagePanel()
{

}

void MessagePanel::predefined(const QString& aimId, const QString& text)
{
    Input_->predefined(aimId, text);
}

void MessagePanel::setFocus()
{
    Input_->setFocusOnInput();
}

void MessagePanel::paintEvent(QPaintEvent * e)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Qt::black);
    QPen pen;
    pen.setColor(Qt::transparent);
    p.setPen(pen);
    p.setOpacity(0.6);

    p.drawRoundedRect(rect(), Utils::scale_value(BORDER_RADIUS), Utils::scale_value(BORDER_RADIUS));
}

void MessagePanel::mouseReleaseEvent(QMouseEvent *e)
{

}

void MessagePanel::sizeChanged()
{
    setFixedHeight(Input_->get_current_height() + Utils::scale_value(CLOSE_BUTTON_HEIGHT));
    emit needMove();
}

SmilesPanel::SmilesPanel(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    auto mainLayout = Utils::emptyVLayout(this);
    mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));
    Model_ = new Smiles::EmojiViewItemModel(this);

    QStringList emojiCategories = Emoji::GetEmojiCategories();
    for (auto i : emojiCategories)
        Model_->addCategory(i);
    
    Model_->resize(size(), true);
    Delegate_ = new Smiles::EmojiTableItemDelegate(this);
    ViewWidget_ = new QWidget(this);
    ViewWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ViewWidget_->setStyleSheet("background-color:rgba(0,0,0,60%);");
    auto viewLayout = new QVBoxLayout(ViewWidget_);
    viewLayout->setContentsMargins(Utils::scale_value(CONTROL_PANEL_SPACING), Utils::scale_value(SMILES_TOP_PADDING), Utils::scale_value(CONTROL_PANEL_SPACING), 0);

    auto hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, Utils::scale_value(CONTROL_PANEL_SPACING));
    hLayout->setAlignment(Qt::AlignLeft);
    EmojiLabel_ = new QLabel(ViewWidget_);
    QPalette p;
    p.setColor(EmojiLabel_->foregroundRole(), QColor("#ffffff"));
    EmojiLabel_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    EmojiLabel_->setPalette(p);
    EmojiLabel_->setText(QT_TRANSLATE_NOOP("contact_list","EMOJI"));
    EmojiLabel_->setFixedWidth(width() / 2 - Utils::scale_value(OPEN_SMILES_WIDTH) / 2);
    EmojiLabel_->setStyleSheet("background: transparent;");
    EmojiLabel_->setFont(Fonts::appFont(Utils::scale_value(12), Fonts::defaultAppFontFamily(), Fonts::FontWeight::Normal));
    hLayout->addWidget(EmojiLabel_);

    auto pix = QPixmap(Utils::parse_image_name(":/resources/basic_elements/arrow_small_c_100.png"));
    pix = pix.transformed(QTransform().rotate(180), Qt::SmoothTransformation);
    auto pix_hover = QPixmap(Utils::parse_image_name(":/resources/basic_elements/arrow_small_b_100.png"));
    pix_hover = pix_hover.transformed(QTransform().rotate(180), Qt::SmoothTransformation);
    
    auto back = new CustomButton(ViewWidget_, pix);
    back->setActive(false);
    back->setHoverImage(pix_hover);
    back->setFixedSize(QSize(Utils::scale_value(OPEN_SMILES_WIDTH), Utils::scale_value(OPEN_SMILES_HEIGTH)));
    back->setStyleSheet("background: transparent; border-style: none;");
    back->setCursor(Qt::PointingHandCursor);
    hLayout->addWidget(back);
    viewLayout->addLayout(hLayout);

    connect(back, SIGNAL(clicked()), this, SIGNAL(closeClicked()), Qt::QueuedConnection);

    View_ = new QTableView(this);
    View_->setFixedSize(width(), Model_->getNeedHeight());
    View_->setModel(Model_);
    View_->setItemDelegate(Delegate_);
    View_->setShowGrid(false);
    View_->verticalHeader()->hide();
    View_->horizontalHeader()->hide();
    View_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    View_->verticalHeader()->setDefaultSectionSize(Utils::scale_value(EMOJI_ITEM_SIZE));
    View_->horizontalHeader()->setDefaultSectionSize(Utils::scale_value(EMOJI_ITEM_SIZE));
    View_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    View_->setFocusPolicy(Qt::NoFocus);
    View_->setSelectionMode(QAbstractItemView::NoSelection);
    View_->setCursor(QCursor(Qt::PointingHandCursor));
    View_->setFrameStyle(QFrame::NoFrame);
    View_->setStyleSheet("background-color:transparent;");
    viewLayout->addWidget(View_);
    Scroll_ = new QScrollArea(this);
    Scroll_->setFrameStyle(QFrame::NoFrame);
    Scroll_->setWidget(View_);
    Scroll_->setStyleSheet("background-color:transparent;");
    ViewWidget_->setFixedHeight(Utils::scale_value(EMOJI_VIEW_HEIGHT));
    viewLayout->addWidget(Scroll_);
    mainLayout->addWidget(ViewWidget_);

    Text_ = new TextEditEx(this, QFont(), QColor(), false, false);
    Text_->hide();

    Timer_ = new QTimer(this);
    Timer_->setInterval(SEND_TIMEOUT);
    Timer_->setSingleShot(true);
    connect(Timer_, SIGNAL(timeout()), this, SLOT(send()), Qt::QueuedConnection);

    connect(View_, &QTableView::clicked, [this](const QModelIndex & _index)
    {
        auto emoji = Model_->getEmoji(_index.column(), _index.row());
        if (emoji)
        {
            auto em = Emoji::GetEmoji(emoji->Codepoint_, emoji->ExtendedCodepoint_, getEmojiSize());
            auto pos = mapFromGlobal(QCursor::pos());
            emit animation(em, pos.x(), width() / 2, pos.y(), 0);
            Delegate_->animate(_index, SMILE_ANIMATION_START, SMILE_ANIMATION_END, SMILE_ANIMATION_DURATION);
            Text_->insertEmoji(emoji->Codepoint_, emoji->ExtendedCodepoint_);
            Timer_->start();
        }
    });
}

SmilesPanel::~SmilesPanel()
{

}

void SmilesPanel::predefined(const QString& aimId, const QString& url)
{
    Timer_->stop();
    send();
    AimId_ = aimId;
    Url_ = url;
}

void SmilesPanel::send()
{
    QString text = Text_->getPlainText();
    Text_->clear();
    if (!text.isEmpty())
    {
        text = Url_ + " " + text;
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set<QString>("contact", AimId_);
        collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
        Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
    }
}

void SmilesPanel::paintEvent(QPaintEvent * e)
{

}

void SmilesPanel::resizeEvent(QResizeEvent * e)
{
    QSize s = size();
    s.setWidth(s.width() - Utils::scale_value(CONTROL_PANEL_SPACING * 2));
    s.setHeight(s.height() - Utils::scale_value(CONTROL_PANEL_SPACING * 2));
    Model_->resize(s, true);
    View_->setFixedSize(width() - Utils::scale_value(CONTROL_PANEL_SPACING * 2), Model_->getNeedHeight());
    Scroll_->setFixedWidth(width() - Utils::scale_value(CONTROL_PANEL_SPACING * 2));
    EmojiLabel_->setFixedWidth(width() / 2 - Utils::scale_value(OPEN_SMILES_WIDTH) / 2 - Utils::scale_value(CONTROL_PANEL_SPACING));

    return QWidget::resizeEvent(e);
}

void SmilesPanel::mouseReleaseEvent(QMouseEvent * e)
{
    emit closeClicked();
}

SmileAnimation::SmileAnimation(QWidget* parent, const QString& prop, QImage smile, int xMin, int xMax, int yMin, int yMax)
    : QPropertyAnimation(parent, prop.toUtf8())
    , Smile_(smile)
    , XMin_(xMin)
    , XMax_(xMax)
    , YMin_(yMin)
    , YMax_(yMax)
{
    Smile_ = Smile_.transformed(QTransform().rotate(-30));
    setStartValue(ANIMATION_START);
    setEndValue(ANIMATION_END);
    setDuration(ANIMATION_DURATION);
    setEasingCurve(QEasingCurve::OutCirc);
}

void SmileAnimation::paint(QPainter& painter)
{
    painter.save();
    painter.setOpacity(calcOpacity());
    auto x = calcX();
    auto y = YMin_ - (YMin_ - YMax_) * (currentValue().toFloat() / ANIMATION_END);
    Utils::check_pixel_ratio(Smile_);
    painter.drawImage(QRect(x, y, Utils::scale_value(SMILE_SIZE), Utils::scale_value(SMILE_SIZE)), Smile_, Smile_.rect());
    painter.restore();
}

float SmileAnimation::calcOpacity()
{
    static float end = ANIMATION_END / 2;
    if (currentValue().toInt() > end)
        return 1.0;

    return currentValue().toFloat() / end;
}

int SmileAnimation::calcX()
{
    static int end = ANIMATION_END / 2;
    if (currentValue().toInt() > end)
        return XMax_;

    if (XMax_ > XMin_)
        return XMin_ + (XMax_ - XMin_) * (currentValue().toFloat() / end);
    else
        return XMin_ - (XMin_ - XMax_) * (currentValue().toFloat() / end);
}

AnimationArea::AnimationArea(QWidget* parent)
    : QWidget(parent)
    , prop1_(0)
    , prop2_(0)
    , prop3_(0)
    , prop4_(0)
    , prop5_(0)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void AnimationArea::addAnimation(QImage smile, int xMin, int xMax, int yMin, int yMax)
{
    if (Animations_.size() < MAX_ANIMATIONS_COUNT)
    {
        auto name = propNameById(Animations_.size() + 1);
        auto anim = new SmileAnimation(this, name, smile, xMin, xMax, yMin, yMax);
        Animations_.push_back(anim);
        anim->start();
        return;
    }

    for (int i = 0; i < MAX_ANIMATIONS_COUNT; ++i)
    {
        if (Animations_[i]->state() == QAbstractAnimation::Stopped)
        {
            auto name = propNameById(i + 1);
            auto anim = new SmileAnimation(this, name, smile, xMin, xMax, yMin, yMax);
            Animations_[i]->deleteLater();
            Animations_[i] = anim;
            anim->start();
            return;
        }
    }

    auto name = propNameById(1);
    auto anim = new SmileAnimation(this, name, smile, xMin, xMax, yMin, yMax);
    Animations_[0]->deleteLater();
    Animations_[0] = anim;
    anim->start();
}

void AnimationArea::paintEvent(QPaintEvent * e)
{
    QPainter p(this);
    for (auto a : Animations_)
    {
        if (a->state() != QAbstractAnimation::Stopped)
            a->paint(p);
    }
}

QString AnimationArea::propNameById(int id)
{
    switch (id)
    {
    case 1:
        return "prop1";
    case 2:
        return "prop2";
    case 3:
        return "prop3";
    case 4:
        return "prop4";
    case 5:
        return "prop6";
    }
     
    return "prop1";
}

SnapsPage::SnapsPage(QWidget* parent)
    : QWidget(parent)
    , Prev_(false)
    , CurrentMedia_(0)
    , Prop_(0)
    , Error_(0)
    , ref_(new bool(false))
{
    Player_ = new FFMpegPlayer(this, false, true);
    Player_->setFillColor(Qt::black);
    Player_->setImageDuration(IMAGE_DURATION);
    Player_->setUpdatePositionRate(50);
    QPainterPath p;
    p.addRect(rect());
    Player_->setClippingPath(p);
    
    Preview_ = new PreviewWidget(this);
    Player_->stackUnder(Preview_);
    Preview_->hide();

    Progress_ = new ProgressBar(this);
    Player_->stackUnder(Progress_);
    Progress_->stackUnder(Preview_);
    Progress_->setFixedHeight(Utils::scale_value(PROGRESS_HEIGHT));
    Progress_->setFriednly("");
    Progress_->hide();

    Control_ = new ControlPanel(this);
    Player_->stackUnder(Control_);
    Control_->setFixedHeight(height());
    Control_->hide();

    MyControl_ = new MyControlPanel(this);
    Player_->stackUnder(MyControl_);
    MyControl_->setFixedHeight(Utils::scale_value(CONTROL_PANEL_HEIGHT));
    MyControl_->hide();

    Message_ = new MessagePanel(this);
    Player_->stackUnder(Message_);
    Message_->setFixedHeight(Utils::scale_value(MESSAGE_HEIGHT + CLOSE_BUTTON_HEIGHT));
    Message_->hide();

    Smiles_ = new SmilesPanel(this);
    Player_->stackUnder(Smiles_);
    Smiles_->setFixedSize(width(), height());
    Smiles_->hide();

    Animations_ = new AnimationArea(this);
    Player_->stackUnder(Animations_);
    Animations_->setFixedSize(size());

    Close_ = new CustomButton(this, ":/resources/basic_elements/close_b_100.png");
    Close_->setActiveImage(":/resources/basic_elements/close_b_100.png");
    Player_->stackUnder(Close_);
    Close_->setFixedSize(Utils::scale_value(CLOSE_BUTTON_WIDTH), Utils::scale_value(CLOSE_BUTTON_HEIGHT));

    Progress_->raise();
    Preview_->raise();
    Control_->raise();
    MyControl_->raise();
    Message_->raise();
    Smiles_->raise();
    Animations_->raise();
    Close_->raise();

    FakeProgress_ = new QPropertyAnimation(this, "prop");
    FakeProgress_->setStartValue(0);
    FakeProgress_->setEndValue(IMAGE_DURATION);
    FakeProgress_->setDuration(IMAGE_DURATION);

    connect(Logic::GetSnapStorage(), SIGNAL(playSnap(QString, QString, QString, QString, qint64, bool)), this, SLOT(playSnap(QString, QString, QString, QString, qint64, bool)), Qt::QueuedConnection);
    connect(Player_, SIGNAL(durationChanged(qint64)), Progress_, SLOT(durationChanged(qint64)), Qt::UniqueConnection);
    connect(Player_, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)), Qt::UniqueConnection);
    connect(Player_, SIGNAL(mediaChanged(qint32)), this, SLOT(mediaChanged(qint32)), Qt::QueuedConnection);
    connect(Player_, SIGNAL(dataReady()), this, SLOT(hidePreview()), Qt::QueuedConnection);
    connect(Player_, SIGNAL(streamsOpenFailed(uint32_t)), this, SLOT(streamsOpenFailed(uint32_t)), Qt::QueuedConnection);
    connect(Control_, SIGNAL(messageClicked()), this, SLOT(messageClicked()), Qt::QueuedConnection);
    connect(Control_, SIGNAL(openSmilesClicked()), this, SLOT(smilesClicked()), Qt::QueuedConnection);
    connect(Control_, SIGNAL(forwardClicked()), this, SLOT(forwardClicked()), Qt::QueuedConnection);
    connect(Message_, SIGNAL(needMove()), this, SLOT(moveMessage()), Qt::DirectConnection);
    connect(Smiles_, SIGNAL(closeClicked()), this, SLOT(closeClicked()), Qt::QueuedConnection);
    connect(Message_, SIGNAL(closeClicked()), this, SLOT(closeClicked()), Qt::QueuedConnection);
    connect(Control_, SIGNAL(animation(QImage, int, int, int, int)), this, SLOT(addAnimation(QImage, int, int, int, int)), Qt::QueuedConnection);
    connect(Smiles_, SIGNAL(animation(QImage, int, int, int, int)), this, SLOT(addAnimation(QImage, int, int, int, int)), Qt::QueuedConnection);

    connect(MyControl_, SIGNAL(save()), this, SLOT(save()), Qt::QueuedConnection);
    connect(MyControl_, SIGNAL(deleteSnap()), this, SLOT(deleteSnap()), Qt::QueuedConnection);
    connect(MyControl_, SIGNAL(forwardClicked()), this, SLOT(forwardClicked()), Qt::QueuedConnection);

    connect(Close_, SIGNAL(clicked()), this, SIGNAL(close()), Qt::QueuedConnection);

    connect(Logic::GetSnapStorage(), SIGNAL(tvStarted()), this, SLOT(showPreview()), Qt::DirectConnection);

    NextTimer_ = new QTimer(this);
    NextTimer_->setSingleShot(true);
    NextTimer_->setInterval(IMAGE_DURATION);
    connect(NextTimer_, SIGNAL(timeout()), this, SLOT(next()), Qt::QueuedConnection);
}

SnapsPage::~SnapsPage()
{
    clear();
}

void SnapsPage::stop()
{
    if (Player_->getLastMedia() != 0)
    {
        clear();
    }
}

void SnapsPage::notifyApplicationWindowActive(const bool isActive)
{
    if (isActive && Player_->state() == QMovie::Paused)
        Player_->play(true);
    else if (!isActive && Player_->state() == QMovie::Running)
        Player_->pause();
}

void SnapsPage::setProp(int val)
{
    Prop_ = val;
    Progress_->positionChanged(Prop_);
}

void SnapsPage::resizeEvent(QResizeEvent * e)
{
    Animations_->setFixedSize(size());
    Animations_->move(0, 0);

    int pHeight = height() - Utils::scale_value(SNAP_TOP_PADDING + CONTROL_PANEL_HEIGHT + CONTROL_PANEL_SPACING * 2);
    auto s = QSize(pHeight * 0.56, pHeight);
    QPainterPath p;
    auto r = QRect(0, 0, s.width(), s.height());
    p.addRect(r);
    Player_->setClippingPath(p);

    Player_->setFixedSize(s);
    Preview_->setFixedSize(s);
    Player_->move(width() / 2 - Player_->width() / 2, Utils::scale_value(SNAP_TOP_PADDING));
    Preview_->move(width() / 2 - Preview_->width() / 2, Utils::scale_value(SNAP_TOP_PADDING));

    Progress_->setFixedWidth(Player_->width());
    Progress_->move(width() / 2 - Progress_->width() / 2, 0);

    Control_->setFixedHeight(height());
    Control_->setFixedWidth(Player_->width());
    Control_->move(width() / 2 - Control_->width() / 2, 0);
    Control_->setLeftPadding(width() / 2 - Control_->width() / 2);

    MyControl_->setFixedWidth(Player_->width());
    MyControl_->move(width() / 2 - MyControl_->width() / 2, height() - Utils::scale_value(CONTROL_PANEL_HEIGHT) - Utils::scale_value(CONTROL_PANEL_SPACING));

    updatePreviews();

    Close_->move(width() - Utils::scale_value(CLOSE_BUTTON_WIDTH), Utils::scale_value(CLOSE_BUTTON_HEIGHT) / 2);

    Message_->setFixedWidth(Player_->width());
    moveMessage();

    Smiles_->setFixedSize(Player_->width(), height());
    Smiles_->move(width() / 2 - Smiles_->width() / 2, 0);

    return QWidget::resizeEvent(e);
}

void SnapsPage::mouseReleaseEvent(QMouseEvent * e)
{
    if (Prev_)
        return QWidget::mouseReleaseEvent(e);

    if (QRect(0, 0, width() / 3, height()).contains(e->pos()))
    {
        uint32_t id = Player_->getMedia();
        if (id == 0 && !UserMediaId_.empty())
            id = UserMediaId_.lastKey();

        auto snap = UserMediaId_[--id];
        if (!snap.AimId_.isEmpty())
        {
            QFileInfo f(snap.Local_);
            Player_->openMedia(snap.Local_, Utils::is_image_extension_not_gif(f.suffix()), id);
            connect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()), Qt::UniqueConnection);
            Prev_ = true;
        }
    }
    else if (QRect(width() / 3, 0, width(), height()).contains(e->pos()) && !Player_->queueIsEmpty())
    {
        next();
    }
    return QWidget::mouseReleaseEvent(e);
}

void SnapsPage::playSnap(QString _path, QString _aimid, QString _originalAimdId, QString _url, qint64 _id, bool _first)
{
    QFileInfo f(_path);
    if (_first)
    {
        connect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()), Qt::UniqueConnection);
        Player_->clearQueue();
        UserMediaId_.clear();
        Queue_.clear();
        Progress_->resetCurrent();
    }

    SnapId id;
    id.AimId_ = _aimid;
    id.OriginalAimdId_ = _originalAimdId;
    id.Id_ = _id;
    id.Url_ = _url;
    id.Local_ = _path;
    id.First_ = _first;

    addPreview(_aimid, _id);

    if (Player_->queueSize() < MAX_PLAYER_QUEUE_SIZE)
    {
        Player_->openMedia(_path, Utils::is_image_extension_not_gif(f.suffix()));
        auto lastId = Player_->getLastMedia();
        UserMediaId_[lastId] = id;

        auto cur = Player_->getMedia();
        if (cur == lastId)
        {
            return;
        }
    }
    else
    {
        Queue_.push_back(id);
    }
}

void SnapsPage::fileLoaded()
{
    disconnect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()));
    Progress_->show();
    auto my = Current_ == MyInfo()->aimId();
    Control_->setVisible(!my);
    MyControl_->setVisible(my);
    if (Prev_)
    {
        int id = 0;
        if (Player_->state() == QMovie::NotRunning)
        {
            auto s = UserMediaId_[Player_->getMedia()];
            if (s.Failed_)
            {
                id = Player_->getMedia();
                Player_->loadFromQueue();
            }
            else if (!UserMediaId_.empty())
            {
                id = UserMediaId_.lastKey();
            }
        }
        else
        {
            id = Player_->stop();
            Player_->loadFromQueue();
        }

        Player_->play(true);

        QTimer::singleShot(100, [this, id]() //rework
        {
            auto snap = UserMediaId_[id];
            if (!snap.AimId_.isEmpty())
            {
                QFileInfo f(snap.Local_);
                Player_->openMedia(snap.Local_, Utils::is_image_extension_not_gif(f.suffix()), id);
                Prev_ = false;
            }
        });
        return;
    }

    if (Player_->state() == QMovie::NotRunning)
    {
        Player_->play(true);
    }
    else
    {
        next();
    }
}

void SnapsPage::mediaChanged(qint32 _id)
{
    NextTimer_->stop();
    if (Player_->queueSize() < MAX_PLAYER_QUEUE_SIZE && !Queue_.isEmpty())
    {
        auto id = Queue_.front();
        Queue_.pop_front();
        QFileInfo f(id.Local_);
        Player_->openMedia(id.Local_, Utils::is_image_extension_not_gif(f.suffix()));
        UserMediaId_[Player_->getLastMedia()] = id;
    }

    if (_id == -1)
    {
        if (Logic::GetSnapStorage()->haveLoading())
            showPreview();
        return;
    }

    FakeProgress_->stop();
    connect(Player_, SIGNAL(durationChanged(qint64)), Progress_, SLOT(durationChanged(qint64)), Qt::UniqueConnection);
    connect(Player_, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)), Qt::UniqueConnection);

    Player_->resetPosition();

    auto id = UserMediaId_.value(_id);
    if (!id.AimId_.isEmpty())
    {
        QFileInfo f(id.Local_);
        if (f.suffix().toLower() == "gif")
        {
            Progress_->durationChanged(IMAGE_DURATION);
            Progress_->positionChanged(0);
            disconnect(Player_, SIGNAL(durationChanged(qint64)), Progress_, SLOT(durationChanged(qint64)));
            disconnect(Player_, SIGNAL(positionChanged(qint64)), this, SLOT(positionChanged(qint64)));
            FakeProgress_->start();
        }

        if (Current_ != id.AimId_)
        {
            int cur = 0;
            for (auto i : UserMediaId_)
            {
                if (i.Id_ == id.Id_)
                    break;

                if (i.AimId_ == id.AimId_)
                    ++cur;
            }

            Progress_->setCurrent(cur);
            emit currentChanged(id.AimId_, id.New_);
        }
        else
        {
            if (_id > CurrentMedia_)
            {
                if (!id.First_)
                    Progress_->next();
            }
            else
            {
                Progress_->prev();
            }
        }

        int snapCount = 0;
        for (auto i : UserMediaId_)
        {
            if (i.AimId_ == id.AimId_)
                ++snapCount;
        }
        for (auto i : Queue_)
        {
            if (i.AimId_ == id.AimId_)
                ++snapCount;
        }
        Progress_->setCount(snapCount + Logic::GetSnapStorage()->loadingSnapsCount(id.AimId_));

        Current_ = id.AimId_;
        CurrentUrl_ = id.Url_;
        CurrentLocal_ = id.Local_;
        CurrentId_ = id.Id_;
        CurrentMedia_ = _id;

        closeClicked();

        auto aimId = id.OriginalAimdId_.isEmpty() ? id.AimId_ : id.OriginalAimdId_;
        Progress_->setFriednly(Logic::GetSnapStorage()->getFriednly(aimId));
        Progress_->setAimId(aimId);
        Progress_->setViews(Logic::GetSnapStorage()->getViews(id.Id_));
        Progress_->setTimestamp(Logic::GetSnapStorage()->getTimestamp(id.Id_));
        Control_->predefined(aimId, id.Url_);
        Message_->predefined(aimId, id.Url_);
        Smiles_->predefined(aimId, id.Url_);
        Ui::GetDispatcher()->read_snap(id.AimId_, id.Id_, true);

        if (id.Failed_)
            showFailPreview(_id);
    }

    updatePreviews();
}

void SnapsPage::messageClicked()
{
    Control_->hide();
    MyControl_->hide();
    Smiles_->hide();
    Message_->show();
    Message_->setFocus();
    Player_->setReplay(true);
}

void SnapsPage::smilesClicked()
{
    Control_->hide();
    MyControl_->hide();
    Smiles_->show();
    Message_->hide();
    Player_->setReplay(true);
}

void SnapsPage::closeClicked()
{
    Message_->hide();
    Smiles_->hide();
    auto my = Current_ == MyInfo()->aimId();
    Control_->setVisible(!my);
    MyControl_->setVisible(my);
    Player_->setReplay(false);
}

void SnapsPage::moveMessage()
{
    Message_->move(width() / 2 - Message_->width() / 2, height() - Message_->height() - Utils::scale_value(CONTROL_PANEL_SPACING));
}

void SnapsPage::addAnimation(QImage smile, int xMin, int xMax, int yMin, int yMax)
{
    Animations_->addAnimation(smile, xMin, xMax, yMin, yMax);
}

void SnapsPage::forwardClicked()
{
    Player_->pause();
    auto url = Preview_->isVisible() ? PreviewUrl_ : CurrentUrl_;
    if (!url.isEmpty())
    {
        Ui::SelectContactsWidget shareDialog(
            nullptr,
            Logic::MembersWidgetRegim::SHARE_LINK,
            QT_TRANSLATE_NOOP("popup_window", "Share link"),
            QT_TRANSLATE_NOOP("popup_window", "Copy link and close"),
            url,
            Ui::MainPage::instance(),
            true);

        const auto action = shareDialog.show();
        if (action == QDialog::Accepted)
        {
            const auto contact = shareDialog.getSelectedContact();
            if (contact != "")
            {
                Logic::getContactListModel()->setCurrent(contact, -1, true, true);
                Ui::GetDispatcher()->sendMessageToContact(contact, url);
            }
            else
            {
                QApplication::clipboard()->setText(url);
            }
        }
    }
    Player_->play(true);
}

void SnapsPage::save()
{
    std::weak_ptr<bool> wr_ref = ref_;
    Utils::saveAs(CurrentLocal_, [this, wr_ref](const QString& file, const QString& dir)
    {
        auto ref = wr_ref.lock();
        if (!ref)
            return;

        QString fullname = dir;
        const auto addTrailingSlash = (!dir.endsWith('\\') && !dir.endsWith('/'));
        if (addTrailingSlash)
        {
            fullname += "/";
        }
        fullname += file;
        QFile::remove(fullname);

        auto aimId = Preview_->isVisible() ? PreviewAimId_ : Current_;
        auto url = Preview_->isVisible() ? PreviewUrl_ : CurrentUrl_;

        Ui::GetDispatcher()->downloadSharedFile(url, false, fullname);
    });
}

void SnapsPage::deleteSnap()
{
    if (Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("snaps_page", "Cancel"),
        QT_TRANSLATE_NOOP("snaps_page", "Yes"),
        QT_TRANSLATE_NOOP("snaps_page", "Are you sure you want to delete this snap?"),
        QT_TRANSLATE_NOOP("snaps_page", "Delete snap"),
        nullptr))
    {
        auto id = Preview_->isVisible() ? PreviewId_ : CurrentId_;
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_int64("snap_id", id);
        Ui::GetDispatcher()->post_message_to_core("snap/delete", collection.get());

        Logic::GetSnapStorage()->startTv(0, 0);
    }
}

void SnapsPage::positionChanged(qint64 pos)
{
    Progress_->positionChanged(pos > 0 ? pos : 0);
}

void SnapsPage::clear()
{
    Player_->stop();
    Player_->clearQueue();
    UserMediaId_.clear();
    Queue_.clear();
    clearPreviewWidgets();
}

void SnapsPage::showPreview()
{
    auto snap = Logic::GetSnapStorage()->getLoadingSnap();
    if (snap.AimId_.isEmpty())
        return;

    Preview_->setPreview(snap.Preview_);
    Preview_->setAimId(snap.AimId_);
    Preview_->show();

    auto aimId = snap.OriginalAimId_.isEmpty() ? snap.AimId_ : snap.OriginalAimId_;
            
    PreviewAimId_ = aimId;
    PreviewUrl_ = snap.Url_;
    PreviewId_ = snap.Id_;
            
    Progress_->resetCurrent();
    Progress_->setCount(Logic::GetSnapStorage()->loadingSnapsCount(snap.AimId_));
    Progress_->durationChanged(1);
    Progress_->setFriednly(Logic::GetSnapStorage()->getFriednly(snap.AimId_));
    Progress_->setAimId(snap.AimId_);
    Progress_->setViews(Logic::GetSnapStorage()->getViews(snap.Id_));
    Progress_->setTimestamp(Logic::GetSnapStorage()->getTimestamp(snap.Id_));
    Control_->predefined(aimId, snap.Url_);
    Message_->predefined(aimId, snap.Url_);
    Smiles_->predefined(aimId, snap.Url_);

    Progress_->show();
    auto my = Current_ == MyInfo()->aimId();
    Control_->setVisible(!my);
    MyControl_->setVisible(my);
}

void SnapsPage::showFailPreview(uint32_t mediaId)
{
    auto id = UserMediaId_[mediaId];

    QPixmap p(Preview_->size());
    QPainter painter(&p);
    painter.fillRect(p.rect(), Qt::white);
    
    if (!Error_)
    {
        Error_ = new TextEditEx(this, Fonts::appFontScaled(17, Fonts::FontWeight::Medium), QColor(Qt::black), false, false);
        Error_->setFrameStyle(QFrame::NoFrame);
        Error_->setStyleSheet("background: transparent;");
        Error_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Error_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Error_->hide();

        Error_->setPlainText(QT_TRANSLATE_NOOP("snaps_page", "Unfortunately, this file is not supported on your device."));
        Error_->adjustHeight(width());
        auto cursor = Error_->textCursor();
        auto textBlockFormat = cursor.blockFormat();
        textBlockFormat.setAlignment(Qt::AlignCenter);
        cursor.mergeBlockFormat(textBlockFormat);
        Error_->setTextCursor(cursor);
    }

    Error_->setFixedWidth(width());
    Error_->render(&painter, QPoint(0, p.height() / 2));
      
    Preview_->setPreview(p);
    Preview_->show();

    auto aimId = id.OriginalAimdId_.isEmpty() ? id.AimId_ : id.OriginalAimdId_;

    PreviewAimId_ = aimId;
    PreviewUrl_ = id.Url_;
    PreviewId_ = id.Id_;

    Progress_->setFriednly(Logic::GetSnapStorage()->getFriednly(id.AimId_));
    Progress_->setAimId(id.AimId_);
    Progress_->setViews(Logic::GetSnapStorage()->getViews(id.Id_));
    Progress_->setTimestamp(Logic::GetSnapStorage()->getTimestamp(id.Id_));
    Control_->predefined(aimId, id.Url_);
    Message_->predefined(aimId, id.Url_);
    Smiles_->predefined(aimId, id.Url_);

    Progress_->show();
    auto my = Current_ == MyInfo()->aimId();
    Control_->setVisible(!my);
    MyControl_->setVisible(my);

    emit currentChanged(id.AimId_, id.New_);

    NextTimer_->start();

    Progress_->durationChanged(IMAGE_DURATION);
    Progress_->positionChanged(0);
    FakeProgress_->start();
}

void SnapsPage::hidePreview()
{
    auto w = Utils::InterConnector::instance().getMainWindow();
    if (!w || !w->isActive()) 
    {
        Player_->pause();
    }

    Preview_->hide();
}

void SnapsPage::streamsOpenFailed(uint32_t mediaId)
{
    if (Prev_)
    {
        Prev_ = false;
        disconnect(Player_, SIGNAL(fileLoaded()), this, SLOT(fileLoaded()));
        showFailPreview(mediaId);

        int id =  Player_->stop();
        Player_->loadFromQueue();

        QTimer::singleShot(100, [this, id]() //rework
        {
            auto snap = UserMediaId_[id];
            if (!snap.AimId_.isEmpty())
            {
                QFileInfo f(snap.Local_);
                Player_->openMedia(snap.Local_, Utils::is_image_extension_not_gif(f.suffix()), id);
                Prev_ = false;
            }
        });
    }

    UserMediaId_[mediaId].Failed_ = true;
}

void SnapsPage::next()
{
    Player_->stop();
    Player_->loadFromQueue();
    Player_->play(true);
}

void SnapsPage::updatePreviews()
{
    if (PreviewWidgets_.isEmpty())
        return;

    for (auto w : PreviewWidgets_)
    {
        w->hide();
    }

    int spaceLeft = width() / 2 - Player_->width() / 2;
    int spaceRight = width() / 2 + Player_->width() / 2;

    int curPos = 0;
    if (!Current_.isEmpty())
    {
        for (auto w : PreviewWidgets_)
        {
            if (w->getAimId() == Current_)
                break;

            ++curPos;
        }
    }

    int tmp = curPos;
    while (--tmp != -1 && spaceLeft > 0)
    {
        auto w = PreviewWidgets_.at(tmp);

        w->setFixedSize(QSize(height() * 0.34, height() * 0.6));
        w->show();

        spaceLeft -= Utils::scale_value(PREVIEW_OFFSET);
        spaceLeft -= w->width();

        w->move(spaceLeft, height() / 2 - w->height() / 2);
    }

    while (++curPos <= PreviewWidgets_.size() - 1 && spaceRight < width())
    {
        auto w = PreviewWidgets_.at(curPos);

        w->setFixedSize(QSize(height() * 0.34, height() * 0.6));
        w->show();

        spaceRight += Utils::scale_value(PREVIEW_OFFSET);

        w->move(spaceRight, height() / 2 - w->height() / 2);
        spaceRight += w->width();
    }
}

void SnapsPage::clearPreviewWidgets()
{
    for (auto w : PreviewWidgets_)
        w->hide();

    PreviewWidgets_.clear();
}

void SnapsPage::addPreview(const QString& _aimId, qint64 id)
{
    for (auto p : PreviewWidgets_)
    {
        if (p->getAimId() == _aimId)
            return;
    }

    auto preview = new PreviewWidget(this);

    preview->setAimId(_aimId);
    preview->setPreview(Logic::GetSnapStorage()->getSnapPreviewFull(id));

    PreviewWidgets_.push_back(preview);

    updatePreviews();
}

}