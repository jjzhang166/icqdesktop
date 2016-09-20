#include "stdafx.h"
#include "core_dispatcher.h"

#include "app_config.h"
#include "gui_settings.h"
#include "my_info.h"
#include "theme_settings.h"
#include "main_window/contact_list/SearchMembersModel.h"
#include "types/typing.h"
#include "utils/gui_coll_helper.h"
#include "utils/InterConnector.h"
#include "utils/LoadPixmapFromDataTask.h"
#include "utils/uid.h"
#include "utils/utils.h"
#include "cache/stickers/stickers.h"
#include "cache/themes/themes.h"
#include "utils/log/log.h"
#include "../common.shared/common_defs.h"
#include "../corelib/corelib.h"
#include "../corelib/core_face.h"
#include "../core/connections/wim/loader/loader_errors.h"

#ifdef _WIN32
    #include "../common.shared/win32/crash_handler.h"
#endif

using namespace Ui;

int Ui::gui_connector::addref()
{
    return ++refCount_;
}

int Ui::gui_connector::release()
{
    if (0 == (--refCount_))
    {
        delete this;
        return 0;
    }

    return refCount_;
}

void Ui::gui_connector::link( iconnector*, const common::core_gui_settings&)
{

}

void Ui::gui_connector::unlink()
{

}

void Ui::gui_connector::receive( const char * _message, int64_t _seq, core::icollection* _messageData)
{
    if (_messageData)
        _messageData->addref();

    emit received(QString(_message), _seq, _messageData);
}

core_dispatcher::core_dispatcher()
    : coreConnector_(nullptr)
    , coreFace_(nullptr)
    , guiConnector_(nullptr)
    , voipController_(*this)
    , lastTimeCallbacksCleanedUp_(QDateTime::currentDateTimeUtc())
    , isStatsEnabled_(true)
    , isImCreated_(false)
    , userStateGoneAway_(false)
{
    init();
}


core_dispatcher::~core_dispatcher()
{
    uninit();
}

voip_proxy::VoipController& core_dispatcher::getVoipController()
{
    return voipController_;
}

qint64 core_dispatcher::downloadSharedFile(const QString& _contact, const QString& _url, const QString& _downloadDir, const QString& _fileName, const core::file_sharing_function _function)
{
    assert(!_contact.isEmpty());
    assert(!_url.isEmpty());
    assert(!_downloadDir.isEmpty());
    assert(_function > core::file_sharing_function::min);
    assert(_function < core::file_sharing_function::max);

    QDir().mkpath(_downloadDir); // just in case

    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("contact", _contact);
    helper.set<QString>("url", _url);
    helper.set<QString>("download_dir", _downloadDir);
    helper.set<QString>("filename", _fileName);
    helper.set<core::file_sharing_function>("function", _function);

    return post_message_to_core("files/download", helper.get());
}

qint64 core_dispatcher::requestFileDirectUri(const QString& _url, const QObject* _object, std::function<void(bool _res, const QString& _uri)> _callback)
{
    assert(!_url.isEmpty());

    core::coll_helper helper(create_collection(), true);
    helper.set<QString>("url", _url);

    return post_message_to_core("files/request_direct_uri", helper.get(), _object, [_callback](core::icollection* _collection)
    {
        gui_coll_helper collParams(_collection, false);

        QString uri;

        int32_t res = collParams.get<int32_t>("result");

        if (res == 0)
        {
            uri = collParams.get<QString>("url");
        }

        _callback((res == 0), uri);
    });
}

