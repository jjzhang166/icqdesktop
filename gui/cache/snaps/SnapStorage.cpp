#include "stdafx.h"
#include "SnapStorage.h"
#include "../../core_dispatcher.h"
#include "../../main_window/history_control/complex_message/FileSharingUtils.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../main_window/contact_list/SnapItemDelegate.h"
#include "../../my_info.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../../common.shared/loader_errors.h"

namespace
{
    const int snap_border_radius = 8;
}

namespace Logic
{
SnapStorage::SnapStorage()
{
    connect(Ui::GetDispatcher(), SIGNAL(userSnaps(Logic::UserSnapsInfo, bool)), this, SLOT(userSnaps(Logic::UserSnapsInfo, bool)), Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), SIGNAL(userSnapsStorage(QList<Logic::UserSnapsInfo>, bool)), this, SLOT(userSnapsStorage(QList<Logic::UserSnapsInfo>, bool)), Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), SIGNAL(snapPreviewInfoDownloaded(qint64, QString, QString)), this, SLOT(snapPreviewInfoDownloaded(qint64, QString, QString)), Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), SIGNAL(imageDownloaded(qint64, QString, QPixmap, QString)), this, SLOT(imageDownloaded(qint64, QString, QPixmap, QString)), Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloaded(qint64, QString, QString)), this, SLOT(fileDownloaded(qint64, QString, QString)), Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), SIGNAL(fileSharingFileDownloading(qint64, QString, qint64, qint64)), this, SLOT(fileDownloading(qint64, QString, qint64, qint64)), Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), SIGNAL(fileSharingError(qint64, QString, qint32)), this, SLOT(fileSharingError(qint64, QString, qint32)), Qt::QueuedConnection);
}

SnapStorage::~SnapStorage()
{

}

QVariant SnapStorage::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid())
        return QVariant();

    return QVariant::fromValue(getData(_index.row(), _index.column()));
}

Qt::ItemFlags SnapStorage::flags(const QModelIndex& _index) const
{
    if (!_index.isValid())
        return Qt::ItemIsEnabled;

    unsigned flags = QAbstractItemModel::flags(_index) | Qt::ItemIsEnabled;

    flags |= ~Qt::ItemIsEditable;

    return (Qt::ItemFlags)flags;
}

void SnapStorage::startTv(int row, int col)
{
    PlayList_.clear();

    auto& index = Index_;
    if (row == getFeaturedRow())
        index = FeaturedIndex_;
    else if (row != getFriendsRow())
        return;

    if (col > index.size())
        return;

    for (auto i = col; i < index.size(); ++i)
    {
        auto u = index[i];
        {
            for (auto s : u.Snaps_)
            {
                if (s.getId() > u.State_.LastViewedSnapId_ || u.State_.LastViewedSnapId_ == -1)
                {
                    PlayItem snap;
                    snap.AimId_ = u.AimId_;
                    snap.OriginalAimId_ = s.OriginalAimId_;
                    snap.Url_ = s.Url_;
                    snap.Id_ = s.SnapId_;
                    snap.Preview_ = s.FullPreview_;
                    snap.LocalPath_ = s.LocalPath_;
                    snap.First_ = PlayList_.isEmpty();
                    PlayList_.push_back(snap);
                }
            }
        }
    }

    if (PlayList_.isEmpty())
    {
        for (auto i = col; i < index.size(); ++i)
        {
            auto u = index[i];
            {
                for (auto s : u.Snaps_)
                {
                    PlayItem snap;
                    snap.AimId_ = u.AimId_;
                    snap.OriginalAimId_ = s.OriginalAimId_;
                    snap.Url_ = s.Url_;
                    snap.Id_ = s.SnapId_;
                    snap.Preview_ = s.FullPreview_;
                    snap.LocalPath_ = s.LocalPath_;
                    snap.First_ = PlayList_.isEmpty();
                    PlayList_.push_back(snap);
                }
            }
        }
    }
    
    if (!PlayList_.isEmpty() && PlayList_.front().LocalPath_.isEmpty())
        emit tvStarted();

    processPlaylist();
}

