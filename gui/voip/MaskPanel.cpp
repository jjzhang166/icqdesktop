
#include "stdafx.h"
#include "MaskPanel.h"
#include "DetachedVideoWnd.h"
#include "../utils/utils.h"
#include "../core_dispatcher.h"
#include "MaskManager.h"



// Items view settings.
enum {
    NORMAL_ITEM_BORDER = 1, SELECTED_ITEM_BORDER = 5,
    NORMAL_ITEM_SIZE = 40 + NORMAL_ITEM_BORDER * 2,
    SELECTED_ITEM_SIZE = 64 + SELECTED_ITEM_BORDER * 2,
    PANEL_SIZE = 116, HIDE_PRIVIEW_PRIMARY_INTERVAL = 7000,
    SCROLLING_SPEED = 10, MASK_SPACING = 12, 
    MASKS_ANIMATION_MOVE_SPEED = 25,
    MASKS_ANIMATION_RESIZE_SPEED = 32, 
    MASKS_ANIMATION_RESIZE_DISTANCE = 32,
    MASKS_ANIMATION_DURATION = 300, // In ms.
};

Ui::MaskWidget::MaskWidget(const voip_masks::Mask* mask) : mask_(mask), loadingProgress_(0.0), maskEngineReady_(false), applyWhenEnebled_(false)
{
    setObjectName("MaskWidget");
    setCursor(Qt::PointingHandCursor);

    updatePreview();
    updateJsonPath();

    if (mask_)
    {
        connect(mask_, SIGNAL(previewLoaded()), this, SLOT(updatePreview()));
        connect(mask_, SIGNAL(loaded()), this, SLOT(updateJsonPath()));
        connect(mask_, SIGNAL(loadingProgress(unsigned)), this, SLOT(updateLoadingProgress(unsigned)));
    }

    auto margin = Utils::scale_value(1);
    setContentsMargins(margin, margin, margin, margin);
	setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE + margin, NORMAL_ITEM_SIZE + margin)));
}

void Ui::MaskWidget::paintEvent(QPaintEvent * /*event*/)
{
	QPainter painter(this);

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::NoBrush);

	{
		painter.save();
		painter.setPen(QPen(QColor(0, 0, 0, 128)));
		painter.setBrush(QBrush(QColor(255, 255, 255, 128)));

		// Draw outer border.
		painter.drawEllipse(contentsRect());
		painter.restore();
	}

	int border = Utils::scale_value(NORMAL_ITEM_BORDER);

	// Selected border.
	if (isChecked())
	{
		painter.save();

		QPen pen(QColor(255, 255, 255, 255));
		pen.setWidth(4);

		painter.setPen(pen);

		// inc half border width.
		border += Utils::scale_value(2);
		auto borderRect = contentsRect().marginsRemoved(QMargins(border, border, border, border));

		// Draw outer border.
		painter.drawEllipse(borderRect);
		painter.restore();

		border = Utils::scale_value(SELECTED_ITEM_BORDER);
	}

	// Draw image.
	if (!image_.isNull())
	{
		auto imageRect = contentsRect().marginsRemoved(QMargins(border, border, border, border));

		QPainterPath path;
		path.addEllipse(imageRect);
		painter.setClipPath(path);

        auto resizedImage = image_.scaled(imageRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		painter.drawPixmap(imageRect, resizedImage, resizedImage.rect());
	}

    // Draw disabled state
    if (!isEnabled() || !maskEngineReady_)
    {
        painter.save();
        painter.setPen(QPen(QColor(0, 0, 0, 0)));
        painter.setBrush(QBrush(QColor(0, 0, 0, 128)));
        
        // Draw outer border.
        int startAngel  = 90 * 16; // 90 degree.
        int spanAngel = (1.0 - loadingProgress_) * 360 * 16; // 90 degree - progress * 360 
        painter.drawPie(contentsRect(), startAngel, !maskEngineReady_ ? 360 * 16 : spanAngel);
        painter.restore();
    }
}