qint64 core_dispatcher::abortSharedFileDownloading(const qint64 _downloadingSeq)
{
    assert(_downloadingSeq > 0);

    core::coll_helper helper(create_collection(), true);
    helper.set_value_as_int64("process_seq", _downloadingSeq);

    return post_message_to_core("files/download/abort", helper.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QString& _localPath)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_string("contact", _contact.toUtf8().data());
    collection.set_value_as_string("file", _localPath.toUtf8().data());

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::uploadSharedFile(const QString& _contact, const QByteArray& _array, const QString& ext)
{
    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_string("contact", _contact.toUtf8().data());
    core::istream* stream = collection->create_stream();
    stream->write((uint8_t*)(_array.data()), _array.size());
    collection.set_value_as_stream("file_stream", stream);
    collection.set_value_as_string("ext", ext.toUtf8().data());

    return post_message_to_core("files/upload", collection.get());
}

qint64 core_dispatcher::abortSharedFileUploading(const QString& _contact, const QString& _localPath, const QString& _uploadingProcessId)
{
    assert(!_contact.isEmpty());
    assert(!_localPath.isEmpty());
    assert(!_uploadingProcessId.isEmpty());

    core::coll_helper helper(create_collection(), true);
    helper.set_value_as_string("contact", _contact.toStdString());
    helper.set_value_as_string("local_path", _localPath.toStdString());
    helper.set_value_as_string("process_seq", _uploadingProcessId.toStdString());

    return post_message_to_core("files/upload/abort", helper.get());
}

qint64 core_dispatcher::getSticker(const qint32 _setId, const qint32 _stickerId, const core::sticker_size _size)
{
    assert(_setId > 0);
    assert(_stickerId > 0);
    assert(_size > core::sticker_size::min);
    assert(_size < core::sticker_size::max);

    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("set_id", _setId);
    collection.set_value_as_int("sticker_id", _stickerId);
    collection.set_value_as_enum("size", _size);

    return post_message_to_core("stickers/sticker/get", collection.get());
}

qint64 core_dispatcher::getTheme(const qint32 _themeId)
{
    //assert(_themeId > 0);

    core::coll_helper collection(create_collection(), true);
    collection.set_value_as_int("theme_id", _themeId);

    return post_message_to_core("themes/theme/get", collection.get());
}

int64_t core_dispatcher::downloadImage(
    const QUrl& _uri,
    const QString& _contactAimid,
    const QString& _destination,
    const bool _isPreview,
    const int32_t _previewWidth,
    const int32_t _previewHeight)
{
    assert(!_contactAimid.isEmpty());
    assert(_uri.isValid());
    assert(!_uri.isLocalFile());
    assert(!_uri.isRelative());
    assert(_previewWidth >= 0);
    assert(_previewHeight >= 0);

    core::coll_helper collection(create_collection(), true);
    collection.set<QUrl>("uri", _uri);
    collection.set<QString>("destination", _destination);
    collection.set<bool>("is_preview", _isPreview);
    collection.set<QString>("contact", _contactAimid);

    if (_isPreview)
    {
        collection.set<int32_t>("preview_height", _previewHeight);
        collection.set<int32_t>("preview_width", _previewWidth);
    }

    const auto seq = post_message_to_core("image/download", collection.get());

    __INFO(
        "snippets",
        "GUI(1): requested image\n"
            __LOGP(seq, seq)
            __LOGP(uri, _uri)
            __LOGP(dst, _destination)
            __LOGP(preview, _isPreview)
            __LOGP(preview_width, _previewWidth)
            __LOGP(preview_height, _previewHeight));

    return seq;
}

void core_dispatcher::cancelImageDownloading(const int64_t _downloadSeq)
{
    assert(_downloadSeq > 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<int64_t>("download_seq", _downloadSeq);

    post_message_to_core("image/download/cancel", collection.get());
}

int64_t core_dispatcher::downloadLinkMetainfo(
    const QString& _contact,
    const QString& _uri,
    const int32_t _previewWidth,
    const int32_t _previewHeight)
{
    assert(!_contact.isEmpty());
    assert(!_uri.isEmpty());
    assert(_previewWidth >= 0);
    assert(_previewHeight >= 0);

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("uri", _uri);
    collection.set<QString>("contact", _contact);
    collection.set<int32_t>("preview_height", _previewHeight);
    collection.set<int32_t>("preview_width", _previewWidth);

    return post_message_to_core("link_metainfo/download", collection.get());
}

int64_t core_dispatcher::download_snap_metainfo(const QString& _contact, const QString& _ttlId)
{
    assert(!_ttlId.isEmpty());

    core::coll_helper collection(create_collection(), true);
    collection.set<QString>("ttl_id", _ttlId);
    collection.set<QString>("contact", _contact);

    return post_message_to_core("snap/get_metainfo", collection.get());
}

int64_t core_dispatcher::pttToText(const QString &_pttLink, const QString &_locale)
{
    assert(!_pttLink.isEmpty());
    assert(!_locale.isEmpty());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("url", _pttLink);
    collection.set<QString>("locale", _locale);

    return post_message_to_core("files/speech_to_text", collection.get());
}

int64_t core_dispatcher::setUrlPlayed(const QString& _url, const bool _isPlayed)
{
    assert(!_url.isEmpty());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("url", _url);
    collection.set<bool>("played", _isPlayed);

    return post_message_to_core("files/set_url_played", collection.get());
}

qint64 core_dispatcher::deleteMessages(const std::vector<int64_t> &_messageIds, const QString &_contactAimId, const bool _forAll)
{
    assert(!_messageIds.empty());
    assert(!_contactAimId.isEmpty());

    core::coll_helper params(create_collection(), true);

    core::ifptr<core::iarray> messages_ids(params->create_array());

    for (const auto id : _messageIds)
    {
        assert(id > 0);

        core::ifptr<core::ivalue> val(params->create_value());
        val->set_as_int64(id);
        messages_ids->push_back(val.get());
    }

    params.set_value_as_array("messages_ids", messages_ids.get());

    params.set<QString>("contact_aimid", _contactAimId);
    params.set<bool>("for_all", _forAll);

    return post_message_to_core("archive/messages/delete", params.get());
}

qint64 core_dispatcher::deleteMessagesFrom(const QString& _contactAimId, const int64_t _fromId)
{
    assert(!_contactAimId.isEmpty());
    assert(_fromId >= 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact_aimid", _contactAimId);
    collection.set<int64_t>("from_id", _fromId);

    return post_message_to_core("archive/messages/delete_from", collection.get());
}

qint64 core_dispatcher::raiseDownloadPriority(int64_t _procId)
{
    assert(_procId > 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<int64_t>("proc_id", _procId);

    return post_message_to_core("download/raise_priority", collection.get());
}

qint64 core_dispatcher::raiseContactDownloadsPriority(const QString &_contactAimid)
{
    assert(!_contactAimid.isEmpty());

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact", _contactAimid);

    return post_message_to_core("download/raise_contact_tasks_priority", collection.get());
}

void core_dispatcher::sendMessageToContact(const QString& _contact, const QString& _text)
{
    assert(!_contact.isEmpty());
    assert(!_text.isEmpty());
    assert((unsigned)_text.length() <= Utils::getInputMaximumChars());

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    collection.set<QString>("contact", _contact);
    collection.set_value_as_string("message", _text.toUtf8().data(), _text.toUtf8().size());

    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
}

void core_dispatcher::read_snap(
    const QString &_contact,
    const uint64_t _snapId,
    const bool _markPrevSnapsRead)
{
    assert(!_contact.isEmpty());
    assert(_snapId > 0);

    core::coll_helper collection(create_collection(), true);

    collection.set<QString>("contact_aimid", _contact);
    collection.set<uint64_t>("snap_id", _snapId);
    collection.set<bool>("mark_prev_snaps_read", _markPrevSnapsRead);

    post_message_to_core("snap/mark_as_read", collection.get());
}

bool core_dispatcher::init()
{
#ifndef __linux__
    QLibrary libcore(CORELIBRARY);
    if (!libcore.load())
    {
        assert(false);
        return false;
    }

    typedef bool (*get_core_instance_function)(core::icore_interface**);
    get_core_instance_function get_core_instance = (get_core_instance_function) libcore.resolve("get_core_instance");

    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace))
    {
        assert(false);
        return false;
    }

    coreFace_ = coreFace;
#else
    core::icore_interface* coreFace = nullptr;
    if (!get_core_instance(&coreFace) || !coreFace)
    {
        assert(false);
        return false;
    }
    coreFace_ = coreFace;
#endif //__linux__
    coreConnector_ = coreFace_->get_core_connector();
    if (!coreConnector_)
        return false;

    gui_connector* connector = new gui_connector();
    QObject::connect(connector, SIGNAL(received(QString, qint64, core::icollection*)), this, SLOT(received(QString, qint64, core::icollection*)), Qt::QueuedConnection);

    guiConnector_ = connector;

    common::core_gui_settings settings(Utils::scale_value(56));

    coreConnector_->link(guiConnector_, settings);

    initMessageMap();

    return true;
}

void core_dispatcher::initMessageMap()
{
    REGISTER_IM_MESSAGE("need_login", onNeedLogin);
    REGISTER_IM_MESSAGE("im/created", onImCreated);
    REGISTER_IM_MESSAGE("login/complete", onLoginComplete);
    REGISTER_IM_MESSAGE("contactlist", onContactList);
    REGISTER_IM_MESSAGE("contactlist/diff", onContactList);
    REGISTER_IM_MESSAGE("login_get_sms_code_result", onLoginGetSmsCodeResult);
    REGISTER_IM_MESSAGE("login_result", onLoginResult);
    REGISTER_IM_MESSAGE("avatars/get/result", onAvatarsGetResult);
    REGISTER_IM_MESSAGE("contact_presence", onContactPresence);
    REGISTER_IM_MESSAGE("gui_settings", onGuiSettings);
    REGISTER_IM_MESSAGE("theme_settings", onThemeSettings);
    REGISTER_IM_MESSAGE("search_result", onSearchResult);
    REGISTER_IM_MESSAGE("archive/images/get/result", onArchiveImagesGetResult);
    REGISTER_IM_MESSAGE("archive/messages/get/result", onArchiveMessagesGetResult);
    REGISTER_IM_MESSAGE("messages/received/dlg_state", onMessagesReceivedDlgState);
    REGISTER_IM_MESSAGE("messages/received/server", onMessagesReceivedServer);
    REGISTER_IM_MESSAGE("archive/messages/pending", onArchiveMessagesPending);
    REGISTER_IM_MESSAGE("messages/received/init", onMessagesReceivedInit);
    REGISTER_IM_MESSAGE("messages/received/message_status", onMessagesReceivedMessageStatus);
    REGISTER_IM_MESSAGE("messages/del_up_to", onMessagesDelUpTo);
    REGISTER_IM_MESSAGE("dlg_state", onDlgState);
    REGISTER_IM_MESSAGE("voip_signal", onVoipSignal);
    REGISTER_IM_MESSAGE("active_dialogs_are_empty", onActiveDialogsAreEmpty);
    REGISTER_IM_MESSAGE("active_dialogs_hide", onActiveDialogsHide);
    REGISTER_IM_MESSAGE("stickers/meta/get/result", onStickersMetaGetResult);
    REGISTER_IM_MESSAGE("themes/meta/get/result", onThemesMetaGetResult);
    REGISTER_IM_MESSAGE("themes/meta/get/error", onThemesMetaGetError);
    REGISTER_IM_MESSAGE("stickers/sticker/get/result", onStickersStickerGetResult);
    REGISTER_IM_MESSAGE("themes/theme/get/result", onThemesThemeGetResult);
    REGISTER_IM_MESSAGE("chats/info/get/result", onChatsInfoGetResult);
    REGISTER_IM_MESSAGE("chats/blocked/result", onChatsBlockedResult);
    REGISTER_IM_MESSAGE("chats/pending/result", onChatsPendingResult);
    REGISTER_IM_MESSAGE("chats/info/get/failed", onChatsInfoGetFailed);

    REGISTER_IM_MESSAGE("files/download/result", fileSharingDownloadResult);
    REGISTER_IM_MESSAGE("image/download/result", imageDownloadResult);
    REGISTER_IM_MESSAGE("image/download/progress", imageDownloadProgress);
    REGISTER_IM_MESSAGE("image/download/result/meta", imageDownloadResultMeta);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/meta", linkMetainfoDownloadResultMeta);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/image", linkMetainfoDownloadResultImage);
    REGISTER_IM_MESSAGE("link_metainfo/download/result/favicon", linkMetainfoDownloadResultFavicon);
    REGISTER_IM_MESSAGE("files/upload/progress", fileUploadingProgress);
    REGISTER_IM_MESSAGE("files/upload/result", fileUploadingResult);

    REGISTER_IM_MESSAGE("files/speech_to_text/result", onFilesSpeechToTextResult);
    REGISTER_IM_MESSAGE("contacts/remove/result", onContactsRemoveResult);
    REGISTER_IM_MESSAGE("app_config", onAppConfig);
    REGISTER_IM_MESSAGE("my_info", onMyInfo);
    REGISTER_IM_MESSAGE("signed_url", onSignedUrl);
    REGISTER_IM_MESSAGE("feedback/sent", onFeedbackSent);
    REGISTER_IM_MESSAGE("messages/received/senders", onMessagesReceivedSenders);
    REGISTER_IM_MESSAGE("typing", onTyping);
    REGISTER_IM_MESSAGE("typing/stop", onTypingStop);
    REGISTER_IM_MESSAGE("contacts/get_ignore/result", onContactsGetIgnoreResult);

    REGISTER_IM_MESSAGE("login_result_attach_uin", onLoginResultAttachUin);
    REGISTER_IM_MESSAGE("login_result_attach_phone", onLoginResultAttachPhone);
    REGISTER_IM_MESSAGE("recv_flags", onRecvFlags);
    REGISTER_IM_MESSAGE("update_profile/result", onUpdateProfileResult);
    REGISTER_IM_MESSAGE("chats/home/get/result", onChatsHomeGetResult);
    REGISTER_IM_MESSAGE("chats/home/get/failed", onChatsHomeGetFailed);
    REGISTER_IM_MESSAGE("user_proxy/result", onUserProxyResult);
    REGISTER_IM_MESSAGE("open_created_chat", onOpenCreatedChat);
    REGISTER_IM_MESSAGE("login_new_user", onLoginNewUser);
    REGISTER_IM_MESSAGE("set_avatar/result", onSetAvatarResult);
    REGISTER_IM_MESSAGE("chats/role/set/result", onChatsRoleSetResult);
    REGISTER_IM_MESSAGE("chats/block/result", onChatsBlockResult);
    REGISTER_IM_MESSAGE("chats/pending/resolve/result", onChatsPendingResolveResult);
    REGISTER_IM_MESSAGE("phoneinfo/result", onPhoneinfoResult);
    REGISTER_IM_MESSAGE("snap/get_metainfo/result", onSnapGetMetainfoResult);
    REGISTER_IM_MESSAGE("contacts/ignore/remove", onContactRemovedFromIgnore);

}

void core_dispatcher::uninit()
{
    if (guiConnector_)
    {
        coreConnector_->unlink();
        guiConnector_->release();

        guiConnector_ = nullptr;
    }

    if (coreConnector_)
    {
        coreConnector_->release();
        coreConnector_ = nullptr;
    }

    if (coreFace_)
    {
        coreFace_->release();
        coreFace_ = nullptr;
    }

}

void core_dispatcher::executeCallback(const int64_t _seq, core::icollection* _params)
{
    assert(_seq > 0);

    const auto callbackInfoIter = callbacks_.find(_seq);
    if (callbackInfoIter == callbacks_.end())
    {
        return;
    }

    const auto &callback = callbackInfoIter->second.callback_;

    assert(callback);
    callback(_params);

    callbacks_.erase(callbackInfoIter);
}

void core_dispatcher::cleanupCallbacks()
{
    const auto now = QDateTime::currentDateTimeUtc();

    const auto secsPassedFromLastCleanup = lastTimeCallbacksCleanedUp_.secsTo(now);
    const auto cleanTimeoutInSecs = 30;
    if (secsPassedFromLastCleanup < cleanTimeoutInSecs)
    {
        return;
    }

    for (auto pairIter = callbacks_.begin(); pairIter != callbacks_.end();)
    {
        const auto callbackInfo = pairIter->second;

        const auto callbackRegisteredTimestamp = callbackInfo.date_;

        const auto callbackAgeInSeconds = callbackRegisteredTimestamp.secsTo(now);

        const auto callbackAgeInSecondsMax = 60;
        if (callbackAgeInSeconds > callbackAgeInSecondsMax)
        {
            pairIter = callbacks_.erase(pairIter);
        }
        else
        {
            ++pairIter;
        }
    }

    lastTimeCallbacksCleanedUp_ = now;
}

void core_dispatcher::fileSharingDownloadResult(const int64_t _seq, core::coll_helper _params)
{
    const auto function = _params.get_value_as_enum<core::file_sharing_function>("function");

    const auto modeDownloadPreviewMetainfo = (function == core::file_sharing_function::download_preview_metainfo);
    const auto modeCheckLocalCopyExists = (function == core::file_sharing_function::check_local_copy_exists);
    const auto modeDownloadFile = (function == core::file_sharing_function::download_file);
    const auto modeDownloadFileMetainfo = (function == core::file_sharing_function::download_file_metainfo);

    const auto rawUri = _params.get<QString>("file_url");
    const auto errorCode = _params.get<int32_t>("error", 0);

    const auto requestFailed = (errorCode != 0);
    if ((modeDownloadFile || modeDownloadFileMetainfo || modeDownloadPreviewMetainfo) &&
        requestFailed)
    {
        emit fileSharingError(_seq, rawUri, errorCode);
        return;
    }

    if (modeDownloadPreviewMetainfo)
    {
        const auto miniPreviewUri = _params.get<QString>("mini_preview_uri");
        const auto fullPreviewUri = _params.get<QString>("full_preview_uri");

        emit fileSharingPreviewMetainfoDownloaded(_seq, miniPreviewUri, fullPreviewUri);

        return;
    }

    const auto filename = _params.get<QString>("file_name");
    const auto filenameShort = _params.get<QString>("file_name_short");
    const auto size = _params.get<int64_t>("file_size");
    const auto downloadUri = _params.get<QString>("file_dlink");
    const auto bytesTransfer = _params.get<int64_t>("bytes_transfer", 0);

    const auto isProgress = !_params.is_value_exist("error");
    if (isProgress)
    {
        assert(bytesTransfer >= 0);
        assert(modeDownloadFile);
        emit fileSharingFileDownloading(_seq, rawUri, bytesTransfer, size);
        return;
    }

    if (modeDownloadFile)
    {
        emit fileSharingFileDownloaded(_seq, rawUri, filename);
        return;
    }

    if (modeDownloadFileMetainfo)
    {
        emit fileSharingFileMetainfoDownloaded(_seq, filenameShort, downloadUri, size);
        return;
    }

    modeCheckLocalCopyExists;
    assert(modeCheckLocalCopyExists);

    emit fileSharingLocalCopyCheckCompleted(_seq, !requestFailed, filename);
}

void core_dispatcher::imageDownloadProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto bytesTotal = _params.get<int64_t>("bytes_total");
    const auto bytesTransferred = _params.get<int64_t>("bytes_transferred");
    const auto pctTransferred = _params.get<int32_t>("pct_transferred");

    assert(bytesTotal > 0);
    assert(bytesTransferred >= 0);
    assert(pctTransferred >= 0);
    assert(pctTransferred <= 100);

    emit imageDownloadingProgress(_seq, bytesTotal, bytesTransferred, pctTransferred);
}

