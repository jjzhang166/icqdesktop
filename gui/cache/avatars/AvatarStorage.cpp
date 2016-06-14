#include "stdafx.h"
#include "AvatarStorage.h"
#include "../../main_window/contact_list/ContactListModel.h"

#include "../../core_dispatcher.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"

namespace
{
	QString CreateKey(const QString &aimId, const int sizePx);

	static int CLEANUP_TIMEOUT = 5 * 60 * 1000; //5min
}

namespace Logic
{
	AvatarStorage::AvatarStorage()
		: Timer_(new QTimer(this))
	{
		Timer_->setSingleShot(false);
		Timer_->setInterval(CLEANUP_TIMEOUT);
		Timer_->start();

        connect(Ui::GetDispatcher(), SIGNAL(avatarLoaded(const QString&, QPixmap*, int)), this, SLOT(avatarLoaded(const QString&, QPixmap*, int)), Qt::QueuedConnection);
		connect(Timer_, SIGNAL(timeout()), this, SLOT(cleanup()), Qt::QueuedConnection);
	}

	AvatarStorage::~AvatarStorage()
	{
	}

	const QPixmapSCptr& AvatarStorage::Get(const QString& aimId, const QString& displayName, const int sizePx, const bool isFilled, bool& isDefault, bool regenerate)
	{
		assert(!aimId.isEmpty());
		assert(sizePx > 0);

		TimesCache_[aimId] = QDateTime::currentDateTime();

		Out isDefault = !LoadedAvatars_.contains(aimId);
		const auto key = CreateKey(aimId, sizePx);

		auto iterByAimIdAndSize = AvatarsByAimIdAndSize_.find(key);
		if (iterByAimIdAndSize != AvatarsByAimIdAndSize_.end())
		{
			return iterByAimIdAndSize->second;
		}

		auto iterByAimId = AvatarsByAimId_.find(aimId);
		if (iterByAimId == AvatarsByAimId_.end())
		{
			auto drawDisplayName = displayName.trimmed();
			if (drawDisplayName.isEmpty())
            {
				drawDisplayName = GetContactListModel()->getDisplayName(aimId).trimmed();
            }

			auto defaultAvatar = Utils::GetDefaultAvatar(aimId, drawDisplayName, sizePx, isFilled);
			assert(defaultAvatar);

			const auto result = AvatarsByAimId_.emplace(aimId, std::make_shared<QPixmap>(std::move(defaultAvatar)));
			assert(result.second);

			iterByAimId = result.first;
		}

		const auto &avatarByAimId = *iterByAimId->second;
        assert(!avatarByAimId.isNull());

		const auto regenerateAvatar = ((avatarByAimId.width() < sizePx) && isDefault);
		if (regenerateAvatar || regenerate)
		{
			AvatarsByAimId_.erase(iterByAimId);
			CleanupSecondaryCaches(aimId);
			return Get(aimId, displayName, sizePx, isFilled, isDefault, regenerate);
		}

        int avatarWidth = avatarByAimId.width();
        int avatarHeight = avatarByAimId.height();
        QPixmap scaledImage;
        if (avatarHeight >= avatarWidth)
            scaledImage = avatarByAimId.scaledToWidth(sizePx, Qt::SmoothTransformation);
        else
            scaledImage = avatarByAimId.scaledToHeight(sizePx, Qt::SmoothTransformation);

		const auto result = AvatarsByAimIdAndSize_.emplace(key, std::make_shared<QPixmap>(std::move(scaledImage)));
		assert(result.second);

		auto requestedAvatarsIter = RequestedAvatars_.find(aimId);
		if (requestedAvatarsIter == RequestedAvatars_.end() || (avatarByAimId.width() < sizePx && avatarByAimId.height() < sizePx))
		{
			Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
			collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_int("size", sizePx); //request only needed size

			Ui::GetDispatcher()->post_message_to_core("avatars/get", collection.get());

			RequestedAvatars_.insert(aimId);
		}
        else
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            collection.set_value_as_int("size", sizePx); //request only needed size

            Ui::GetDispatcher()->post_message_to_core("avatars/show", collection.get());
        }