QString Ui::MaskWidget::maskPath()
{
	return maskPath_;
}

void Ui::MaskWidget::setPixmap(const QPixmap& image)
{
	image_ = image;
	update();
}

QPixmap Ui::MaskWidget::pixmap()
{
	return image_;
}

void Ui::MaskWidget::setChecked(bool check)
{
	QPushButton::setChecked(check);

	setFixedSize(Utils::scale_value(check ? QSize(SELECTED_ITEM_SIZE, SELECTED_ITEM_SIZE) :
		QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));

    if (check)
    {
        if (isEnabled())
        {
            Ui::GetDispatcher()->getVoipController().loadMask(maskPath().toStdString());
        }
        else
        {
            applyWhenEnebled_ = true;
        }
    }
    else
    {
        applyWhenEnebled_ = false;
    }
}

void Ui::MaskWidget::updatePreview()
{
    if (mask_)
    {
        image_ = mask_->preview();
        update();
    }
}

void Ui::MaskWidget::updateJsonPath()
{
    if (mask_)
    {
        maskPath_ = mask_->jsonPath();
        name_     = mask_->id();
        setEnabled(!maskPath_.isEmpty() && maskEngineReady_);
        updateLoadingProgress(!maskPath_.isEmpty() ? 100 : 0);
        update();
    }
}

void Ui::MaskWidget::updateLoadingProgress(unsigned progress)
{
    loadingProgress_ = progress / 100.0f;
    update();
}

void Ui::MaskWidget::setMaskEngineReady(bool ready)
{
    maskEngineReady_ = ready;
    updateJsonPath();
    update();
}

void Ui::MaskWidget::changeEvent(QEvent * event)
{
    if (event->type() == QEvent::EnabledChange && isEnabled() && applyWhenEnebled_)
    {
        Ui::GetDispatcher()->getVoipController().loadMask(maskPath().toStdString());
        applyWhenEnebled_ = false;
    }

    QPushButton::changeEvent(event);
}

bool Ui::MaskWidget::isEmptyMask()
{
    return mask_ == nullptr;
}

bool Ui::MaskWidget::isLoaded()
{
    return mask_ == nullptr || !maskPath_.isEmpty();
}

const QString& Ui::MaskWidget::name()
{
    return name_;
}

void Ui::MaskWidget::setXPos(int x)
{
    move(x, y());
}

void Ui::MaskWidget::setYCenter(int y)
{
    move(x(), y - height() / 2);
}

int  Ui::MaskWidget::yCenter()
{
    return y() + height() / 2;
}

void Ui::MaskWidget::updateSize()
{
    setFixedSize(Utils::scale_value(isChecked() ? QSize(SELECTED_ITEM_SIZE, SELECTED_ITEM_SIZE) :
        QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));
}


Ui::ScrollWidget::ScrollWidget()
{
}

void Ui::ScrollWidget::animationShow()
{
    runAnimation(false);
}

void Ui::ScrollWidget::animationHide()
{
    runAnimation(true);
}

template <typename T> void Ui::ScrollWidget::addAnimation(QObject* mask, const QByteArray& param, const T& start, const T& finish,
    float duration, const QEasingCurve& curve, bool out)
{
    auto animation = new QPropertyAnimation(mask, param);
    animation->setStartValue(start);
    animation->setEndValue(finish);
    animation->setDuration(duration);
    animation->setEasingCurve(curve);
    animation->setDirection(out ? QAbstractAnimation::Backward : QAbstractAnimation::Forward);
    animation->start(QAbstractAnimation::DeleteWhenStopped);    
}

