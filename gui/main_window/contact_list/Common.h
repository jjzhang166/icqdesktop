#pragma once
#include "../../utils/utils.h"


namespace ContactList
{

	typedef std::unique_ptr<QTextBrowser> QTextBrowserUptr;

	class DipPixels
	{
	public:
		explicit DipPixels(const int px) : Px_(px) {}

		int px() const;

		const DipPixels operator+(const DipPixels& rhs) const { return DipPixels(Px_ + rhs.Px_); }

		const DipPixels operator-(const DipPixels& rhs) const { return DipPixels(Px_ - rhs.Px_); }

		const DipPixels operator*(const DipPixels& rhs) const { return DipPixels(Px_ * rhs.Px_); }

		const DipPixels operator/(const DipPixels& rhs) const { return DipPixels(Px_ / rhs.Px_); }

	private:
		int Px_;
	};

	class DipFont
	{
	public:
        DipFont(const Utils::FontsFamily family, const DipPixels size);

		QFont font() const;

	private:
		DipPixels Size_;

        Utils::FontsFamily Family_;

	};

	struct VisualDataBase
	{
		VisualDataBase(
			const QString &aimId,
			const QPixmap &avatar,
			const QString &state,
			const QString &status,
			const bool isHovered,
			const bool isSelected,
			const QString &contactName,
			const bool haveLastSeen,
			const QDateTime &lastSeen,
			const bool isWithCheckBox,
            bool isChatMember);

		const QString AimId_;

		const QPixmap &Avatar_;

		const QString State_;

		const QString Status_;

		const QString &ContactName_;

		const bool IsHovered_;

		const bool IsSelected_;

		const bool HaveLastSeen_;

		const QDateTime LastSeen_;

		bool IsOnline() const { return HaveLastSeen_ && !LastSeen_.isValid(); }

		bool HasLastSeen() const { return HaveLastSeen_; }

		const bool isCheckedBox_;
        bool isChatMember_;
	};

	const auto dip = [](const int px) { return DipPixels(px); };

	const auto dif = [](const Utils::FontsFamily family, const int sizePx) { return DipFont(family, dip(sizePx)); };

	QString FormatTime(const QDateTime &time);

	QTextBrowserUptr CreateTextBrowser(const QString &name, const QString &stylesheet, const int textHeight);

	DipPixels ItemWidth(bool fromAlert, bool _isWithCheckBox, bool _isShortVIew);

	DipPixels ItemLength(bool _isWidth, double _koeff, DipPixels _addWidth);
    
    int ContactItemHeight();
}