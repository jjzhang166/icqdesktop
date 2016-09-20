#pragma once

namespace Ui
{

	typedef std::unique_ptr<class HistoryControlPageItem> HistoryControlPageItemUptr;

    namespace themes
    {
        typedef std::shared_ptr<class theme> themePtr;
    }

	class HistoryControlPageItem : public QWidget
	{
        Q_OBJECT

	public:
        HistoryControlPageItem(QWidget *parent);

        virtual void clearSelection();

        virtual QString formatRecentsText() const = 0;

        bool hasAvatar() const;

        bool hasTopMargin() const;

        virtual bool isSelected() const;

        virtual void onActivityChanged(const bool isActive);

        virtual void onVisibilityChanged(const bool isVisible);

        virtual void setHasAvatar(const bool value);

        virtual void select();

        virtual void setTopMargin(const bool value);

        virtual void setContact(const QString& _aimId);

        virtual void setSender(const QString& _sender);

        const QString& getAimid() const { return aimId_; }

        virtual themes::themePtr theme() const;

        virtual bool setLastRead(const bool _isLastRead);

        virtual qint64 getId() const;

        void setDeleted(const bool _isDeleted);

        bool isDeleted() const;

    protected:
        virtual void drawLastReadAvatar(QPainter& _p, const QString& _aimid, const QString& _friendly, const int _rightPadding, const int _bottomPadding);

        void setAimid(const QString &aimId);

    private:
        bool Selected_;

        bool HasTopMargin_;

        bool HasAvatar_;

        bool HasAvatarSet_;

        bool isDeleted_;

        QString aimId_;
	};

}