void core_dispatcher::imageDownloadResult(const int64_t _seq, core::coll_helper _params)
{
    const auto isProgress = !_params.is_value_exist("error");
    if (isProgress)
    {
        return;
    }

    const auto rawUri = _params.get<QString>("url");
    const auto data = _params.get_value_as_stream("data");
    const auto local = _params.get<QString>("local");

    __INFO(
        "snippets",
        "completed image downloading\n"
            __LOGP(_seq, _seq)
            __LOGP(uri, rawUri)
            __LOGP(local_path, local)
            __LOGP(success, !data->empty()));

    if (data->empty())
    {
        emit imageDownloadError(_seq, rawUri);
        return;
    }

    assert(!local.isEmpty());
    assert(!rawUri.isEmpty());

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, rawUri, local]
        (QPixmap pixmap)
        {
            if (pixmap.isNull())
            {
                emit imageDownloadError(_seq, rawUri);
                return;
            }

            emit imageDownloaded(_seq, rawUri, pixmap, local);
        },
        Qt::QueuedConnection);
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::imageDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto previewWidth = _params.get<int32_t>("preview_width");
    const auto previewHeight = _params.get<int32_t>("preview_height");
    const auto downloadUri = _params.get<QString>("download_uri");
    const auto fileSize = _params.get<int64_t>("file_size");

    assert(previewWidth >= 0);
    assert(previewHeight >= 0);

    const QSize previewSize(previewWidth, previewHeight);

    Data::LinkMetadata linkMeta(QString(), QString(), QString(), QString(), previewSize, downloadUri, fileSize);

    emit imageMetaDownloaded(_seq, linkMeta);
}

