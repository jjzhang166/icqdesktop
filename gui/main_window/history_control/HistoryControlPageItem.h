#pragma once

namespace Ui
{

	typedef std::unique_ptr<class HistoryControlPageItem> HistoryControlPageItemUptr;

    namespace themes
    {
        class theme;
        typedef std::shared_ptr<theme> themePtr;
    }

	class HistoryControlPageItem : public QWidget
	{
        Q_OBJECT

    protected:

        virtual void drawLastReadAvatar(QPainter& _p, const QString& _aimid, const QString& _friendly, const int _rightPadding);

    // template methods
	public:
        virtual QString formatRecentsText() const = 0;

    public:
		HistoryControlPageItem(QWidget *parent);

        virtual void clearSelection();

        bool hasAvatar() const;

        bool hasTopMargin() const;

        bool isSelected() const;

        virtual void setHasAvatar(const bool value);

        virtual void select();

        virtual void setTopMargin(const bool value);

        virtual void setContact(const QString& _aimId);

        virtual void setSender(const QString& _sender);

        virtual QString getContact() const { return aimId_; }

        virtual themes::themePtr theme() const;

        virtual bool setLastRead(const bool _isLastRead);

        virtual qint64 getId() const;

    private:

        bool HasTopMargin_;

        bool HasAvatar_;

    protected:
        QString aimId_;

        bool Selected_;

	};

}