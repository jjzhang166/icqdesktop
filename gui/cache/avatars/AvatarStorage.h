#pragma once

namespace Logic
{
	typedef std::shared_ptr<const QPixmap> QPixmapSCptr;

	class AvatarStorage : public QObject
	{
		friend AvatarStorage* GetAvatarStorage();

		Q_OBJECT

	Q_SIGNALS:
		void avatarChanged(QString);

	private Q_SLOTS:
		void avatarLoaded(const QString &aimId, QPixmap *avatar, int size);

		void cleanup();

	public:
		~AvatarStorage();

		const QPixmapSCptr& Get(const QString& aimId, const QString& display_name, const int sizePx, const bool isFilled, bool& isDefault);

		const QPixmapSCptr& GetRounded(const QString& aimId, const QString& display_name, const int sizePx, const QString &state, const bool isFilled, bool& isDefault);

		QString GetLocal(const QString& aimId, const QString& display_name, const int sizePx, const bool isFilled);

        void UpdateDefaultAvatarIfNeed(const QString& aimId);

	private:
		typedef std::map<QString, QPixmapSCptr> CacheMap;

		AvatarStorage();

		void CleanupSecondaryCaches(const QString &aimId);

		const QPixmapSCptr& GetRounded(const QPixmap &avatar, const QString &aimId, const QString &state);

		CacheMap AvatarsByAimIdAndSize_;

		CacheMap AvatarsByAimId_;

		CacheMap RoundedAvatarsByAimIdAndSize_;

		std::set<QString> RequestedAvatars_;

		QStringList LoadedAvatars_;

		std::map<QString, QDateTime> TimesCache_;

		QTimer* Timer_;
	};

	AvatarStorage* GetAvatarStorage();
}