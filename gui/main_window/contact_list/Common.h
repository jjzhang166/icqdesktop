#pragma once

#include "../../namespaces.h"
#include "../../controls/CommonStyle.h"
#include "../../fonts.h"
#include "../../utils/utils.h"

FONTS_NS_BEGIN

enum class FontFamily;
enum class FontWeight;

FONTS_NS_END

namespace ContactList
{
	typedef std::unique_ptr<QTextBrowser> QTextBrowserUptr;

	class DipPixels
	{
	public:
		explicit DipPixels(const int _px) : Px_(_px) {}

		int px() const;

		const DipPixels operator+(const DipPixels& _rhs) const { return DipPixels(Px_ + _rhs.Px_); }

		const DipPixels operator-(const DipPixels& _rhs) const { return DipPixels(Px_ - _rhs.Px_); }

		const DipPixels operator*(const DipPixels& _rhs) const { return DipPixels(Px_ * _rhs.Px_); }

		const DipPixels operator/(const DipPixels& _rhs) const { return DipPixels(Px_ / _rhs.Px_); }

        bool operator < (const DipPixels& _other) const
        {
            return Px_ < _other.Px_;
        }

	private:
		int Px_;
	};

	class DipFont
	{
	public:
        DipFont(const Fonts::FontFamily _family, const Fonts::FontWeight _weight, const DipPixels _size);

		QFont font() const;

	private:
		DipPixels Size_;

        Fonts::FontFamily Family_;

        Fonts::FontWeight Weight_;

	};

	struct VisualDataBase
	{
		VisualDataBase(
			const QString& _aimId,
			const QPixmap& _avatar,
			const QString& _state,
			const QString& _status,
			const bool _isHovered,
			const bool _isSelected,
			const QString& _contactName,
			const bool _haveLastSeen,
			const QDateTime& _lastSeen,
			const bool _isWithCheckBox,
            bool _isChatMember,
            bool _official,
            const bool _drawLastRead,
            const QPixmap& _lastReadAvatar,
            const QString& role,
            int _UnreadsCounter,
            const QString _term);

		const QString AimId_;

		const QPixmap &Avatar_;

		const QString State_;

		const QString &ContactName_;

		const bool IsHovered_;

		const bool IsSelected_;

		const bool HaveLastSeen_;

		const QDateTime LastSeen_;

		bool IsOnline() const { return HaveLastSeen_ && !LastSeen_.isValid(); }

		bool HasLastSeen() const { return HaveLastSeen_; }

		const bool isCheckedBox_;

        bool isChatMember_;

        bool isOfficial_;

        const bool drawLastRead_;

        const QPixmap& lastReadAvatar_;

        const QString& GetStatus() const;

        bool HasStatus() const;

        void SetStatus(const QString& _status);

        QString role_;

        const int unreadsCounter_;

        const QString searchTerm_;

    private:
        QString Status_;
	};

	const auto dip = [](const int _px) { return DipPixels(_px); };

	const auto dif = [](const Fonts::FontFamily _family, const Fonts::FontWeight _weight, const int _sizePx) { return DipFont(_family, _weight, dip(_sizePx)); };

	QString FormatTime(const QDateTime &time);

	QTextBrowserUptr CreateTextBrowser(const QString& _name, const QString& _stylesheet, const int _textHeight);

	DipPixels ItemWidth(bool fromAlert, bool _isWithCheckBox, bool _isShortView, bool _isPictureOnlyView = false);

    struct ViewParams;
    DipPixels ItemWidth(const ViewParams& _viewParams);

    int CorrectItemWidth(int _itemWidth, int _fixedWidth);

	DipPixels ItemLength(bool _isWidth, double _koeff, DipPixels _addWidth);

    int ContactItemHeight();
    int GroupItemHeight();
    int SearchInAllChatsHeight();

    bool IsPictureOnlyView();