void SnapStorage::startUserSnaps(const QString& _aimId)
{
    auto aimId = _aimId.isEmpty() ? Ui::MyInfo()->aimId() : _aimId;
    if (!Snaps_.contains(aimId))
        return;

    auto u = Snaps_[aimId];
    if (u.Snaps_.isEmpty())
        return;

    for (auto s : u.Snaps_)
    {
        PlayItem snap;
        snap.AimId_ = u.AimId_;
        snap.OriginalAimId_ = s.OriginalAimId_;
        snap.Url_ = s.Url_;
        snap.Id_ = s.SnapId_;
        snap.Preview_ = s.FullPreview_;
        snap.LocalPath_ = s.LocalPath_;
        snap.User_ = true;
        snap.First_ = PlayList_.isEmpty();
        PlayList_.push_back(snap);
    }

    processPlaylist();
}

QPixmap SnapStorage::getFirstUserPreview(const QString& _aimId)
{
    QPixmap result;
    auto aimId = _aimId.isEmpty() ? Ui::MyInfo()->aimId() : _aimId;
    if (!Snaps_.contains(aimId))
        return result;

    auto u = Snaps_[aimId];
    if (u.Snaps_.isEmpty())
        return result;

    return u.Snaps_.first().SourcePreview_;
}

QPixmap SnapStorage::getSnapPreviewFull(qint64 _id)
{
    for (auto u : Snaps_)
    {
        for (auto s : u.Snaps_)
        {
            if (s.SnapId_ == _id)
                return s.FullPreview_;
        }
    }

    return QPixmap();
}

int SnapStorage::getSnapsCount(const QString& _aimId) const
{
    auto u = Snaps_.find(_aimId);
    if (u == Snaps_.end())
        return 0;

    return u->Snaps_.size();
}

int SnapStorage::loadingSnapsCount(const QString& _aimId) const
{
    int result = 0;
    for (auto p : PlayList_)
    {
        if (p.AimId_ == _aimId)
            ++result;
    }

    return result;
}

QString SnapStorage::getFriednly(const QString& _aimId) const
{
    QString result = Logic::getContactListModel()->getDisplayName(_aimId);
    if (Snaps_.find(_aimId) == Snaps_.end())
    {
        for (auto u : Snaps_)
        {
            for (auto s : u.Snaps_)
            {
                if (s.OriginalAimId_ == _aimId)
                {
                    return s.OriginalFriendly_.isEmpty() ? result : s.OriginalFriendly_;
                }
            }
        }
        return result;
    }
    else if (Snaps_[_aimId].Friendly_.isEmpty())
    {
        return result;
    }

    if (result != _aimId)
        return result;

    return Snaps_[_aimId].Friendly_;
}

int SnapStorage::getViews(qint64 id) const
{
    for (auto u : Snaps_)
    {
        for (auto s : u.Snaps_)
        {
            if (s.SnapId_ == id)
                return s.Views_;
        }
    }

    return 0;
}

int32_t SnapStorage::getTimestamp(qint64 id) const
{
    for (auto u : Snaps_)
    {
        for (auto s : u.Snaps_)
        {
            if (s.SnapId_ == id)
                return s.Timestamp_;
        }
    }

    return 0;
}

PlayItem SnapStorage::getLoadingSnap() const
{
    if (PlayList_.isEmpty())
        return PlayItem();

    return PlayList_.front();
}

bool SnapStorage::haveLoading() const
{
    return !PlayList_.empty();
}

int SnapStorage::getFriendsSnapsCount() const
{
    return Index_.size();
}

int SnapStorage::getFeaturedSnapsCount() const
{
    return FeaturedIndex_.size();
}

void SnapStorage::refresh()
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    Ui::GetDispatcher()->post_message_to_core("snaps/refresh", collection.get());
}

void SnapStorage::fileDownloaded(qint64 _seq, QString _url, QString _localPath)
{
    if (!Seq_.contains(_seq))
        return;

    Seq_.removeAll(_seq);

    updateSnapLocalPath(_url, _localPath);

    for (QMap<QString, Logic::UserSnapsInfo>::iterator i = Snaps_.begin(); i != Snaps_.end(); ++i)
    {
        for (QList<SnapInfo>::iterator is = i->Snaps_.begin(); is != i->Snaps_.end(); ++is)
        {
            if (is->Url_ == _url)
            {
                is->LocalPath_ = _localPath;
                for (QList<PlayItem>::iterator iter = PlayList_.begin(); iter != PlayList_.end(); ++iter)
                {
                    if (iter->Url_ == _url)
                    {
                        iter->LocalPath_ = _localPath;
                        processPlaylist();
                        return;
                    }
                }
            }
        }
    }
}

