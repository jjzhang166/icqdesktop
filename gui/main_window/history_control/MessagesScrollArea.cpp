#include "stdafx.h"

#include "../ContactDialog.h"

#include "../smiles_menu/SmilesMenu.h"

#include "../../utils/InterConnector.h"
#include "../../utils/log/log.h"
#include "../../utils/profiling/auto_stop_watch.h"
#include "../../utils/utils.h"

#include "MessageItem.h"
#include "MessagesScrollbar.h"
#include "MessagesScrollAreaLayout.h"

#include "MessagesScrollArea.h"

namespace
{
    double getMaximumScrollDistance();

    double getMinimumSpeed();

    double getMaximumSpeed();

    template<class T>
    T sign(const T value)
    {
        static_assert(
            std::is_arithmetic<T>::value,
            "std::is_arithmetic<T>::value");

        static_assert(
            std::numeric_limits<T>::is_signed,
            "std::numeric_limits<T>::is_signed");

        if (value == 0)
        {
            return 0;
        }

        return ((value > 0) ? 1 : -1);
    }

    double cubicOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed);

    double expoOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed);

    const auto MOMENT_UNINITIALIZED = std::numeric_limits<int64_t>::min();

    const auto IDLE_USER_ACTIVITY_TIMEOUT_MS = (build::is_debug() ? 1000 : 10000);

    const auto scrollEasing = expoOut;
}

namespace Ui
{

    enum class MessagesScrollArea::ScrollingMode
    {
        Min,

        Plain,
        Selection,

        Max
    };

    MessagesScrollArea::MessagesScrollArea(QWidget *parent, QWidget *typingWidget)
        : QWidget(parent)
        , Scrollbar_(new MessagesScrollbar(this))
        , ScrollAnimationTimer_(this)
        , LastAnimationMoment_(MOMENT_UNINITIALIZED)
        , IsSelecting_(false)
        , ScrollDistance_(0)
        , Mode_(ScrollingMode::Plain)
        , Resizing_(false)
        , TouchScrollInProgress_(false)
        , IsUserActive_(false)
        , UserActivityTimer_(this)
        , Layout_(new MessagesScrollAreaLayout(this, Scrollbar_, typingWidget))
    {
        assert(parent);
        assert(Layout_);
        assert(typingWidget);

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setContentsMargins(0, 0, 0, 0);
        setProperty("MessagesWidget", true);
        setLayout(Layout_);

        // initially invisible, will became visible when there will be enough content
        Scrollbar_->setVisible(false);

        ScrollAnimationTimer_.setInterval(10);
        ScrollAnimationTimer_.setTimerType(Qt::CoarseTimer);

        auto success = QObject::connect(
            &ScrollAnimationTimer_, &QTimer::timeout,
            this, &MessagesScrollArea::onAnimationTimer,
            Qt::QueuedConnection
        );
        assert(success);

        WheelEventsBufferResetTimer_.setInterval(300);
        WheelEventsBufferResetTimer_.setTimerType(Qt::PreciseTimer);

        success = QObject::connect(
            &WheelEventsBufferResetTimer_, &QTimer::timeout,
            this, &MessagesScrollArea::onWheelEventResetTimer,
            Qt::QueuedConnection
        );

        success = QObject::connect(
            Scrollbar_, &QScrollBar::sliderPressed,
            this, &MessagesScrollArea::onSliderPressed
        );
        assert(success);

        success = QObject::connect(
            Scrollbar_, &QScrollBar::valueChanged,
            this, &MessagesScrollArea::onSliderValue
        );
        assert(success);

        success = QObject::connect(
            Scrollbar_, &QScrollBar::sliderMoved,
            this, &MessagesScrollArea::onSliderMoved
        );
        assert(success);

        UserActivityTimer_.setInterval(IDLE_USER_ACTIVITY_TIMEOUT_MS);

        connect(&UserActivityTimer_, &QTimer::timeout,
                this, &MessagesScrollArea::onIdleUserActivityTimeout,
                Qt::QueuedConnection);

        Utils::grabTouchWidget(this);
    }

    void MessagesScrollArea::insertWidget(const Logic::MessageKey &key, QWidget *widget)
    {
        WidgetsList widgets;
        widgets.emplace_back(
            std::make_pair(key, widget)
        );

        insertWidgets(widgets);
    }

    void MessagesScrollArea::insertWidgets(const WidgetsList& _widgets)
    {
        assert(!_widgets.empty());
        Layout_->insertWidgets(_widgets);

        updateScrollbar();
    }