void core_dispatcher::fileUploadingProgress(const int64_t _seq, core::coll_helper _params)
{
    const auto bytesUploaded = _params.get<int64_t>("bytes_transfer");
    const auto uploadingId = _params.get<QString>("uploading_id");

    emit fileSharingUploadingProgress(uploadingId, bytesUploaded);
}

void core_dispatcher::fileUploadingResult(const int64_t _seq, core::coll_helper _params)
{
    const auto uploadingId = _params.get<QString>("uploading_id");
    const auto localPath = _params.get<QString>("local_path");
    const auto link = _params.get<QString>("file_url");
    const auto contentType = _params.get<int32_t>("content_type");
    const auto error = _params.get<int32_t>("error");

    const auto success = (error == 0);
    const auto isFileTooBig = (error == (int32_t)core::wim::loader_errors::too_large_file);
    emit fileSharingUploadingResult(uploadingId, success, localPath, link, contentType, isFileTooBig);
}

void core_dispatcher::linkMetainfoDownloadResultMeta(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");
    const auto title = _params.get<QString>("title", "");
    const auto annotation = _params.get<QString>("annotation", "");
    const auto siteName = _params.get<QString>("site_name", "");
    const auto contentType = _params.get<QString>("content_type", "");
    const auto previewWidth = _params.get<int32_t>("preview_width", 0);
    const auto previewHeight = _params.get<int32_t>("preview_height", 0);
    const auto downloadUri = _params.get<QString>("download_uri", "");
    const auto fileSize = _params.get<int64_t>("file_size", -1);

    assert(previewWidth >= 0);
    assert(previewHeight >= 0);
    const QSize previewSize(previewWidth, previewHeight);

    const Data::LinkMetadata linkMetadata(title, annotation, siteName, contentType, previewSize, downloadUri, fileSize);

    emit linkMetainfoMetaDownloaded(_seq, success, linkMetadata);
}