void Ui::ScrollWidget::runAnimation(bool out)
{
    auto masks = findChildren<MaskWidget*>();

    QMap<QWidget*, QRect> maskPositions;
    updateGeometry();

    // Save correct position on panel.
    for (auto mask : masks)
    {
        maskPositions[mask] = mask->frameGeometry();
    }

    // Remove masks widgets from layout.
    for (auto mask : masks)
    {
        layout()->removeWidget(mask);

        // Move to start position.
        if (!out)
        {
            mask->move(mask->frameGeometry().center().x(), 0);
            mask->setFixedSize(0, 0);
        }
    }

    // Make size fixed, to stay scroll on the same plase during animation.
    setFixedSize(size());

    int index = 0;

    auto addAnimations = [] (MaskWidget* mask, const QRect& rect, double duration, bool out, ScrollWidget* self) {
        // Animation for opacity.
        auto opacityEffect = new QGraphicsOpacityEffect(mask);
        mask->setGraphicsEffect(opacityEffect);
        self->addAnimation(opacityEffect, "opacity", 0.0, 1.0,
            duration * 1.5, QEasingCurve::InOutSine, out);

        self->addAnimation(mask, "minimumSize", QSize(0, 0), rect.size(),
            duration, QEasingCurve::Linear, out);

        self->addAnimation(mask, "maximumSize", QSize(0, 0), rect.size(),
            duration, QEasingCurve::Linear, out);

        self->addAnimation(mask, "yCenter", 0, rect.center().y(),
            duration, QEasingCurve::InOutCubic, out);

        self->addAnimation(mask, "xPos", rect.center().x(), rect.left(),
            duration, QEasingCurve::Linear, out);
    };

    
    auto scrollParentRect = parentWidget() ? parentWidget()->geometry() : QRect();
    auto padding = Utils::scale_value(QSize(SELECTED_ITEM_SIZE, SELECTED_ITEM_SIZE));
    // TODO: Hack, because looks like using only geometry is not enough.
    scrollParentRect.setSize(scrollParentRect.size() + 2 * padding);

    auto positionOnParent = mapFromParent(QPoint(0, 0));
    scrollParentRect.moveTo(positionOnParent - QPoint(padding.width(), padding.height()));

    // Add animations for masks.
    if (!out)
    {
        for (auto mask : masks)
        {
            int duration = 0;
            auto rect = maskPositions[mask];

            bool isVisible = scrollParentRect.contains(rect);
            duration = isVisible ? (MASKS_ANIMATION_DURATION + index * MASKS_ANIMATION_DURATION / 5) : 0;

            addAnimations(mask, rect, duration, out, this);

            if (isVisible)
            {
            index++;
            }
        }
    }
    else
    {
        for (int i = masks.size() - 1; i >= 0; i--)
        {
            auto mask = masks[i];
            int duration = 0;
            auto rect = maskPositions[mask];

            bool isVisible = scrollParentRect.contains(rect);
            duration = isVisible ? MASKS_ANIMATION_DURATION : 0;

            auto startAnimationTime = isVisible ? index * MASKS_ANIMATION_DURATION / 5 : 0;
            // For out animation, we start from last to first item.
            QTimer::singleShot(startAnimationTime, this, [=]
            {
                addAnimations(mask, rect, duration, out, this);
            });

            if (isVisible)
            {
                index++;
            }
        }
    }

    auto endAnimation = (MASKS_ANIMATION_DURATION + index * MASKS_ANIMATION_DURATION / 5) + 100;
    
    QTimer::singleShot(endAnimation, this, [=]
    {
        QBoxLayout* boxLayout = qobject_cast<QBoxLayout*>(layout());
        if (boxLayout)
        {
            int index = 0;
            auto masks = findChildren<MaskWidget*>();

            for (auto mask : masks)
            {
                mask->updateSize();
                boxLayout->insertWidget((index == 0) ? index : index * 2, mask, 0, Qt::AlignLeft | Qt::AlignTop);
                index++;
            }
            updateGeometry();
            layout()->activate();
        }

        // Disable fixed size for scroll widget.
        setFixedSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        emit animationFinished(out);
    });
}