    struct ViewParams
    {
        ViewParams(int _regim, bool _shortView, int _fixedWidth, int _leftMargin, int _rightMargin)
            : regim_(_regim)
            , shortView_(_shortView)
            , fixedWidth_(_fixedWidth)
            , leftMargin_(_leftMargin)
            , rightMargin_(_rightMargin)
            , pictOnly_(false)
        {}

        ViewParams()
            : regim_(-1)
            , shortView_(false)
            , fixedWidth_(-1)
            , leftMargin_(0)
            , rightMargin_(0)
            , pictOnly_(false)
        {}

        int regim_;
        bool shortView_;
        int fixedWidth_;
        int leftMargin_;
        int rightMargin_;
        bool pictOnly_;
    };

    class ContactListParams
    {
    public:
        const DipPixels itemHeight() const { return is_cl_ ? dip(44) : dip(68); }
        const DipPixels itemPadding() const { return dip(16); }
        const DipPixels checkboxWidth() const { return dip(20); }
        const DipPixels remove_size() const { return dip(20); }
        const DipPixels approve_size() const { return dip(32); }
        const DipPixels role_offset() const { return  dip(24); }
        const DipPixels role_ver_offset() const { return dip(4); }

        const DipPixels itemWidth() const { return dip(333); }
        const DipPixels itemWidthAlert() const { return dip(320); }

        const DipPixels itemLeftBorder() const { return itemPadding(); }

        DipPixels avatarX() const
        {
            if (!is_cl_)
                return dip(16);
            return withCheckBox_ ? checkboxWidth() + dip(24) + dip(10) : itemLeftBorder();
        }

        const DipPixels avatarY() const
        {
            if (!is_cl_)
                return dip(6);
            return dip(6);
        }
        const DipPixels avatarW() const
        {
            if (!is_cl_)
                return dip(56);
            return dip(32);
        }
        const DipPixels avatarH() const
        {
            if (!is_cl_)
                return dip(56);
            return dip(32);
        }

        const DipPixels timeFontSize() const { return dip(12); }
        const DipPixels timeY() const { return !is_cl_ ? avatarY() + dip(24) : dip(27); }
        const DipFont timeFont() const
        {
            return dif(Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), 12);
        }
        const QColor timeFontColor() const { return QColor(0x69, 0x69, 0x69); }

        const DipPixels onlineSignLeftPadding() const { return dip(12); }
        const DipPixels onlineSignSize() const { return  dip(4); }
        const DipPixels onlineSignY() const { return  dip(18); }
        const QColor onlineSignColor() const { return QColor("#579e1c"); }

        DipPixels GetContactNameX() const
        {
            if (!is_cl_)
                return  avatarX() + avatarW() + dip(13);
            return (avatarX() + avatarW() + dip(12) + DipPixels(Utils::unscale_value(leftMargin_)));
        }

        const DipPixels contactNameRightPadding (){ return  dip(12);}
        const DipPixels contactNameFontSize (){ return is_cl_ ? dip(16) : dip(17);}
        const DipPixels contactNameHeight (){ return  dip(24);}
        const DipPixels contactNameCenterY (){ return  dip(10);}
        const DipPixels contactNameTopY (){ return  dip(2);}
        const DipPixels nameY (){ return  dip(12);}

        QString getContactNameStylesheet(const QString& fontColor, const Fonts::FontWeight fontWeight)
        {
            assert(fontWeight > Fonts::FontWeight::Min);
            assert(fontWeight < Fonts::FontWeight::Max);

            const auto fontQss = Fonts::appFontFullQss(contactNameFontSize().px(), Fonts::defaultAppFontFamily(), fontWeight);

            const auto result =
                QString("%1; color: %2; background-color: transparent;")
                .arg(fontQss)
                .arg(fontColor);

            return result;
        };

        DipPixels GetStatusX()
        {
            return GetContactNameX();
        }