void SnapStorage::fileDownloading(qint64 _seq, QString _url, qint64 _downloaded, qint64 _total)
{
    if (!Seq_.contains(_seq))
        return;

    for (auto i : PlayList_)
    {
        if (i.Url_ == _url)
        {
            int percents = ((float)_downloaded / _total) * 100;
            emit snapProgress(_url, percents);
            return;
        }
    }
}

void SnapStorage::fileSharingError(qint64 _seq, QString _url, qint32 _err)
{
    if (!Seq_.contains(_seq))
        return;

    Seq_.removeAll(_seq);

    if ((loader_errors)_err == loader_errors::metainfo_not_found)
    {
        if (PlayList_.begin()->Url_ == _url)
            PlayList_.pop_front();
    }

    processPlaylist();
}

void SnapStorage::userSnaps(Logic::UserSnapsInfo _info, bool _fromRefresh)
{
    if (!updateSnaps(_info) && !_fromRefresh)
        return;

    rebuildIndex(false);
}

void SnapStorage::userSnapsStorage(QList<Logic::UserSnapsInfo> _snaps, bool _fromCache)
{
    for (auto s : _snaps)
    {
        updateSnaps(s);
    }

    rebuildIndex(_fromCache);

    if (_fromCache)
        refresh();
}

void SnapStorage::snapPreviewInfoDownloaded(qint64 _snapId, QString _preview, QString _ttl_id)
{
    for (QMap<QString, Logic::UserSnapsInfo>::iterator i = Snaps_.begin(); i != Snaps_.end(); ++i)
    {
        for (QList<SnapInfo>::iterator is = i->Snaps_.begin(); is != i->Snaps_.end(); ++is)
        {
            if (is->getId() == _snapId || is->TtlId_ == _ttl_id)
            {
                is->PreviewUri_ = _preview;
                if (!_preview.isEmpty())
                {
                    is->PreviewSeq_ = Ui::GetDispatcher()->downloadImage(_preview, i->AimId_, QString(), false, 0, 0);
                    Seq_.push_back(is->PreviewSeq_);
                }
            }
        }
    }
}

void SnapStorage::imageDownloaded(qint64 _seq, QString _rawUri, QPixmap _image, QString _localPath)
{
    if (!Seq_.contains(_seq))
        return;

    for (QMap<QString, Logic::UserSnapsInfo>::iterator i = Snaps_.begin(); i != Snaps_.end(); ++i)
    {
        for (QList<SnapInfo>::iterator is = i->Snaps_.begin(); is != i->Snaps_.end(); ++is)
        {
            if (is->PreviewSeq_ == _seq)
            {
                is->SourcePreview_ = _image;
                is->MiniPreview_ = preparePreview(_image);
                is->FullPreview_ = prepareFull(_image);
                is->PreviewSeq_ = -1;
                Seq_.removeAll(_seq);

                updateSnapImage(is->getId(), _image);
                return;
            }
        }
    }
}

bool SnapStorage::updateSnaps(Logic::UserSnapsInfo _info)
{
    const auto haveSnaps = !_info.Snaps_.empty();
    QStringList snapsUrls;
    if (!haveSnaps)
    {
        Snaps_.remove(_info.AimId_);
    }
    else
    {
        if (Snaps_.contains(_info.AimId_))
        {
            if (Snaps_[_info.AimId_].Snaps_.size() == _info.Snaps_.size())
            {
                for (int i = 0; i < _info.Snaps_.size(); ++i)
                {
                    if (Snaps_[_info.AimId_].Snaps_[i].getId() != _info.Snaps_[i].getId())
                    {
                        snapsUrls << _info.Snaps_[i].Url_;
                    }
                }
            }
            else
            {
                for (auto s : _info.Snaps_)
                {
                    snapsUrls << s.Url_;
                }
            }

            for (QList<SnapInfo>::iterator is = _info.Snaps_.begin(); is != _info.Snaps_.end(); ++is)
            {
                for (auto us : Snaps_[_info.AimId_].Snaps_)
                {
                    if (is->getId() == us.getId())
                    {
                        is->PreviewSeq_ = us.PreviewSeq_;
                        is->PreviewUri_ = us.PreviewUri_;
                        is->FullPreview_ = us.FullPreview_;
                        is->MiniPreview_ = us.MiniPreview_;
                    }
                }
            }

            Snaps_[_info.AimId_] = _info;
        }
        else
        {
            for (auto s : _info.Snaps_)
            {
                snapsUrls << s.Url_;
            }
            Snaps_[_info.AimId_] = _info;
        }
    }

    if (haveSnaps)
    {
        qSort(Snaps_[_info.AimId_].Snaps_.begin(), Snaps_[_info.AimId_].Snaps_.end(), [](const SnapInfo& first, const SnapInfo& second) { return first.Timestamp_ < second.Timestamp_; });
        calcUserLastSnaps(_info.AimId_);
    }

    return !snapsUrls.isEmpty();
}