Ui::MaskPanel::MaskPanel(QWidget* _parent, QWidget* _container, int topOffset, int bottomOffset) :
#ifdef __APPLE__
    // We use Qt::WindowDoesNotAcceptFocus for mac, bcause it fix ghost window after call stop on Sierra
    BaseVideoPanel(_parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint)
#else
    BaseVideoPanel(_parent)
#endif
    , container_(_container)
    , rootWidget_(nullptr)
	, topOffset_(topOffset)
	, bottomOffset_(bottomOffset)
	, maskListWidget_(nullptr)
	, direction_(QBoxLayout::TopToBottom)
	, scrollWidget_(nullptr)
	, layoutMaskListWidget_(nullptr)
    , hasLocalVideo_(false)
    , hidePreviewPrimaryTimer_(new QTimer())
    , backgroundWidget_(nullptr)
{
	// Layout
	// rootLayout -> rootWidget_ -> layoutTarget -> maskListWidget_ -> layoutMaskListWidget -> maskList_ -> Masks
	//									|---------> currentMaskButton_          |-------->upButton_/downButton_
	//

    setStyleSheet(Utils::LoadStyle(":/voip/mask_panel.qss"));
    setObjectName("MaskPanel");
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* rootLayout = new QHBoxLayout();
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
	rootLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setLayout(rootLayout);

#ifndef __APPLE__
    backgroundWidget_ = new PanelBackground(this);
    backgroundWidget_->updateSizeFromParent();
#endif
    
    rootWidget_ = new QWidget(this);
    rootWidget_->setContentsMargins(0, 0, 0, 0);
    layout()->addWidget(rootWidget_);
    
    
    QVBoxLayout* layoutTarget = new QVBoxLayout();
    layoutTarget->setContentsMargins(Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16));
    layoutTarget->setSpacing(0);
    layoutTarget->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    rootWidget_->setLayout(layoutTarget);

	// This widget contains up/down buttons, mask list.
	maskListWidget_ = createMaskListWidget();

	layoutTarget->addWidget(maskListWidget_);

	currentMaskButton_ = new MaskWidget(nullptr);
    currentMaskButton_->setMaskEngineReady(true);

	layoutTarget->addWidget(currentMaskButton_);

	layoutTarget->setDirection(QBoxLayout::LeftToRight);

    hidePreviewPrimaryTimer_->setInterval(HIDE_PRIVIEW_PRIMARY_INTERVAL);
    hidePreviewPrimaryTimer_->setSingleShot(true);
    
	connect(currentMaskButton_, SIGNAL(clicked()), this, SLOT(showMaskList()));
	connect(upButton_, SIGNAL(clicked()), this, SLOT(scrollListUp()));
	connect(downButton_, SIGNAL(clicked()), this, SLOT(scrollListDown()));

    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMediaLocalVideo(bool)),
        this, SLOT(setVideoStatus(bool)), Qt::DirectConnection);

    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipCallCreated(const voip_manager::ContactEx&)), 
        this, SLOT(updateMaskList()), Qt::DirectConnection);

    connect(hidePreviewPrimaryTimer_, SIGNAL(timeout()), this, SLOT(autoHidePreviewPrimary()), Qt::DirectConnection);

    connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMinimalBandwidthChanged(bool)), this, SLOT(onVoipMinimalBandwidthChanged(bool)), Qt::DirectConnection);
   

    updateMaskList();

    hideMaskListWithOutAnimation();
}

Ui::MaskPanel::~MaskPanel()
{
}


void Ui::MaskPanel::changeEvent(QEvent* _e)
{
    QWidget::changeEvent(_e);

    if (_e->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow() || (rootWidget_ && rootWidget_->isActiveWindow()))
        {
            if (container_)
            {
                container_->raise();
                raise();
            }
        }
    }
}

void Ui::MaskPanel::enterEvent(QEvent* _e)
{
    QWidget::enterEvent(_e);
    hidePreviewPrimaryTimer_->stop();
    emit onMouseEnter();
}