void core_dispatcher::linkMetainfoDownloadResultImage(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        emit linkMetainfoImageDownloaded(_seq, false, QPixmap());
        return;
    }

    const auto data = _params.get_value_as_stream("data");

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, success]
        (QPixmap pixmap)
        {
            emit linkMetainfoImageDownloaded(_seq, success, pixmap);
        },
        Qt::QueuedConnection);
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::linkMetainfoDownloadResultFavicon(const int64_t _seq, core::coll_helper _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    if (!success)
    {
        emit linkMetainfoFaviconDownloaded(_seq, false, QPixmap());
        return;
    }

    const auto data = _params.get_value_as_stream("data");

    auto task = new Utils::LoadPixmapFromDataTask(data);

    const auto succeeded = QObject::connect(
        task, &Utils::LoadPixmapFromDataTask::loadedSignal,
        this,
        [this, _seq, success]
        (QPixmap pixmap)
        {
            emit linkMetainfoFaviconDownloaded(_seq, success, pixmap);
        },
        Qt::QueuedConnection);
    assert(succeeded);

    QThreadPool::globalInstance()->start(task);
}

void core_dispatcher::onFilesSpeechToTextResult(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");
    int comeback = _params.get_value_as_int("comeback");
    QString text = _params.get_value_as_string("text");

    emit speechToText(_seq, error, text, comeback);
}

void core_dispatcher::onContactsRemoveResult(const int64_t _seq, core::coll_helper _params)
{
    QString contact = _params.get_value_as_string("contact");

    emit contactRemoved(contact);
}

void core_dispatcher::onAppConfig(const int64_t _seq, core::coll_helper _params)
{
    Ui::AppConfigUptr config(new AppConfig(_params));

    Ui::SetAppConfig(config);

    emit appConfig();
}

void core_dispatcher::onMyInfo(const int64_t _seq, core::coll_helper _params)
{
    Ui::MyInfo()->unserialize(&_params);
    Ui::MyInfo()->CheckForUpdate();

    emit myInfo();
}

void core_dispatcher::onSignedUrl(const int64_t _seq, core::coll_helper _params)
{
    emit signedUrl(_params.get_value_as_string("url"));
}

void core_dispatcher::onFeedbackSent(const int64_t _seq, core::coll_helper _params)
{
    emit feedbackSent(_params.get_value_as_bool("succeeded"));
}

void core_dispatcher::onMessagesReceivedSenders(const int64_t _seq, core::coll_helper _params)
{
    QString aimId = _params.get_value_as_string("aimid");
    QVector< QString > sendersAimIds;
    if (_params.is_value_exist("senders"))
    {
        auto array = _params.get_value_as_array("senders");
        for (int i = 0; array && i < array->size(); ++i)
        {
            sendersAimIds.push_back(array->get_at(i)->get_as_string());
        }
    }

    emit messagesReceived(aimId, sendersAimIds);
}