QPixmap SnapStorage::preparePreview(const QPixmap& _preview)
{
    auto previewSize = Logic::SnapItemDelegate::getSnapPreviewItemSize();
    QPixmap p;
    if (_preview.height() > _preview.width())
    {
        p = _preview.scaledToWidth(previewSize.width(), Qt::SmoothTransformation);
    }
    else
    {
        p = _preview.scaledToHeight(previewSize.height(), Qt::SmoothTransformation);
    }

    auto r = QRect(0, 0, previewSize.width(), previewSize.height());

    QPixmap result = p.copy(r);
    
    QImage resultImg(result.size(), QImage::Format_ARGB32);
    QPainter painter(&resultImg);
    
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    QColor color1(Qt::black);
    color1.setAlpha(0.3 * 255);
    QColor color2(Qt::black);
    color2.setAlpha(0.1 * 255);
    QColor color3(Qt::black);
    color3.setAlpha(0);

    auto gHeight = Logic::SnapItemDelegate::getGradientHeight();
    auto gRect = QRect(0, resultImg.height() - gHeight, resultImg.width(), gHeight);
    QLinearGradient g(gRect.topLeft(), gRect.bottomLeft());
    g.setColorAt(0, color3);
    g.setColorAt(0.7, color2);
    g.setColorAt(1, color1);
    QPainter gPainter(&result);
    gPainter.setRenderHint(QPainter::Antialiasing);
    gPainter.setRenderHint(QPainter::SmoothPixmapTransform);
    gPainter.fillRect(gRect, g);
    
    painter.fillRect(resultImg.rect(), Qt::white);
    auto b = QBrush(result);
    painter.setBrush(b);
    painter.setPen(QPen(Qt::transparent, 0));
    painter.drawRoundedRect(resultImg.rect(), Utils::scale_value(snap_border_radius), Utils::scale_value(snap_border_radius));
    
    return QPixmap::fromImage(resultImg);
}

QPixmap SnapStorage::prepareFull(const QPixmap& _preview)
{  
    return _preview;
}

void SnapStorage::rebuildIndex(bool fromCache)
{
    Index_.clear();
    FeaturedIndex_.clear();

    auto myAimId = Ui::MyInfo()->aimId();
    for (auto s : Snaps_)
    {
        if (s.IsFriend_ || s.AimId_ == myAimId)
            Index_.push_back(s);
        else
            FeaturedIndex_.push_back(s);
    }

    std::sort(FeaturedIndex_.begin(), FeaturedIndex_.end(), [](const Logic::UserSnapsInfo& first, Logic::UserSnapsInfo& second)
    {
        return first.LastNewSnapTimestamp_  > second.LastNewSnapTimestamp_;
    });

    std::sort(Index_.begin(), Index_.end(), [myAimId](const Logic::UserSnapsInfo& first, Logic::UserSnapsInfo& second)
    {
        return first.LastSnapTimestamp_ > second.LastSnapTimestamp_;
    });

    for (auto u : Snaps_)
    {
        int32_t timestamp = -1;
        for (auto s : u.Snaps_)
        {
            if (s.Timestamp_ > timestamp)
            {
                timestamp = s.Timestamp_;
            }

            if (!s.Url_.isEmpty() && !DownloadingUrls_.contains(s.Url_))
            {
                Ui::GetDispatcher()->download_snap_metainfo(u.AimId_, Ui::ComplexMessage::extractIdFromFileSharingUri(s.Url_));
                DownloadingUrls_ << s.Url_;
            }
        }
    }

    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
    emit indexChanged();

    int rows = 0;
    if (!Index_.empty())
        ++rows;
    if (!FeaturedIndex_.empty())
        ++rows;

    setRowCount(rows);
    setColumnCount(std::max(Index_.size(), FeaturedIndex_.size()));
}