		return result.first->second;
	}

    void AvatarStorage::UpdateAvatar(const QString& aimId)
    {
        if (TimesCache_.find(aimId) != TimesCache_.end())
        {
            AvatarsByAimId_.erase(aimId);
            CleanupSecondaryCaches(aimId);
            RequestedAvatars_.erase(aimId);
            LoadedAvatars_.removeAll(aimId);
            TimesCache_.erase(aimId);
        }
    }
    
	const QPixmapSCptr& AvatarStorage::GetRounded(const QString& aimId, const QString& display_name, const int sizePx, const QString &state, const bool isFilled, bool& isDefault, bool regenerate, bool from_cl)
	{
		assert(!aimId.isEmpty());
		assert(sizePx > 0);

		const auto &avatar = Get(aimId, display_name, sizePx, isFilled, isDefault, regenerate);
        if (avatar->isNull())
        {
            assert(!"avatar is null");
            return avatar;
        }

		return GetRounded(*avatar, aimId, state, from_cl, isDefault);
	}

	QString AvatarStorage::GetLocal(const QString& aimId, const QString& display_name, const int sizePx, const bool isFilled)
	{
		bool isDefault = false;
		QPixmapSCptr avatar = Get(aimId, display_name, sizePx, isFilled, isDefault, false);

		QFile file(QString(QDir::tempPath() + "/%1_%2.png").arg(aimId).arg(isDefault ? "_def" : "_"));
		if (!file.exists())
		{
			file.open(QIODevice::WriteOnly);
			avatar->save(&file, "PNG");
		}

		QFileInfo fileInfo(file);
		return fileInfo.absoluteFilePath();
	}

	void AvatarStorage::avatarLoaded(const QString &aimId, QPixmap *pixmap, int size)
	{
		if (!pixmap)
		{
			return;
		}

		assert(!aimId.isEmpty());

        QPixmapSCptr avatar(pixmap);
        auto scaledImage = avatar->scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QPixmapSCptr cache = std::make_shared<QPixmap>(std::move(scaledImage));
		auto iterPixmap = AvatarsByAimId_.find(aimId);
		if (iterPixmap != AvatarsByAimId_.end())
		{
			iterPixmap->second = cache;
		}
		else
		{
			AvatarsByAimId_.emplace(aimId, cache);
		}

		CleanupSecondaryCaches(aimId);

		LoadedAvatars_ << aimId;

		emit avatarChanged(aimId);
	}

    void AvatarStorage::UpdateDefaultAvatarIfNeed(const QString& aimId)
    {
        assert(!aimId.isEmpty());

        if (LoadedAvatars_.contains(aimId))
            return;

        if (AvatarsByAimId_.find(aimId) != AvatarsByAimId_.end())
            AvatarsByAimId_.erase(aimId);
        CleanupSecondaryCaches(aimId);
        emit avatarChanged(aimId);
    }

	void AvatarStorage::cleanup()
	{
		QDateTime now = QDateTime::currentDateTime();
		for (std::map<QString, QDateTime>::iterator iter = TimesCache_.begin(); iter != TimesCache_.end();)
		{
			if (iter->second.msecsTo(now) >= CLEANUP_TIMEOUT)
			{
				AvatarsByAimId_.erase(iter->first);
				CleanupSecondaryCaches(iter->first);
				RequestedAvatars_.erase(iter->first);
				LoadedAvatars_.removeAll(iter->first);
				iter = TimesCache_.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

    // TODO : use two-step hash here
	void AvatarStorage::CleanupSecondaryCaches(const QString &aimId)
	{
		const auto cleanupSecondaryCache = [&aimId](CacheMap &cache)
		{
			for (auto i = cache.begin(); i != cache.end(); ++i)
			{
				const auto &key = i->first;
				if (!key.startsWith(aimId))
				{
					continue;
				}

				for(;;)
				{
					i = cache.erase(i);

					if (i == cache.end())
					{
						break;
					}

					const auto &key = i->first;
					if (!key.startsWith(aimId))
					{
						break;
					}
				}

				break;
			}
		};

		cleanupSecondaryCache(RoundedAvatarsByAimIdAndSize_);
		cleanupSecondaryCache(AvatarsByAimIdAndSize_);
	}

	const QPixmapSCptr& AvatarStorage::GetRounded(const QPixmap &avatar, const QString &aimId, const QString &state, bool from_cl, bool isDefault)
	{
		assert(!avatar.isNull());
		assert(!aimId.isEmpty());

		QString key;
		key.reserve(128);
		key += aimId;
		key += "/";
		key += QString::number(avatar.width());
		key += "/";
		key += state;

		auto i = RoundedAvatarsByAimIdAndSize_.find(key);
		if (i == RoundedAvatarsByAimIdAndSize_.end())
		{
			const auto roundedAvatar = Utils::RoundImage(avatar, state, isDefault, from_cl);
			i = RoundedAvatarsByAimIdAndSize_
				.emplace(
					key,
					std::make_shared<QPixmap>(std::move(roundedAvatar)))
				.first;
		}

		return i->second;
	}

	AvatarStorage* GetAvatarStorage()
	{
		static std::unique_ptr<AvatarStorage> storage(new AvatarStorage());
		return storage.get();
	}
}

namespace
{
	QString CreateKey(const QString &aimId, const int sizePx)
	{
		assert(!aimId.isEmpty());
		assert(sizePx > 0);

		QString result;
		result.reserve(128);

		result += aimId;
		result += "/";
		result += QString::number(sizePx);

		return result;
	}
}