void Ui::MaskPanel::leaveEvent(QEvent* _e)
{
    QWidget::leaveEvent(_e);
    hidePreviewPrimaryTimer_->start();
    emit onMouseLeave();
}

void Ui::MaskPanel::keyReleaseEvent(QKeyEvent* _e)
{
    QWidget::keyReleaseEvent(_e);
    if (_e->key() == Qt::Key_Escape)
    {
        emit onkeyEscPressed();
    }
}

void Ui::MaskPanel::updatePosition(const QWidget& parent)
{
#ifdef __APPLE__
    QRect parentRect = platform_macos::getWidgetRect(*parentWidget());
#endif

    auto rc = parentWidget()->geometry();
	
	if (direction_ == QBoxLayout::LeftToRight)
	{
#ifdef _WIN32
        move(rc.x(), rc.y() + topOffset_);
#endif
        setFixedHeight(Utils::scale_value(PANEL_SIZE));
        setFixedWidth(rc.width());
        
#ifdef __APPLE__
        platform_macos::setWindowPosition(*this,
                                          QRect(parentRect.left(),
                                                parentRect.top() + parent.height() - Utils::scale_value(PANEL_SIZE) - topOffset_,
                                                parentRect.width(),
                                                Utils::scale_value(PANEL_SIZE)));
#endif

        rootWidget_->layout()->setContentsMargins(Utils::scale_value(12), Utils::scale_value(16), Utils::scale_value(12), Utils::scale_value(16));
	}
	else if (direction_ == QBoxLayout::TopToBottom)
	{
#ifdef _WIN32
        move(rc.x(), rc.y() + topOffset_);
#endif
        setFixedHeight(rc.height() - topOffset_ - bottomOffset_);
        setFixedWidth(Utils::scale_value(PANEL_SIZE));
        
#ifdef __APPLE__
        platform_macos::setWindowPosition(*this,
                                          QRect(parentRect.left(),
                                                parentRect.top() + bottomOffset_,
                                                Utils::scale_value(PANEL_SIZE),
                                                parentRect.height() - topOffset_ - bottomOffset_));
#endif
        rootWidget_->layout()->setContentsMargins(Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16), Utils::scale_value(16));
	}
}

void Ui::MaskPanel::setMaskList(const voip_masks::MaskList& maskList)
{
    hideMaskListWithOutAnimation();

    clearMaskList();

	auto maskWidget = appendItem(nullptr);

	for (auto mask : maskList)
	{
		maskLayout_->addSpacing(Utils::scale_value(MASK_SPACING));
		appendItem(mask);
	}

    updateUpDownButtons();

    selectMask(maskWidget);
}

void Ui::MaskPanel::showMaskList()
{
    sendOpenStatistic();

	maskListWidget_->show();
    scrollWidget_->animationShow();
	currentMaskButton_->hide();

    Ui::GetDispatcher()->getVoipController().initMaskEngine();

    auto selectedMask = getSelectedWidget();
    if (selectedMask && !selectedMask->isEmptyMask())
    {
        emit makePreviewPrimary();
    }

    emit onShowMaskList();

    scrollWidget_->setFocus();
}

void Ui::MaskPanel::hideMaskListWithOutAnimation()
{
    hidePreviewPrimaryTimer_->stop();
    maskListWidget_->hide();
    currentMaskButton_->show();
    makeInterlocutorPrimary();

    emit onHideMaskList();
}

void Ui::MaskPanel::hideMaskList()
{
    hidePreviewPrimaryTimer_->stop();
    scrollWidget_->animationHide();
}

void Ui::MaskPanel::animationFinished(bool out)
{
    if (out)
    {
        hideMaskListWithOutAnimation();
    }
    else
    {
        hidePreviewPrimaryTimer_->start();
    }
}