Logic::SnapItem SnapStorage::getData(int row, int col) const
{
    Logic::SnapItem item;

    if (row != getFriendsRow() && row != getFeaturedRow())
        return item;

    const auto& index = row == getFeaturedRow() ? FeaturedIndex_ : Index_;

    if (col >= index.size())
        return item;

    UserSnapsInfo u = index[col];

    item.AimId_ = u.AimId_;
    item.Friendly_ = u.Friendly_;
    item.Views_ = u.State_.Views_;

    int timestamp = 0;
    for (auto s : u.Snaps_)
    {
        if (s.getId() > u.State_.LastViewedSnapId_ || u.State_.LastViewedSnapId_ == -1)
            item.HaveNewSnap_ = true;

        if (s.Timestamp_ > timestamp)
        {
            timestamp = s.Timestamp_;
            item.Snap_ = s.MiniPreview_;
        }
    }
    return item;
}

void SnapStorage::calcUserLastSnaps(const QString& _aimid)
{
    int32_t lastSnapTimestamp = 0;
    int32_t lastNewSnapTimestamp = 0;

    for (auto s : Snaps_[_aimid].Snaps_)
    {
        if (s.Timestamp_ > lastSnapTimestamp)
            lastSnapTimestamp = s.Timestamp_;

        if ((s.getId() > Snaps_[_aimid].State_.LastViewedSnapId_ || Snaps_[_aimid].State_.LastViewedSnapId_ == -1) && s.Timestamp_ > lastNewSnapTimestamp)
            lastNewSnapTimestamp = s.Timestamp_;
    }

    Snaps_[_aimid].LastSnapTimestamp_ = lastSnapTimestamp;
    Snaps_[_aimid].LastNewSnapTimestamp_ = lastNewSnapTimestamp;
}

void SnapStorage::updateSnapImage(qint64 _snapId, QPixmap _image)
{
    for (unsigned i = 0; i < Index_.size(); ++i)
    {
        for (QList<SnapInfo>::iterator is = Index_[i].Snaps_.begin(); is != Index_[i].Snaps_.end(); ++is)
        {
            if (is->getId() == _snapId)
            {
                is->SourcePreview_ = _image;
                is->MiniPreview_ = preparePreview(_image);
                is->FullPreview_ = prepareFull(_image);
                emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
            }
        }
    }

    for (unsigned i = 0; i < FeaturedIndex_.size(); ++i)
    {
        for (QList<SnapInfo>::iterator is = FeaturedIndex_[i].Snaps_.begin(); is != FeaturedIndex_[i].Snaps_.end(); ++is)
        {
            if (is->getId() == _snapId)
            {
                is->SourcePreview_ = _image;
                is->MiniPreview_ = preparePreview(_image);
                is->FullPreview_ = prepareFull(_image);
                emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
            }
        }
    }
}

void SnapStorage::updateSnapLocalPath(const QString& _url, const QString& _local)
{
    for (unsigned i = 0; i < Index_.size(); ++i)
    {
        for (QList<SnapInfo>::iterator is = Index_[i].Snaps_.begin(); is != Index_[i].Snaps_.end(); ++is)
        {
            if (is->Url_ == _url)
            {
                is->LocalPath_ = _local;
            }
        }
    }

    for (unsigned i = 0; i < FeaturedIndex_.size(); ++i)
    {
        for (QList<SnapInfo>::iterator is = FeaturedIndex_[i].Snaps_.begin(); is != FeaturedIndex_[i].Snaps_.end(); ++is)
        {
            if (is->Url_ == _url)
            {
                is->LocalPath_ = _local;
            }
        }
    }
}

void SnapStorage::downloadSnap(const QString& _aimId, const QString& _url)
{
    Seq_.push_back(Ui::GetDispatcher()->downloadSharedFile(_url, false));
    Seq_.push_back(Ui::GetDispatcher()->downloadSharedFile(_url, false));
}

void SnapStorage::processPlaylist()
{
    QList<PlayItem>::iterator iter = PlayList_.begin();
    while (iter != PlayList_.end() && !iter->LocalPath_.isEmpty())
    {
        if (iter->User_)
            emit playUserSnap(iter->LocalPath_, iter->AimId_, iter->OriginalAimId_, iter->Url_, iter->Id_, iter->First_);
        else
            emit playSnap(iter->LocalPath_, iter->AimId_, iter->OriginalAimId_, iter->Url_, iter->Id_, iter->First_);
        iter = PlayList_.erase(iter);
    }
    
    if (!PlayList_.isEmpty())
    {
        auto item = PlayList_.begin();
        downloadSnap(item->AimId_, item->Url_);
    }
}

SnapStorage* GetSnapStorage()
{
    static std::unique_ptr<SnapStorage> storage(new SnapStorage());
    return storage.get();
}
}