void core_dispatcher::onTyping(const int64_t _seq, core::coll_helper _params)
{
    onEventTyping(_params, true);
}

void core_dispatcher::onTypingStop(const int64_t _seq, core::coll_helper _params)
{
    onEventTyping(_params, false);
}

void core_dispatcher::onContactsGetIgnoreResult(const int64_t _seq, core::coll_helper _params)
{
    QVector< QString > ignoredAimIds;

    auto array = _params.get_value_as_array("aimids");

    for (int i = 0; array && i < array->size(); ++i)
    {
        ignoredAimIds.push_back(array->get_at(i)->get_as_string());
    }

    Logic::updateIgnoredModel(ignoredAimIds);

    emit recvPermitDeny(ignoredAimIds.isEmpty());
}



core::icollection* core_dispatcher::create_collection() const
{
    core::ifptr<core::icore_factory> factory(coreFace_->get_factory());
    return factory->create_collection();
}

qint64 core_dispatcher::post_message_to_core(const QString& _message, core::icollection* _collection, const QObject* _object, const message_processed_callback _callback)
{
    const auto seq = Utils::get_uid();

    coreConnector_->receive(_message.toUtf8(), seq, _collection);

    if (_callback)
    {
        const auto result = callbacks_.emplace(seq, callback_info(_callback, QDateTime::currentDateTimeUtc(), _object));
        assert(result.second);
    }

    if (_object)
    {
        QObject::connect(_object, &QObject::destroyed, this, [this](QObject* _obj)
        {
            for (auto iter = callbacks_.begin(); iter != callbacks_.end(); ++iter)
            {
                if (iter->second.object_ == _obj)
                {
                    callbacks_.erase(iter);
                    return;
                }
            }
        });
    }


    return seq;
}

void core_dispatcher::set_enabled_stats_post(bool _isStatsEnabled)
{
    isStatsEnabled_ = _isStatsEnabled;
}

bool core_dispatcher::get_enabled_stats_post() const
{
    return isStatsEnabled_;
}

qint64 core_dispatcher::post_stats_to_core(core::stats::stats_event_names _eventName)
{
    core::stats::event_props_type props;
    return post_stats_to_core(_eventName, props);
}

qint64 core_dispatcher::post_stats_to_core(core::stats::stats_event_names _eventName, const core::stats::event_props_type& _props)
{
    if (!get_enabled_stats_post())
        return -1;

    assert(_eventName > core::stats::stats_event_names::min);
    assert(_eventName < core::stats::stats_event_names::max);

    core::coll_helper coll(create_collection(), true);
    coll.set_value_as_enum("event", _eventName);

    core::ifptr<core::iarray> propsArray(coll->create_array());

    for (const auto &prop : _props)
    {
        assert(!prop.first.empty() && !prop.second.empty());

        core::coll_helper collProp(coll->create_collection(), true);

        collProp.set_value_as_string("name", prop.first);
        collProp.set_value_as_string("value", prop.second);
        core::ifptr<core::ivalue> val(coll->create_value());
        val->set_as_collection(collProp.get());
        propsArray->push_back(val.get());
    }

    coll.set_value_as_array("props", propsArray.get());
    return post_message_to_core("stats", coll.get());
}

void core_dispatcher::received(const QString _receivedMessage, const qint64 _seq, core::icollection* _params)
{
    if (_seq > 0)
    {
        executeCallback(_seq, _params);
    }

    cleanupCallbacks();

    core::coll_helper collParams(_params, true);

    auto iter_handler = messages_map_.find(_receivedMessage.toStdString());
    if (iter_handler == messages_map_.end())
    {
        return;
    }

    iter_handler->second(_seq, collParams);
}

bool core_dispatcher::isImCreated() const
{
    return isImCreated_;
}

void core_dispatcher::onEventTyping(core::coll_helper _params, bool _isTyping)
{
    Logic::TypingFires currentTypingStatus(
        _params.get_value_as_string("aimId"),
        _params.get_value_as_string("chatterAimId"),
        _params.get_value_as_string("chatterName"));

    static std::list<Logic::TypingFires> typingFires;

    auto iterFires = std::find(typingFires.begin(), typingFires.end(), currentTypingStatus);

    if (_isTyping)
    {
        if (iterFires == typingFires.end())
        {
            typingFires.push_back(currentTypingStatus);
        }
        else
        {
            ++iterFires->counter_;
        }

        QTimer::singleShot(6000, [this, currentTypingStatus]()
        {
            auto iterFires = std::find(typingFires.begin(), typingFires.end(), currentTypingStatus);

            if (iterFires != typingFires.end())
            {
                --iterFires->counter_;
                if (iterFires->counter_ <= 0)
                {
                    typingFires.erase(iterFires);
                    emit typingStatus(currentTypingStatus, false);
                }
            }
        });
    }
    else
    {
        if (iterFires != typingFires.end())
        {
            --iterFires->counter_;

            if (iterFires->counter_ <= 0)
            {
                typingFires.erase(iterFires);
            }
        }
    }

    emit typingStatus(currentTypingStatus, _isTyping);
}



void core_dispatcher::setUserState(const core::profile_state state)
{
    assert(state > core::profile_state::min);
    assert(state < core::profile_state::max);

    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);

    std::stringstream stream;
    stream << state;

    collection.set_value_as_string("state", stream.str());
    collection.set_value_as_string("aimid", MyInfo()->aimId().toStdString());
    Ui::GetDispatcher()->post_message_to_core("set_state", collection.get());
    post_stats_to_core(stateToStatisticsEvent(state));
}

void core_dispatcher::invokeStateAway()
{
    // dnd or invisible can't swap to away
    if (MyInfo()->state().toLower() == "online")
    {
        userStateGoneAway_ = true;
        setUserState(core::profile_state::away);
    }
}

void core_dispatcher::invokePreviousState()
{
    if (userStateGoneAway_)
    {
        userStateGoneAway_ = false;
        setUserState(core::profile_state::online);
    }
}

