#pragma once

#include "CustomAbstractListModel.h"

#include "../../types/contact.h"
#include "../../types/message.h"

namespace Ui
{
    class MainWindow;
}

namespace Logic
{
	class RecentsModel : public CustomAbstractListModel
	{
		Q_OBJECT

	Q_SIGNALS:
		void orderChanged();
		void updated();
        void readStateChanged(QString);
        void selectContact(QString);
        void dlgStateHandled(Data::DlgState);
        void favoriteChanged(QString);

	private Q_SLOTS:
		void activeDialogHide(QString);
		void contactChanged(QString);
		void dlgState(Data::DlgState);
		void sortDialogs();

	public:
		explicit RecentsModel(QObject *parent);

		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		QVariant data(const QModelIndex &index, int role) const;
		Qt::ItemFlags flags(const QModelIndex &index) const;

		Data::DlgState getDlgState(const QString& aimId = QString(), bool fronDialog = false);

        void toggleFavoritesVisible();

		void sendLastRead(const QString& aimId = QString());
		void markAllRead();
		void hideChat(const QString& aimId);
		void muteChat(const QString& aimId, bool mute);

        bool isFavorite(const QString& aimid) const;
        bool isServiceItem(const QModelIndex& i) const;
        bool isFavoritesGroupButton(const QModelIndex& i) const;
        bool isFavoritesVisible() const;

		QModelIndex contactIndex(const QString& aimId);

		int totalUnreads() const;
        
        QString firstContact();
        QString nextUnreadAimId();
        QString nextAimId(QString aimId);
        QString prevAimId(QString aimId);

	private:
        int correctIndex(int i) const;
        int favoritesIndent() const;

		std::vector<Data::DlgState> Dialogs_;
		QHash<QString, int> Indexes_;
		QTimer* Timer_;
        quint16 FavoritesCount_;
        bool FavoritesVisible_;
	};

	RecentsModel* GetRecentsModel();
    void ResetRecentsModel();
}