Ui::MaskWidget* Ui::MaskPanel::appendItem(const voip_masks::Mask* mask)
{
	MaskWidget* icon = new MaskWidget(mask);

	icon->setCheckable(true);
	icon->setFlat(true);
	icon->setChecked(false);
	icon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Disable change layout if widget was hide.
    QSizePolicy sp_retain = icon->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    icon->setSizePolicy(sp_retain);

    if (mask)
    {
        icon->setMaskEngineReady(Ui::GetDispatcher()->getVoipController().isMaskEngineEnabled());

        connect(&Ui::GetDispatcher()->getVoipController(), SIGNAL(onVoipMaskEngineEnable(bool)),
            icon, SLOT(setMaskEngineReady(bool)), Qt::DirectConnection);
    }
    else
    {
        icon->setMaskEngineReady(true);
    }

	connect(icon, SIGNAL(clicked(bool)), this, SLOT(changedMask()));

	maskLayout_->addWidget(icon, 0, Qt::AlignLeft | Qt::AlignTop);

	return icon;
}

void Ui::MaskPanel::changedMask()
{
	MaskWidget* mask = qobject_cast<MaskWidget*>(sender());

	if (mask)
	{
        sendSelectMaskStatistic(mask);
		selectMask(mask);
        hidePreviewPrimaryTimer_->start();
	}
}

void Ui::MaskPanel::selectMask(MaskWidget* mask)
{
	// Reset buttons state.
	auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
	for (auto button : maskButtons)
	{
		button->setChecked(false);
	}

	if (mask)
	{
		QString maskPath = mask->maskPath();

        if (!hasLocalVideo_ && !mask->isEmptyMask())
        {
            Ui::GetDispatcher()->getVoipController().setSwitchVCaptureMute();
            hasLocalVideo_ = true;
        }

        mask->setChecked(true);

        QMetaObject::invokeMethod(this, "scrollToWidget", Qt::QueuedConnection,
            Q_ARG(QString, mask->name()));

        if (!mask->isEmptyMask())
        {
            currentMaskButton_->setPixmap(mask->pixmap());
            emit makePreviewPrimary();
        }
        else
        {
            // Set first mask pixmap for current mask button.
            auto mask = getFirstMask();
            auto pixmap = mask ? mask->pixmap() : QPixmap();
            currentMaskButton_->setPixmap(pixmap);
        }
	}
}

QWidget* Ui::MaskPanel::createMaskListWidget()
{
	layoutMaskListWidget_ = new QVBoxLayout();
	layoutMaskListWidget_->setContentsMargins(0, 0, 0, 0);
	layoutMaskListWidget_->setSpacing(0);
	layoutMaskListWidget_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	maskLayout_ = new QVBoxLayout();
	maskLayout_->setContentsMargins(0, 0, 0, 0);
	maskLayout_->setSpacing(0);
	maskLayout_->setAlignment(Qt::AlignTop | Qt::AlignLeft);

	scrollWidget_ = new ScrollWidget();
	scrollWidget_->setObjectName("MaskList");

	// Scroll area with all masks.
	maskList_ = new QScrollArea();
	maskList_->setObjectName("MaskList");
	maskList_->setProperty("orintation", "vertical");
	maskList_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	maskList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	maskList_->setWidgetResizable(true);
	maskList_->setWidget(scrollWidget_);

	scrollWidget_->setLayout(maskLayout_);

	// Scroll up/down buttons.
	upButton_ = new QPushButton();
    upButton_->setProperty("MaskArrow", "up");
	upButton_->setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));
    upButton_->setCursor(Qt::PointingHandCursor);

	downButton_ = new QPushButton();
    downButton_->setProperty("MaskArrow", "down");
	downButton_->setFixedSize(Utils::scale_value(QSize(NORMAL_ITEM_SIZE, NORMAL_ITEM_SIZE)));
    downButton_->setCursor(Qt::PointingHandCursor);

	QWidget* res = new QWidget();
	res->setLayout(layoutMaskListWidget_);

    // Disable change layout if widget was hide.
    QSizePolicy retainUp = upButton_->sizePolicy();
    retainUp.setRetainSizeWhenHidden(true);
    upButton_->setSizePolicy(retainUp);

    // Disable change layout if widget was hide.
    QSizePolicy retainDown = downButton_->sizePolicy();
    retainDown.setRetainSizeWhenHidden(true);
    downButton_->setSizePolicy(retainDown);

	layoutMaskListWidget_->addWidget(upButton_, 0, Qt::AlignLeft | Qt::AlignTop);
	layoutMaskListWidget_->addSpacing(Utils::scale_value(MASK_SPACING));
	layoutMaskListWidget_->addWidget(maskList_);
	layoutMaskListWidget_->addSpacing(Utils::scale_value(MASK_SPACING));
	layoutMaskListWidget_->addWidget(downButton_, 0, Qt::AlignLeft | Qt::AlignTop);

    maskList_->horizontalScrollBar()->setSingleStep(Utils::scale_value(SCROLLING_SPEED));
    maskList_->verticalScrollBar()->setSingleStep(Utils::scale_value(SCROLLING_SPEED));

	connect(maskList_->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateUpDownButtons()));
	connect(maskList_->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(updateUpDownButtons()));
	connect(maskList_->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateUpDownButtons()));
	connect(maskList_->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(updateUpDownButtons()));

    connect(scrollWidget_, SIGNAL(animationFinished(bool)), this, SLOT(animationFinished(bool)), Qt::DirectConnection);

	return res;
}

