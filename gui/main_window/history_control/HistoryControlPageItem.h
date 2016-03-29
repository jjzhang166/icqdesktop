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

        virtual QString getContact() const { return aimId_; }

        themes::themePtr theme() const;


    private:
        bool HasTopMargin_;

        bool HasAvatar_;

    protected:
        QString aimId_;

        bool Selected_;
	};

}