void core_dispatcher::onNeedLogin(const int64_t _seq, core::coll_helper _params)
{
    userStateGoneAway_ = false;

    emit needLogin();
}

void core_dispatcher::onImCreated(const int64_t _seq, core::coll_helper _params)
{
    isImCreated_ = true;

    emit im_created();
}

void core_dispatcher::onLoginComplete(const int64_t _seq, core::coll_helper _params)
{
    emit loginComplete();
}

void core_dispatcher::onContactList(const int64_t _seq, core::coll_helper _params)
{
    auto cl = std::make_shared<Data::ContactList>();

    QString type;

    Data::UnserializeContactList(&_params, *cl, type);

    emit contactList(cl, type);
}

void core_dispatcher::onLoginGetSmsCodeResult(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int code_length = _params.get_value_as_int("code_length", 0);
    int error = result ? 0 : _params.get_value_as_int("error");

    emit getSmsResult(_seq, error, code_length);
}

void core_dispatcher::onLoginResult(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    emit loginResult(_seq, error);
}

void core_dispatcher::onAvatarsGetResult(const int64_t _seq, core::coll_helper _params)
{
    std::unique_ptr<QPixmap> avatar(Data::UnserializeAvatar(&_params));

    const QString contact(_params.get_value_as_string("contact"));

    const int size = _params.get_value_as_int("size");

    emit avatarLoaded(contact, avatar.release(), size);
}

void core_dispatcher::onContactPresence(const int64_t _seq, core::coll_helper _params)
{
    emit presense(Data::UnserializePresence(&_params));
}

void core_dispatcher::onGuiSettings(const int64_t _seq, core::coll_helper _params)
{
#ifdef _WIN32
    core::dump::set_os_version(_params.get_value_as_string("os_version"));
#endif

    get_gui_settings()->unserialize(_params);

    emit guiSettings();
}

void core_dispatcher::onThemeSettings(const int64_t _seq, core::coll_helper _params)
{
    get_qt_theme_settings()->unserialize(_params);

    emit themeSettings();
}

void core_dispatcher::onSearchResult(const int64_t _seq, core::coll_helper _params)
{
    QStringList contacts;

    Data::UnserializeSearchResult(&_params, contacts);

    emit searchResult(contacts);
}

void core_dispatcher::onArchiveImagesGetResult(const int64_t _seq, core::coll_helper _params)
{
    emit getImagesResult(Data::UnserializeImages(_params));
}

void core_dispatcher::onArchiveMessages(Ui::MessagesBuddiesOpt _type, const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    QString aimId;
    bool havePending = false;
    auto msgs = std::make_shared<Data::MessageBuddies>();
    auto modifications = std::make_shared<Data::MessageBuddies>();

    Data::UnserializeMessageBuddies(&_params, myAimid, Out aimId, Out havePending, Out *msgs, Out *modifications);

    emit messageBuddies(msgs, aimId, _type, havePending, _seq);

    if (_params.is_value_exist("deleted"))
    {
        QList<int64_t> deletedIds;

        const auto deletedIdsArray = _params.get_value_as_array("deleted");
        assert(!deletedIdsArray->empty());

        for (auto i = 0; i < deletedIdsArray->size(); ++i)
        {
            deletedIds.push_back(deletedIdsArray->get_at(i)->get_as_int64());
        }

        emit messagesDeleted(aimId, deletedIds);
    }

    if (!modifications->empty())
    {
        emit messagesModified(aimId, modifications);
    }
}

void core_dispatcher::onArchiveMessagesGetResult(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Requested, _seq, _params);
}

void core_dispatcher::onMessagesReceivedDlgState(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::DlgState, _seq, _params);
}

void core_dispatcher::onMessagesReceivedServer(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::FromServer, _seq, _params);
}

void core_dispatcher::onArchiveMessagesPending(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Pending, _seq, _params);
}

void core_dispatcher::onMessagesReceivedInit(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::Init, _seq, _params);
}

void core_dispatcher::onMessagesReceivedMessageStatus(const int64_t _seq, core::coll_helper _params)
{
    onArchiveMessages(Ui::MessagesBuddiesOpt::MessageStatus, _seq, _params);
}

void core_dispatcher::onMessagesDelUpTo(const int64_t _seq, core::coll_helper _params)
{
    const auto id = _params.get<int64_t>("id");
    assert(id > -1);

    const auto contact = _params.get<QString>("contact");
    assert(!contact.isEmpty());

    emit messagesDeletedUpTo(contact, id);
}

void core_dispatcher::onDlgState(const int64_t _seq, core::coll_helper _params)
{
    const auto myAimid = _params.get<QString>("my_aimid");

    Data::DlgState state;
    Data::UnserializeDlgState(&_params, myAimid, Out state);

    emit dlgState(state);

}

void core_dispatcher::onVoipSignal(const int64_t _seq, core::coll_helper _params)
{
    voipController_.handlePacket(_params);
}

void core_dispatcher::onActiveDialogsAreEmpty(const int64_t _seq, core::coll_helper _params)
{
    emit Utils::InterConnector::instance().showNoRecentsYet();
}

void core_dispatcher::onActiveDialogsHide(const int64_t _seq, core::coll_helper _params)
{
    QString aimId = Data::UnserializeActiveDialogHide(&_params);

    emit activeDialogHide(aimId);
}

void core_dispatcher::onStickersMetaGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::unserialize(_params);

    emit onStickers();
}

void core_dispatcher::onThemesMetaGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::themes::unserialize(_params);

    get_qt_theme_settings()->flushThemesToLoad();   // here we call delayed themes requests

    emit onThemesMeta();
}

void core_dispatcher::onThemesMetaGetError(const int64_t _seq, core::coll_helper _params)
{
    emit onThemesMetaError();
}

void core_dispatcher::onStickersStickerGetResult(const int64_t _seq, core::coll_helper _params)
{
    Ui::Stickers::setStickerData(_params);

    emit onSticker(
        _params.get_value_as_int("set_id"),
        _params.get_value_as_int("sticker_id"));
}