void Ui::MaskPanel::scrollListUp()
{
 	maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
	maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
    hidePreviewPrimaryTimer_->start();
}

void Ui::MaskPanel::scrollListDown()
{
	maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
	maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
    hidePreviewPrimaryTimer_->start();
}

void Ui::MaskPanel::updateUpDownButtons()
{
	auto scroll = (direction_ == QBoxLayout::LeftToRight ? maskList_->horizontalScrollBar() : maskList_->verticalScrollBar());
	upButton_->setVisible(scroll->value()   != scroll->minimum());
	downButton_->setVisible(scroll->value() != scroll->maximum());
}

void Ui::MaskPanel::setPanelMode(QBoxLayout::Direction direction)
{
	if (direction_ != direction)
	{
		if (direction == QBoxLayout::LeftToRight || direction == QBoxLayout::TopToBottom)
		{
			layoutMaskListWidget_->setDirection(direction);
			maskLayout_->setDirection(direction);
			if (parentWidget())
			{
				updatePosition(*parentWidget());
			}
			if (direction == QBoxLayout::LeftToRight)
			{
				maskList_->setProperty("orintation", "horizontal");
				maskList_->verticalScrollBar()->setValue(maskList_->horizontalScrollBar()->value());

                upButton_->setProperty("MaskArrow", "left");
                downButton_->setProperty("MaskArrow", "right");
			}
			else
			{
				maskList_->setProperty("orintation", "vertical");
				maskList_->horizontalScrollBar()->setValue(maskList_->verticalScrollBar()->value());

                upButton_->setProperty("MaskArrow", "up");
                downButton_->setProperty("MaskArrow", "down");
			}

            auto updateCssStyle = [](QWidget* widget)
            {
                widget->style()->unpolish(widget);
                widget->style()->polish(widget);
                widget->update();
            };

            updateCssStyle(maskList_);
            updateCssStyle(upButton_);
            updateCssStyle(downButton_);
			
			direction_ = direction;
		}
	}
}

void Ui::MaskPanel::setVideoStatus(bool bEnabled)
{
    hasLocalVideo_ = bEnabled;
}

void Ui::MaskPanel::updateMaskList()
{
    auto maskList = Ui::GetDispatcher()->getVoipController().getMaskManager()->getAvailableMasks();
    setMaskList(maskList);
}

void Ui::MaskPanel::setTopOffset(int topOffset)
{
    topOffset_ = topOffset;
    updatePosition(*parentWidget());
}