    bool MessagesScrollArea::isSelecting() const
    {
        return IsSelecting_;
    }

    bool MessagesScrollArea::isViewportFull() const
    {
        const auto scrollBounds = Layout_->getViewportScrollBounds();

        const auto scrollRange = (scrollBounds.second - scrollBounds.first);
        assert(scrollRange >= 0);

        return (scrollRange > 0);
    }

    bool MessagesScrollArea::containsWidget(QWidget *widget) const
    {
        assert(widget);

        return Layout_->containsWidget(widget);
    }

    void MessagesScrollArea::removeWidget(QWidget *widget)
    {
        assert(widget);

        Layout_->removeWidget(widget);

        updateScrollbar();
    }

    void MessagesScrollArea::replaceWidget(const Logic::MessageKey &key, QWidget *widget)
    {
        assert(key.hasId());
        assert(widget);

        auto existingWidget = getItemByKey(key);
        if (!existingWidget)
        {
            return;
        }

        removeWidget(existingWidget);

        insertWidget(key, widget);
    }

    bool MessagesScrollArea::touchScrollInProgress() const
    {
        return TouchScrollInProgress_ || isScrolling();
    }

    void MessagesScrollArea::scrollToBottom()
    {
        stopScrollAnimation();

        if (!Layout_->isViewportAtBottom())
        {
            Layout_->setViewportByOffset(0);
        }

        resetUserActivityTimer();
    }

    void MessagesScrollArea::updateItemKey(const Logic::MessageKey &key)
    {
        Layout_->updateItemKey(key);
    }

    void MessagesScrollArea::updateScrollbar()
    {
        const auto viewportScrollBounds = Layout_->getViewportScrollBounds();

        auto scrollbarMaximum = (viewportScrollBounds.second - viewportScrollBounds.first);
        assert(scrollbarMaximum >= 0);

        const auto scrollbarVisible = (scrollbarMaximum > 0);
        Scrollbar_->setVisible(scrollbarVisible);

        Scrollbar_->setMaximum(scrollbarMaximum);

        if (!scrollbarVisible)
        {
            return;
        }

        const auto viewportAbsY = Layout_->getViewportAbsY();
        const auto scrollPos = (viewportAbsY - viewportScrollBounds.first);
        assert(scrollPos >= 0);

        Scrollbar_->setValue(scrollPos);
    }

    void MessagesScrollArea::cancelSelection()
    {
        IsSelecting_ = false;
    }