void core_dispatcher::onThemesThemeGetResult(const int64_t _seq, core::coll_helper _params)
{
    bool failed = _params.get_value_as_int("failed", 0) != 0;
    if (!failed)
    {
        Ui::themes::setThemeData(_params);
    }
    emit onTheme(_params.get_value_as_int("theme_id"), failed);
}

void core_dispatcher::onChatsInfoGetResult(const int64_t _seq, core::coll_helper _params)
{
    auto info = std::make_shared<Data::ChatInfo>();

    Data::UnserializeChatInfo(&_params, *info);

    if (!info->AimId_.isEmpty())
        emit chatInfo(_seq, info);
}

void core_dispatcher::onChatsBlockedResult(const int64_t _seq, core::coll_helper _params)
{
    QList<Data::ChatMemberInfo> blocked;

    Data::UnserializeChatMembers(&_params, blocked);

    emit chatBlocked(blocked);
}

void core_dispatcher::onChatsPendingResult(const int64_t _seq, core::coll_helper _params)
{
    QList<Data::ChatMemberInfo> pending;

    Data::UnserializeChatMembers(&_params, pending);

    emit chatPending(pending);

}

void core_dispatcher::onChatsInfoGetFailed(const int64_t _seq, core::coll_helper _params)
{
    auto errorCode = _params.get_value_as_int("error");

    emit chatInfoFailed(_seq, (core::group_chat_info_errors) errorCode);
}

void core_dispatcher::onLoginResultAttachUin(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    emit loginResultAttachUin(_seq, error);
}

void core_dispatcher::onLoginResultAttachPhone(const int64_t _seq, core::coll_helper _params)
{
    bool result = _params.get_value_as_bool("result");
    int error = result ? 0 : _params.get_value_as_int("error");

    emit loginResultAttachPhone(_seq, error);
}

void core_dispatcher::onRecvFlags(const int64_t _seq, core::coll_helper _params)
{
    emit recvFlags(_params.get_value_as_int("flags"));
}

void core_dispatcher::onUpdateProfileResult(const int64_t _seq, core::coll_helper _params)
{
    emit updateProfile(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsHomeGetResult(const int64_t _seq, core::coll_helper _params)
{
    bool restart = false, finished = false;
    QString newTag;
    QList<Data::ChatInfo> chats;

    UnserializeChatHome(&_params, chats, newTag, restart, finished);

    emit chatsHome(chats, newTag, restart, finished);
}

void core_dispatcher::onChatsHomeGetFailed(const int64_t _seq, core::coll_helper _params)
{
    int error = _params.get_value_as_int("error");

    emit chatsHomeError(error);
}

void core_dispatcher::onUserProxyResult(const int64_t _seq, core::coll_helper _params)
{
    Utils::ProxySettings userProxy;

    userProxy.type_ = (core::proxy_types)_params.get_value_as_int("settings_proxy_type", (int32_t)core::proxy_types::auto_proxy);
    userProxy.proxyServer_ = _params.get_value_as_string("settings_proxy_server", "");
    userProxy.port_ = _params.get_value_as_int("settings_proxy_port", Utils::ProxySettings::invalidPort);
    userProxy.username_ = _params.get_value_as_string("settings_proxy_username", "");
    userProxy.password_ = _params.get_value_as_string("settings_proxy_password", "");
    userProxy.needAuth_ = _params.get_value_as_bool("settings_proxy_need_auth", false);

    *Utils::get_proxy_settings() = userProxy;

    emit getUserProxy();
}

void core_dispatcher::onOpenCreatedChat(const int64_t _seq, core::coll_helper _params)
{
    auto aimId = _params.get_value_as_string("aimId");

    emit openChat(aimId);
}

void core_dispatcher::onLoginNewUser(const int64_t _seq, core::coll_helper _params)
{
    emit login_new_user();
}

void core_dispatcher::onSetAvatarResult(const int64_t _seq, core::coll_helper _params)
{
    emit Utils::InterConnector::instance().setAvatar(_params.get_value_as_int64("seq"), _params.get_value_as_int("error"));
}

void core_dispatcher::onChatsRoleSetResult(const int64_t _seq, core::coll_helper _params)
{
    emit setChatRoleResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsBlockResult(const int64_t _seq, core::coll_helper _params)
{
    emit blockMemberResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onChatsPendingResolveResult(const int64_t _seq, core::coll_helper _params)
{
    emit pendingListResult(_params.get_value_as_int("error"));
}

void core_dispatcher::onPhoneinfoResult(const int64_t _seq, core::coll_helper _params)
{
    Data::PhoneInfo data;
    data.deserialize(&_params);

    emit phoneInfoResult(_seq, data);
}

void core_dispatcher::onSnapGetMetainfoResult(int64_t _seq, const core::coll_helper& _params)
{
    assert(_seq > 0);

    const auto success = _params.get<bool>("success");

    const auto snapId = _params.get<uint64_t>("snap_id", 0);
    const auto expireUtc = _params.get<int64_t>("expire_utc", 0);
    const auto authorUin = _params.get<QString>("author_uin", "");
    const auto authorName = _params.get<QString>("author_name", "");

    emit snapMetainfoDownloaded(_seq, success, snapId, expireUtc, authorUin, authorName);
}


void core_dispatcher::onContactRemovedFromIgnore(const int64_t _seq, core::coll_helper _params)
{
    auto cl = std::make_shared<Data::ContactList>();

    QString type;

    Data::UnserializeContactList(&_params, *cl, type);

    emit contactList(cl, type);
}



namespace { std::unique_ptr<core_dispatcher> gDispatcher; }

core_dispatcher* Ui::GetDispatcher()
{
    if (!gDispatcher)
    {
        assert(false);
    }

    return gDispatcher.get();
}

void Ui::createDispatcher()
{
    if (gDispatcher)
    {
        assert(false);
    }

    gDispatcher.reset(new core_dispatcher());
}

void Ui::destroyDispatcher()
{
    if (!gDispatcher)
    {
        assert(false);
    }

    gDispatcher.reset();
}