void Ui::MaskPanel::chooseFirstMask()
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    if (maskButtons.size() > 1 && maskButtons[0]->isChecked())
    {
        selectMask(maskButtons[1]);
        sendSelectMaskStatistic(maskButtons[1]);
    }
}

void Ui::MaskPanel::autoHidePreviewPrimary()
{
    hideMaskList();
}

Ui::MaskWidget* Ui::MaskPanel::getSelectedWidget()
{
    MaskWidget* selectedMask = nullptr;
    enumerateMask([&](MaskWidget* mask)
    {
        if (mask->isChecked())
        {
            selectedMask = mask;
        }
    });

    return selectedMask;
}

Ui::MaskWidget* Ui::MaskPanel::getFirstMask()
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
    if (maskButtons.size() > 1)
    {
        return maskButtons[1];
    }

    return nullptr;
}


void Ui::MaskPanel::callDestroyed()
{
    clearMaskList();
}

void Ui::MaskPanel::clearMaskList()
{
    // Remove all current masks.
    QLayoutItem* item = NULL;
    while ((item = maskLayout_->takeAt(0)) != NULL)
    {
        delete item->widget();
        delete item;
    }
}

void Ui::MaskPanel::resizeEvent(QResizeEvent * event)
{
    if (backgroundWidget_)
    {
        backgroundWidget_->updateSizeFromParent();
    }
}

void Ui::MaskPanel::onVoipMinimalBandwidthChanged(bool _bEnable)
{
    if (_bEnable)
    {
        // Disable masks in ecomon mode.
        auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();
        selectMask(maskButtons[0]);
        hideMaskList();
    }
    currentMaskButton_->setDisabled(_bEnable);
}

void Ui::MaskPanel::wheelEvent(QWheelEvent * event)
{
    if (scrollWidget_->isVisible())
    {
        // Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
        // But we made the same scroll as default.
        int mouseWheelValue = event->delta() / 30;

        while (mouseWheelValue != 0)
        {
            if (mouseWheelValue < 0)
            {
                maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
                maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            }
            else
            {
                maskList_->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
                maskList_->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
            }

            (mouseWheelValue > 0 ? mouseWheelValue-- : mouseWheelValue++);
        }

        hidePreviewPrimaryTimer_->start();
    }
}

void Ui::MaskPanel::sendOpenStatistic()
{
    core::stats::event_props_type props;

    bool bLoaded = true;

    enumerateMask([&bLoaded](MaskWidget* mask)
        {
            bLoaded = bLoaded && mask->isLoaded();
        });

    props.emplace_back("Masks_Ready", bLoaded ? "yes" : "no");
    props.emplace_back("Model_Ready", Ui::GetDispatcher()->getVoipController().isMaskEngineEnabled() ? "yes" : "no");
    
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::masks_open, props);
}

void Ui::MaskPanel::enumerateMask(std::function<void(MaskWidget* mask)> func)
{
    auto maskButtons = maskList_->widget()->findChildren<MaskWidget*>();

    std::for_each(maskButtons.begin(), maskButtons.end(), func);
}

void Ui::MaskPanel::sendSelectMaskStatistic(MaskWidget* mask)
{
    core::stats::event_props_type props;

    if (mask && !mask->name().isEmpty())
    {
        bool isAccepted = false;
        getCallStatus(isAccepted);

        props.emplace_back("name", mask->name().toUtf8().data());
        props.emplace_back("when", isAccepted ? "call_started" : "call_outgoing");

        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::masks_select, props);
    }
}

void Ui::MaskPanel::scrollToWidget(QString maskName)
{
    MaskWidget* selectedMask = nullptr;
    enumerateMask([&](MaskWidget* mask)
    {
        if (maskName == mask->name())
        {
            selectedMask = mask;
        }
    });

    if (selectedMask)
    {
        maskList_->ensureWidgetVisible(selectedMask, 0, 0);
    }
}

bool Ui::MaskPanel::isOpened()
{
    return maskListWidget_->isVisible();
}
