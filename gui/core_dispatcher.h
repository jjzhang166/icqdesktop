#pragma once

#include "voip/VoipProxy.h"

#include "../corelib/core_face.h"
#include "../corelib/collection_helper.h"

#include "types/contact.h"
#include "types/message.h"
#include "types/chat.h"
#include "types/typing.h"

#include "../corelib/enumerations.h"

namespace voip_manager
{
    struct Contact;
    struct ContactEx;
}

namespace voip_proxy
{
    struct device_desc;
}

namespace core
{
    enum class file_sharing_function;
    enum class sticker_size;
    enum class group_chat_info_errors;
}

namespace launch
{
    int main(int argc, char *argv[]);
}

namespace Utils
{
    struct ProxySettings;
}

namespace Ui
{
    namespace stickers
    {
        enum class sticker_size;
    }

    class gui_signal : public QObject
    {
        Q_OBJECT
    public:

Q_SIGNALS:
        void received(const QString, const qint64, core::icollection*);

    };

    class gui_connector : public gui_signal, public core::iconnector
    {
        std::atomic<int>	ref_count_;

        // ibase interface
        virtual int addref() override;
        virtual int release() override;

        // iconnector interface
        virtual void link(iconnector*) override;
        virtual void unlink() override;
        virtual void receive(const char *, int64_t, core::icollection*) override;
    public:
        gui_connector() : ref_count_(1) {}
    };

    enum class MessagesBuddiesOpt
    {
        Min,

        Requested,
        FromServer,
        DlgState,
        Pending,
        Init,
        MessageStatus,

        Max
    };

    typedef std::function<void(core::icollection*)> message_processed_callback;


    class core_dispatcher : public QObject
    {
        Q_OBJECT

Q_SIGNALS:
        void needLogin();
        void contactList(std::shared_ptr<Data::ContactList>, QString);
        void im_created();
        void login_complete();
        void messageBuddies(std::shared_ptr<Data::MessageBuddies>, QString, Ui::MessagesBuddiesOpt, bool, qint64);
        void getSmsResult(int64_t, int err_code, int code_length);
        void loginResult(int64_t, int code);
        void loginResultAttachUin(int64_t, int code);
        void loginResultAttachPhone(int64_t, int code);
        void avatarLoaded(const QString&, QPixmap*, int);

        void presense(Data::Buddy*);
        void searchResult(QStringList);
        void dlgState(Data::DlgState);
        void activeDialogHide(QString);
        void guiSettings();
        void themeSettings();
        void chatInfo(qint64, std::shared_ptr<Data::ChatInfo>);
        void chatBlocked(QList<Data::ChatMemberInfo>);
        void chatInfoFailed(qint64, core::group_chat_info_errors);
        void myInfo();
        void login_new_user();
        void messagesDeleted(QString, QList<int64_t>);
        void messagesDeletedUpTo(QString, int64_t);
        void messagesModified(QString, std::shared_ptr<Data::MessageBuddies>);

        void typingStatus(Logic::TypingFires _typing, bool _isTyping);

        void messagesReceived(QString, QVector< QString >);

        void contactRemoved(QString);
        void openChat(QString _aimid);

        void feedbackSent(bool);

        // sticker signals
        void on_stickers();
        void on_sticker(qint32 _set_id, qint32 _sticker_id);

        void on_themes_meta();
        void on_themes_meta_error();
        void on_theme(int,bool);

        // remote files signals
        void imageDownloaded(qint64, QString, QPixmap, QString);
        void imageDownloadError(int64_t seq, QString rawUri);

        void fileSharingError(qint64 seq, QString rawUri, qint32 errorCode);

        void fileSharingFileDownloaded(qint64 seq, QString rawUri, QString localPath);
        void fileSharingFileDownloading(qint64 seq, QString rawUri, qint64 bytesTransferred);

        void fileSharingFileMetainfoDownloaded(qint64 seq, QString filename, QString downloadUri, qint64 size);
        void fileSharingPreviewMetainfoDownloaded(qint64 seq, QString miniPreviewUri, QString fullPreviewUri);

        void fileSharingLocalCopyCheckCompleted(qint64 seq, bool success, QString localPath);

        void fileSharingUploadingProgress(QString uploadingId, qint64 bytesTransferred);
        void fileSharingUploadingResult(QString seq, bool success, QString uri, bool isFileTooBig);

        void signedUrl(QString);
        void speechToText(qint64, int, QString, int);

        void chatsHome(QList<Data::ChatInfo>, QString, bool, bool);
        void chatsHomeError(int);

        void recv_permit_deny(bool);

        void recvFlags(int);
        void updateProfile(int);
        void getUserProxy();

        void set_chat_role_result(int);
        void block_member_result(int);

        public Q_SLOTS:
            void received(const QString, const qint64, core::icollection*);

    public:
        core_dispatcher();
        virtual ~core_dispatcher();

        core::icollection* create_collection() const;
        qint64 post_message_to_core(const QString& message, core::icollection *collection, const message_processed_callback callback = nullptr);

        qint64 post_stats_to_core(core::stats::stats_event_names event_name);
        qint64 post_stats_to_core(core::stats::stats_event_names event_name, const core::stats::event_props_type& _props);
        void set_enabled_stats_post(bool _is_stats_enabled);
        bool get_enabled_stats_post() const;

        voip_proxy::VoipController& getVoipController();

        qint64 downloadSharedFile(const QString &contact, const QString &url, const QString &download_dir, const QString& filename, const core::file_sharing_function function);
        qint64 abortSharedFileDownloading(const QString &contact, const QString &url, const qint64 downloadingSeq);

        qint64 uploadSharedFile(const QString &contact, const QString &localPath);
        qint64 abortSharedFileUploading(const QString &contact, const QString &localPath, const QString &uploadingProcessId);

        qint64 getSticker(const qint32 setId, const qint32 stickerId, const core::sticker_size size);
        qint64 getTheme(const qint32 _themeId);

        int64_t downloadImage(const QUrl &uri, const QString& destination, const bool isPreview);

        qint64 delete_messages(const std::vector<int64_t> &_message_ids, const QString &_contact_aimid, const bool _for_all);
        qint64 delete_messages_from(const QString &_contact_aimid, const int64_t _from_id);

        bool is_im_created() const;
        
    private:
        typedef std::tuple<message_processed_callback, QDateTime> callback_info;

    private:
        bool init();
        void uninit();

        void cleanup_callbacks();
        void execute_callback(const int64_t seq, core::icollection* _params);
        void fileSharingDownloadResult(const int64_t seq, core::coll_helper _params);
        void imageDownloadResult(const int64_t seq, core::coll_helper _params);
        void fileUploadingProgress(core::coll_helper _params);
        void fileUploadingResult(core::coll_helper _params);
        void onEventTyping(core::coll_helper _params, bool _is_typing);

    private:

        core::iconnector* core_connector_;
        core::icore_interface* core_face_;
        voip_proxy::VoipController _voip_controller;
        core::iconnector* gui_connector_;
        std::unordered_map<int64_t, callback_info> callbacks_;
        QDateTime last_time_callbacks_cleaned_up_;

        bool is_stats_enabled_;
        bool is_im_created_;
    };

    core_dispatcher* GetDispatcher();
    void create_dispatcher();
    void destroy_dispatcher();
}