    void MessagesScrollArea::cancelWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.stop();
    }

    void MessagesScrollArea::enumerateMessagesItems(const MessageItemVisitor visitor, const bool reversed) const
    {
        assert(visitor);
        Layout_->enumerateMessagesItems(visitor, reversed);
    }

    void MessagesScrollArea::enumerateWidgets(const WidgetVisitor visitor, const bool reversed) const
    {
        assert(visitor);
        Layout_->enumerateWidgets(visitor, reversed);
    }

    QWidget* MessagesScrollArea::getItemByPos(const int32_t pos) const
    {
        return Layout_->getItemByPos(pos);
    }

    QWidget* MessagesScrollArea::getItemByKey(const Logic::MessageKey &key) const
    {
        return Layout_->getItemByKey(key);
    }

    int32_t MessagesScrollArea::getItemsCount() const
    {
        return Layout_->getItemsCount();
    }

    Logic::MessageKeyVector MessagesScrollArea::getItemsKeys() const
    {
        return Layout_->getItemsKeys();
    }

    QString MessagesScrollArea::getSelectedText() const
    {
        QString selection_text;
        QString first_message_text_only;
        QString first_message_full;

        enumerateMessagesItems(
            [&selection_text, &first_message_text_only, &first_message_full]
            (Ui::MessageItem* _item, const bool)
            {
                if (first_message_text_only.isEmpty())
                {
                    first_message_text_only = _item->selection(true);
                    first_message_full = _item->selection(false);
                }
                else
                {
                    selection_text += _item->selection();
                }

                return true;

            }, true
        );

        QString result = (selection_text.isEmpty() ? first_message_text_only : (first_message_full + selection_text));
        if (!result.isEmpty() && result.endsWith("\n\n"))
            result = result.left(result.length() - 1);

        return result;
    }

    QList<Logic::MessageKey> MessagesScrollArea::getKeysToUnload() const
    {
        if (!Layout_->isViewportAtBottom())
        {
            return QList<Logic::MessageKey>();
        }

        const auto itemsHeight = Layout_->getItemsHeight();

        const auto triggerThresholdK = 2.5;
        const auto triggerThreshold = (int32_t)(Layout_->getViewportHeight() * triggerThresholdK);
        assert(triggerThreshold > 0);

        const auto triggerThreasholdNotReached = (itemsHeight < triggerThreshold);
        if (triggerThreasholdNotReached)
        {
            return QList<Logic::MessageKey>();
        }

        const auto unloadThresholdK = 2.8;
        const auto unloadThreshold = (int32_t)(Layout_->getViewportHeight() * unloadThresholdK);
        assert(unloadThreshold > 0);

        return Layout_->getWidgetsOverBottomOffset(unloadThreshold);
    }

    void MessagesScrollArea::mouseMoveEvent(QMouseEvent *e)
    {
        QWidget::mouseMoveEvent(e);

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);

        if (!isLeftButton || !isGenuine)
        {
            return;
        }

        resetUserActivityTimer();

        LastMouseGlobalPos_ = e->globalPos();

        applySelection();

        auto mouseY = e->pos().y();

        const auto topScrollStartPosition = 0;

        const auto scrollUp = (mouseY <= topScrollStartPosition);
        if (scrollUp)
        {
            ScrollDistance_ = mouseY;
            startScrollAnimation(ScrollingMode::Selection);
            return;
        }

        const auto bottomScrollStartPosition = (height());
        assert(bottomScrollStartPosition > 0);

        const auto scrollDown = (mouseY >= bottomScrollStartPosition);
        if (scrollDown)
        {
            assert(!scrollUp);

            ScrollDistance_ = (mouseY - bottomScrollStartPosition);
            startScrollAnimation(ScrollingMode::Selection);

            return;
        }

        stopScrollAnimation();
    }

    void MessagesScrollArea::mousePressEvent(QMouseEvent *e)
    {
        resetUserActivityTimer();

        QWidget::mousePressEvent(e);

        auto contactDialog = Utils::InterConnector::instance().getContactDialog();
        auto smilesMenu = contactDialog->getSmilesMenu();

        const auto isLeftButton = ((e->buttons() & Qt::LeftButton) != 0);
        const auto isGenuine = (e->source() == Qt::MouseEventNotSynthesized);

        const auto isSelectionStart = (isLeftButton && isGenuine && smilesMenu->IsHidden());

        if (!isSelectionStart)
        {
            return;
        }

        if (!(e->modifiers() & Qt::ShiftModifier))
        {
            clearSelection();
            LastMouseGlobalPos_ = e->globalPos();
            applySelection(true);
        }
        else
        {
            auto old = LastMouseGlobalPos_;
            LastMouseGlobalPos_ = e->globalPos();
            applySelection();
            LastMouseGlobalPos_ = old;
        }

        IsSelecting_ = true;
    }

    void MessagesScrollArea::mouseReleaseEvent(QMouseEvent *e)
    {
        resetUserActivityTimer();

        QWidget::mouseReleaseEvent(e);

        IsSelecting_ = false;

        if (e->source() == Qt::MouseEventNotSynthesized)
        {
            stopScrollAnimation();
        }
    }

    void MessagesScrollArea::wheelEvent(QWheelEvent *e)
    {
        e->accept();

        resetUserActivityTimer();

        if (!Scrollbar_->isVisible())
        {
            stopScrollAnimation();
            return;
        }

        cancelWheelBufferReset();

        startScrollAnimation(ScrollingMode::Plain);

        const auto pxDelta = e->pixelDelta();
        const auto isHorMove = (
            !pxDelta.isNull() &&
            (std::abs(pxDelta.x()) >= std::abs(pxDelta.y())));

        if (isHorMove)
        {
            return;
        }

        auto delta = e->delta();
        delta /= Utils::scale_value(1);
        delta = -delta;

        if (platform::is_apple())
        {
            delta /= Utils::scale_value(2);
        }

        if (delta == 0)
        {
            return;
        }

        const auto directionChanged = enqueWheelEvent(delta);
        if (directionChanged)
        {
            ScrollDistance_ = 0;
            WheelEventsBuffer_.clear();
        }

        ScrollDistance_ += delta;

        ScrollDistance_ = std::max(ScrollDistance_, -getMaximumScrollDistance());
        ScrollDistance_ = std::min(ScrollDistance_, getMaximumScrollDistance());
    }

    bool MessagesScrollArea::event(QEvent *e)
    {
        if (e->type() == QEvent::TouchBegin)
        {
            TouchScrollInProgress_ = true;
            QTouchEvent* te = static_cast<QTouchEvent*>(e);
            PrevTouchPoint_ = te->touchPoints().first().pos();
            e->accept();
            return true;
        }
        else if (e->type() == QEvent::TouchUpdate)
        {
            QTouchEvent* te = static_cast<QTouchEvent*>(e);
            if (te->touchPointStates() & Qt::TouchPointMoved)
            {
                QPointF newPoint = te->touchPoints().first().pos();
                if (!newPoint.isNull())
                {
                    int touchDelta = newPoint.y() - PrevTouchPoint_.y();
                    PrevTouchPoint_ = newPoint;

                    startScrollAnimation(ScrollingMode::Plain);

                    const auto wheelMultiplier = -1;
                    const auto delta = (touchDelta * wheelMultiplier);
                    assert(delta != 0);

                    const auto wheelDirection = sign(delta);

                    const auto currentDirection = sign(ScrollDistance_);

                    const auto directionChanged = (wheelDirection != currentDirection);
                    if (directionChanged)
                    {
                        ScrollDistance_ = 0;
                    }

                    ScrollDistance_ += delta;
                }
            }

            e->accept();
            return true;
        }
        else if (e->type() == QEvent::TouchEnd || e->type() == QEvent::TouchCancel)
        {
            TouchScrollInProgress_ = false;
            e->accept();
            return true;
        }

        return QWidget::event(e);
    }

    void MessagesScrollArea::onAnimationTimer()
    {
        if (!ScrollAnimationTimer_.isActive())
        {
            return;
        }

        if (ScrollDistance_ == 0)
        {
            stopScrollAnimation();
            return;
        }

        const auto now = QDateTime::currentMSecsSinceEpoch();

        const auto step = evaluateScrollingStep(now);

        const auto emptyStep = ((int32_t)step == 0);
        if (!emptyStep && !Layout_->shiftViewportAbsY(step))
        {
            stopScrollAnimation();
            return;
        }

        updateScrollbar();

        const auto isPlainMode = (Mode_ == ScrollingMode::Plain);
        if (isPlainMode)
        {
            const auto newScrollDistance = (ScrollDistance_ - step);

            const auto stop = (sign(newScrollDistance) != sign(ScrollDistance_));
            if (stop)
            {
                stopScrollAnimation();
                return;
            }

            ScrollDistance_ = newScrollDistance;
        }

        LastAnimationMoment_ = now;
    }

    void MessagesScrollArea::onMessageHeightChanged(QSize oldSize, QSize newSize)
    {
        if (oldSize == newSize)
        {
            assert(false);
        }
    }

    void MessagesScrollArea::onSliderMoved(int value)
    {
        stopScrollAnimation();

        Layout_->setViewportByOffset(
            Scrollbar_->maximum() - value
        );

        resetUserActivityTimer();
    }

    void MessagesScrollArea::onSliderPressed()
    {
        stopScrollAnimation();
    }

    void MessagesScrollArea::onSliderValue(int value)
    {
        assert(value >= 0);

        if (Scrollbar_->isInFetchRange(value))
        {
            emit fetchRequestedEvent();
        }

        if (Mode_ == ScrollingMode::Selection)
        {
            applySelection();
        }

        if (isScrollAtBottom())
            emit scrollMovedToBottom();
    }

    void MessagesScrollArea::applySelection(bool forShift)
    {
        assert(!LastMouseGlobalPos_.isNull());

        enumerateMessagesItems(
            [this, forShift](Ui::MessageItem *messageItem, const bool)
            {
                messageItem->selectByPos(LastMouseGlobalPos_, forShift);

                return true;
            },
            false
        );
    }

    void MessagesScrollArea::clearSelection()
    {
        Layout_->enumerateWidgets(
            [this](QWidget *widget, const bool /*flag*/)
                {
                    if (auto item = qobject_cast<Ui::HistoryControlPageItem*>(widget))
                    {
                        item->clearSelection();
                        return true;
                    }
                    return true;
                }, false);
    }

    bool MessagesScrollArea::enqueWheelEvent(const int32_t delta)
    {
        const auto WHEEL_HISTORY_LIMIT = 3;

        const auto directionBefore = evaluateWheelHistorySign();

        WheelEventsBuffer_.emplace_back(delta);

        const auto isHistoryOverwhelmed = (WheelEventsBuffer_.size() > WHEEL_HISTORY_LIMIT);
        if (isHistoryOverwhelmed)
        {
            WheelEventsBuffer_.pop_front();
        }

        const auto directionAfter = evaluateWheelHistorySign();

        assert(WheelEventsBuffer_.size() <= WHEEL_HISTORY_LIMIT);
        const auto isHistoryFilled = (WheelEventsBuffer_.size() == WHEEL_HISTORY_LIMIT);

        const auto isSignChanged = (
            (directionAfter != 0) &&
            (directionBefore != 0) &&
            (directionAfter != directionBefore));

        return (isHistoryFilled && isSignChanged);
    }

    double MessagesScrollArea::evaluateScrollingVelocity() const
    {
        assert(ScrollDistance_ != 0);
        assert(std::abs(ScrollDistance_) <= getMaximumScrollDistance());

        auto speed = scrollEasing(
            std::abs(ScrollDistance_),
            getMaximumScrollDistance(),
            getMinimumSpeed(),
            getMaximumSpeed());

        speed *= sign(ScrollDistance_);

        return speed;
    }

    double MessagesScrollArea::evaluateScrollingStep(const int64_t now) const
    {
        assert(now > 0);

        const auto timeDiff = (now - LastAnimationMoment_);

        const auto velocity = evaluateScrollingVelocity();

        auto step = velocity;
        step *= timeDiff;
        step /= 1000;

        if (step == 0)
        {
            step = sign(velocity);
        }

        return step;
    }

    int32_t MessagesScrollArea::evaluateWheelHistorySign() const
    {
        const auto sum = std::accumulate(
            WheelEventsBuffer_.cbegin(),
            WheelEventsBuffer_.cend(),
            0);

        return sign(sum);
    }

    bool MessagesScrollArea::isScrolling() const
    {
        return ScrollAnimationTimer_.isActive();
    }

    void MessagesScrollArea::scheduleWheelBufferReset()
    {
        WheelEventsBufferResetTimer_.start();
    }

    void MessagesScrollArea::startScrollAnimation(const ScrollingMode mode)
    {
        assert(mode > ScrollingMode::Min);
        assert(mode < ScrollingMode::Max);

        Mode_ = mode;

        if (isScrolling())
        {
            return;
        }

        ScrollAnimationTimer_.start();

        assert(LastAnimationMoment_ == MOMENT_UNINITIALIZED);
        LastAnimationMoment_ = QDateTime::currentMSecsSinceEpoch();
    }

    void MessagesScrollArea::stopScrollAnimation()
    {
        LastAnimationMoment_ = MOMENT_UNINITIALIZED;
        ScrollDistance_ = 0;

        scheduleWheelBufferReset();

        if (!isScrolling())
        {
            return;
        }

        ScrollAnimationTimer_.stop();
    }

    void MessagesScrollArea::resetUserActivityTimer()
    {
        UserActivityTimer_.start();
    }

    void MessagesScrollArea::onIdleUserActivityTimeout()
    {
        UserActivityTimer_.stop();

        auto keysToUnload = getKeysToUnload();
        if (keysToUnload.empty())
        {
            return;
        }

        emit needCleanup(keysToUnload);
    }

    void MessagesScrollArea::onWheelEventResetTimer()
    {
        WheelEventsBuffer_.clear();

        WheelEventsBufferResetTimer_.stop();
    }

    bool MessagesScrollArea::isScrollAtBottom() const
    {
        return Layout_->isViewportAtBottom();
    }
}

namespace
{
    double getMaximumScrollDistance()
    {
        return Utils::scale_value(3000);
    }

    double getMinimumSpeed()
    {
        return Utils::scale_value(100);
    }

    double getMaximumSpeed()
    {
        return Utils::scale_value(3000);
    }

    double cubicOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed)
    {
        assert(maxScrollDistance > 0);
        assert(std::abs(scrollDistance) <= maxScrollDistance);
        assert(minSpeed >= 0);
        assert(maxSpeed > 0);
        assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#cub2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        t /= d;

        t--;

        return (c * (std::pow(t, 3.0) + 1) + b);
    }

    double expoOut(const double scrollDistance, const double maxScrollDistance, const double minSpeed, const double maxSpeed)
    {
        assert(maxScrollDistance > 0);
        assert(std::abs(scrollDistance) <= maxScrollDistance);
        assert(minSpeed >= 0);
        assert(maxSpeed > 0);
        assert(maxSpeed > minSpeed);

        // see http://gizma.com/easing/#expo2

        auto t = scrollDistance;
        const auto b = minSpeed;
        const auto c = (maxSpeed - minSpeed);
        const auto d = maxScrollDistance;

        return (c * (-std::pow(2.0, -10 * t/d) + 1) + b);
    }
}