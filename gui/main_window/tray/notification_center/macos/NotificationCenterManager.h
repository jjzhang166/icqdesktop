#pragma once

namespace Ui
{
	class NotificationCenterManager : public QObject
	{
		Q_OBJECT

	Q_SIGNALS:
		void messageClicked(QString);
        void osxThemeChanged();
        
    private Q_SLOTS:
        void avatarChanged(QString);

	public:
		NotificationCenterManager();
		~NotificationCenterManager();
        
		void DisplayNotification(const QString& aimdId, const QString& senderNick, const QString& message);
		void HideNotifications(const QString& aimId);

		void Activated(const QString& aimId);
        void themeChanged();
        
//		void Remove(Microsoft::WRL::ComPtr<ABI::Windows::UI::Notifications::IToastNotification> notification, const QString& aimId);
        
        static void updateBadgeIcon(int unreads);
	};
}