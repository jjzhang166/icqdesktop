#pragma once

#include "../../types/chat.h"

namespace Logic
{
	class LiveChatsModel : public QAbstractListModel
	{
		Q_OBJECT
    Q_SIGNALS:
        void selected(Data::ChatInfo);

	public:
		explicit LiveChatsModel(QObject* _parent);

        void initIfNeeded();

		int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
		QVariant data(const QModelIndex& _index, int _role) const override;
		Qt::ItemFlags flags(const QModelIndex& _index) const override;

        void select(const QModelIndex& _index);

        void pending(const QString& _aimId);

        void joined(const QString& _aimId);

    private Q_SLOTS:
        void chatsHome(QList<Data::ChatInfo> _chats, QString _newTag, bool _restart, bool _finished);
        void chatsHomeError(int);
        void avatarLoaded(QString);

    private:
        void requestMore() const;

	private:
        QList<Data::ChatInfo> cache_;
        QString tag_;
        mutable QString requestedTag_;
        bool finished_;
        bool inited_;
	};

	LiveChatsModel* GetLiveChatsModel();
    void ResetLiveChatsModel();
}