        const DipPixels statusY (){ return  dip(21);}
        const DipPixels statusFontSize (){ return  dip(13);}
        const DipPixels statusHeight (){ return  dip(20);}
        const QString getStatusStylesheet ()
        {
            return QString("font: %1 %2px \"%3\"; color: #696969; background-color: transparent")
                .arg(Fonts::defaultAppFontQssWeight())
                .arg(statusFontSize().px())
                .arg(Fonts::defaultAppFontQssName());
        };

        DipPixels GetAddContactX()
        {
            return GetContactNameX();
        }

        const DipPixels addContactY (){ return  dip(30);}
        const DipFont addContactFont (){ return  dif(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Semibold, 16);}
        const DipFont emptyIgnoreListFont (){ return dif(Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), 16);}
        const DipFont findInAllChatsFont (){ return dif(Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), 16);}
        const QColor emptyIgnoreListColor(){ return QColor("#000000");}

        DipPixels GetGroupX()
        {
            return GetAddContactX();
        }
        const DipPixels groupY (){ return dip(17);}
        const DipFont groupFont (){ return dif(Fonts::defaultAppFontFamily(), Fonts::defaultAppFontWeight(), 12);}
        const QColor groupColor(){ return QColor("#579e1c");}

        const DipPixels dragOverlayPadding (){ return dip(8);}
        const DipPixels dragOverlayBorderWidth (){ return dip(2);}
        const DipPixels dragOverlayBorderRadius (){ return dip(8);}
        const DipPixels dragOverlayVerPadding (){ return  dip(1);}

        const DipPixels official_hor_padding (){ return  dip(6);}
        const DipPixels official_ver_padding (){ return  dip(4);}

        const int getItemMiddleY()
        {
            return avatarH().px() / 2.0 + avatarY().px();
        };

        ContactListParams(bool _is_cl)
            : is_cl_(_is_cl)
            , withCheckBox_(false)
            , leftMargin_(0)
        {}

        bool getWithCheckBox() const
        {
            return withCheckBox_;
        }

        void setWithCheckBox(bool _withCheckBox)
        {
            withCheckBox_ = _withCheckBox;
        }

        void setLeftMargin(int _leftMargin)
        {
            leftMargin_ = _leftMargin;
        }

        int getLeftMargin() const
        {
            return leftMargin_;
        }

        bool isCL() const
        {
            return is_cl_;
        }

        void setIsCL(bool _isCl)
        {
            is_cl_ = _isCl;
        }

        void resetParams()
        {
            leftMargin_ = 0;
            withCheckBox_ = false;
        }

        const QString getNameFontColor(const bool isUnread)
        {
            const auto color =
                isUnread ?
                Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor()) :
                    Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor());

            return color;
        };

        const QString getMessageFontColor(const bool isUnread)
        {
            const auto color = (isUnread ? Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor()) : "#696969");

            return color;
        };

        const QString getFontWeight(const bool isUnread)
        {
            const auto fontWeight = (isUnread ? Fonts::FontWeight::Semibold : Fonts::FontWeight::Normal);

            const auto fontWeightQss = Fonts::appFontWeightQss(fontWeight);

            return fontWeightQss;
        };

        const DipPixels serviceItemHeight (){ return  dip(25);}
        const DipPixels unknownsItemHeight (){ return  dip(44);}
		const DipPixels itemHorPadding (){ return  dip(16);}
		const DipPixels itemHorPaddingUnknown (){ return  dip(9);}
        const DipPixels favoritesStatusPadding (){ return  dip(4);}

        const QString getRecentsNameFontColor(const bool isUnread);

        const QString getRecentsMessageFontColor(const bool isUnread);

        const QString getRecentsFontWeight(const bool isUnread);


		const DipPixels nameHeight (){ return  dip(24);}
        const DipPixels messageFontSize (){ return  dip(15);}
		const DipPixels messageHeight (){ return  dip(24);}
		const DipPixels messageX (){ return  GetContactNameX();}
		const DipPixels messageY (){ return  dip(34);}

		const QString getMessageStylesheet(const bool isUnread)
        {
            const auto fontWeight = Fonts::FontWeight::Normal;

            const auto fontQss = Fonts::appFontFullQss(messageFontSize().px(), Fonts::defaultAppFontFamily(), fontWeight);

            const auto fontColor = getRecentsMessageFontColor(isUnread);

            const auto result =
                QString("%1; color: %2; background-color: transparent")
                    .arg(fontQss)
                    .arg(fontColor);

            return result;
        };

		const DipPixels deliveryStateY (){ return  avatarY() + dip(11);}
		const DipPixels deliveryStateRightPadding (){ return  dip(4);}


		const DipFont unreadsFont (){ return  dif(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Semibold, 13);}
		const DipPixels unreadsPadding (){ return  dip(6);}
		const DipPixels unreadsMinimumExtent (){ return  dip(20);}
		const DipPixels unreadsY (){ return is_cl_ ? dip(12) : dip(24);}
		const DipPixels unreadsLeftPadding (){ return  dip(12);}

        const DipPixels lastReadY (){ return  dip(38);}

        const DipFont unknownsUnreadsFont (){ return  dif(Fonts::defaultAppFontFamily(), Fonts::FontWeight::Semibold, 13);}
        const DipPixels unknownsUnreadsPadding (){ return  dip(6);}
        const DipPixels unknownsUnreadsMinimumExtent (){ return  dip(20);}
        const DipPixels unknownsUnreadsY (){ return  dip(9);}
        const DipPixels unknownsUnreadsLeftPadding (){ return  dip(12);}

        int getLastReadAvatarSize() const { return Utils::scale_value(16); }

        const DipPixels interPadding() const { return  dip(6);}
        const DipPixels interLeftPadding() const { return  dip(12);}


        const DipPixels addContactSize() const { return  dip(30); }
        QRect& addContactFrame()
        {
            static QRect addContactFrameRect(0, 0, 0, 0);
            return addContactFrameRect;
        }

        const DipPixels removeContactSize() const { return dip(20); }
        QRect& removeContactFrame()
        {
            static QRect removeContactFrameRect(0, 0, 0, 0);
            return removeContactFrameRect;
        }

        QRect& deleteAllFrame()
        {
            static QRect deleteAllFrameRect(0, 0, 0, 0);
            return deleteAllFrameRect;
        }

    private:
        bool is_cl_;
        bool withCheckBox_;
        int leftMargin_;
    };

    ContactListParams& GetContactListParams();

    ContactListParams& GetRecentsParams(int _regim);

    void RenderAvatar(QPainter &painter, int _x, const QPixmap& _avatar, ContactList::ContactListParams& _contactListPx);

    void RenderMouseState(QPainter &painter, const bool isHovered, const bool isSelected, const ContactList::ContactListParams& _contactListPx, const ContactList::ViewParams& viewParams_);

    void RenderContactName(QPainter &painter, const ContactList::VisualDataBase &visData, const int y, const int rightBorderPx, ContactList::ViewParams _viewParams, ContactList::ContactListParams& _contactListPx);

    int RenderDate(QPainter &painter, const QDateTime &date, const VisualDataBase &item, ContactListParams& _contactListPx, const ViewParams& viewParams_);

    int RenderRemove(QPainter &painter, ContactListParams& _contactListPx, const ViewParams& viewParams_);

    int GetXOfRemoveImg(bool _isWithCheckBox, bool _shortView, int width);
}

namespace Logic
{
    class AbstractItemDelegateWithRegim : public QItemDelegate
    {
    public:
        AbstractItemDelegateWithRegim(QObject* parent)
            : QItemDelegate(parent)
        {}
        virtual void setRegim(int _regim) = 0;

        virtual void setFixedWidth(int width) = 0;
    };
}
