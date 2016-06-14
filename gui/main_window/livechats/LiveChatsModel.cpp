#include "stdafx.h"
#include "LiveChatsModel.h"

#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace
{
    const int preload_size = 30;
}

namespace Logic
{
    static std::unique_ptr<LiveChatsModel> g_livechat_model;

    LiveChatsModel::LiveChatsModel(QObject *parent)
        : QAbstractListModel(parent)
        , inited_(false)
        , finished_(false)
    {
        connect(Ui::GetDispatcher(), SIGNAL(chatsHome(QList<Data::ChatInfo>, QString, bool, bool)), this, SLOT(chatsHome(QList<Data::ChatInfo>, QString, bool, bool)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(chatsHomeError(int)), this, SLOT(chatsHomeError(int)), Qt::QueuedConnection);
        connect(GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarLoaded(QString)), Qt::QueuedConnection);
        requestedTag_ = "_";
    }

    void LiveChatsModel::initIfNeeded()
    {
        if (!inited_)
        {
            inited_ = true;
            requestMore();
        }
    }

    void LiveChatsModel::avatarLoaded(QString aimid)
    {
        if (!inited_)
            return;

        int i = 0;
        for (auto iter : cache_)
        {
            if (iter.AimId_ == aimid)
            {
                emit dataChanged(index(i), index(i));
                return;
            }

            int memberCount = 0;
            for (auto member : iter.Members_)
            {
                if (member.AimId_ == aimid)
                {
                    emit dataChanged(index(i), index(i));
                    return;
                }

                if (++memberCount > 3)
                    break;
            }

            ++i;
        }
    }

    int LiveChatsModel::rowCount(const QModelIndex &parent) const
    {
        return cache_.size();
    }

    QVariant LiveChatsModel::data(const QModelIndex &index, int role) const
    {
        int row = index.row();
        if (row < 0 || row > rowCount())
            return QVariant();

        if (role == Qt::DisplayRole && rowCount() - index.row() <= preload_size)
            requestMore();

        return QVariant::fromValue(cache_.at(row));
    }

    Qt::ItemFlags LiveChatsModel::flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return Qt::ItemIsEnabled;
        return QAbstractItemModel::flags(index) | Qt::ItemIsEnabled;
    }

    void LiveChatsModel::select(const QModelIndex& index)
    {
        int row = index.row();
        if (row < 0 || row > rowCount())
            return;

        emit selected(cache_[row]);
    }

    void LiveChatsModel::chatsHome(QList<Data::ChatInfo> _chats, QString _newTag, bool _restart, bool _finished)
    {
        tag_ = _newTag;
        finished_ = _finished;

        if (_restart)
        {
            cache_.clear();
            tag_.clear();
            finished_ = false;
            requestMore();
            emit dataChanged(index(0), index(0));
            return;
        }

        for (auto iter : _chats)
            cache_.append(iter);

        emit dataChanged(index(0), index(rowCount()));
    }

    void LiveChatsModel::chatsHomeError(int)
    {
        requestMore();
    }

    void LiveChatsModel::requestMore() const
    {
        if (tag_ == requestedTag_ || finished_)
            return;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("tag", tag_);
        Ui::GetDispatcher()->post_message_to_core("chats/home/get", collection.get());
        requestedTag_ = tag_;
    }

	LiveChatsModel* GetLiveChatsModel()
	{	
        if (!g_livechat_model)
            g_livechat_model.reset(new LiveChatsModel(0));

		return g_livechat_model.get();
	}

    void ResetLiveChatsModel()
    {
        if (g_livechat_model)
            g_livechat_model.reset();
    }
}