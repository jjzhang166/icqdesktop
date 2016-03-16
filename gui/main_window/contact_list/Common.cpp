#include "stdafx.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/InterConnector.h"
#include "../../main_window/MainWindow.h"

#include "Common.h"

namespace ContactList
{
	VisualDataBase::VisualDataBase(
		const QString &aimId,
		const QPixmap &avatar,
		const QString &state,
		const QString &status,
		const bool isHovered,
		const bool isSelected,
		const QString &contactName,
		const bool haveLastSeen,
		const QDateTime &lastSeen,
		bool isWithCheckBox,
        bool isChatMember)
		: AimId_(aimId)
		, Avatar_(avatar)
		, State_(state)
		, Status_(status)
		, IsHovered_(isHovered)
		, IsSelected_(isSelected)
		, ContactName_(contactName)
		, HaveLastSeen_(haveLastSeen)
		, LastSeen_(lastSeen)
		, isCheckedBox_(isWithCheckBox)
        , isChatMember_(isChatMember)
	{
		assert(!AimId_.isEmpty());
		assert(!ContactName_.isEmpty());
	}

	int DipPixels::px() const
	{
		return Utils::scale_value(Px_);
	}

    DipFont::DipFont(const Utils::FontsFamily family, const DipPixels size)
		: Family_(family)
		, Size_(size)
	{
//		assert(!Family_.isEmpty());
	}

	QFont DipFont::font() const
	{
        return Utils::appFont(Family_, Size_.px());
	}

	QString FormatTime(const QDateTime &time)
	{
		if (!time.isValid())
		{
			return QString();
		}

		const auto current = QDateTime::currentDateTime();

		const auto days = time.daysTo(current);

		if (days == 0)
		{
			const auto minutes = time.secsTo(current) / 60;
			if (minutes < 1)
			{
				return QT_TRANSLATE_NOOP("contact_list", "now");
			}

			if (minutes < 10)
			{
				return QVariant(minutes).toString() + QT_TRANSLATE_NOOP("contact_list", " min");
			}

			return time.time().toString(Qt::SystemLocaleShortDate);
		}

		if (days == 1)
		{
			return QT_TRANSLATE_NOOP("contact_list", "yesterday");
		}

		const auto date = time.date();
		return Utils::GetTranslator()->formatDate(date, date.year() == current.date().year());
	}

	QTextBrowserUptr CreateTextBrowser(const QString &name, const QString &stylesheet, const int textHeight)
	{
		assert(!name.isEmpty());
		assert(!stylesheet.isEmpty());

		QTextBrowserUptr ctrl(new QTextBrowser);

		ctrl->setObjectName(name);
		ctrl->setStyleSheet(stylesheet);
		ctrl->setFixedHeight(textHeight);

		ctrl->setFrameStyle(QFrame::NoFrame);
		ctrl->setContentsMargins(0, 0, 0, 0);

		ctrl->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		ctrl->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

		return ctrl;
	}
	
	DipPixels ItemWidth(bool fromAlert, bool _isWithCheckBox, bool _isShortVIew)
	{
		if (fromAlert)
			return DipPixels(320);

        if (_isShortVIew)
            return DipPixels(280);

		// TODO : use L::checkboxWidth
		return ItemLength(true, 1. / 3, _isWithCheckBox ? dip(30) : dip(0));
	}

	DipPixels ItemLength(bool _isWidth, double _koeff, DipPixels _addWidth)
	{
        assert(!!Utils::InterConnector::instance().getMainWindow() && "Common.cpp (ItemLength)");
        auto main_rect = Utils::GetMainRect();
        if (main_rect.width() && main_rect.height())
        {
            auto main_length = _isWidth ? main_rect.width() : main_rect.height();
            return _addWidth + DipPixels(Utils::unscale_value(main_length) * _koeff);
        }
        assert("Couldn't get rect: Common.cpp (ItemLength)");
        return DipPixels(0);
	}

    int ContactItemHeight()
    {
        return Utils::scale_value(44);
    }
}