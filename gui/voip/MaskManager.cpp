#include "stdafx.h"

#include "../core_dispatcher.h"
#include "../utils/gui_coll_helper.h"

#include "MaskManager.h"

namespace
{
    const int UpdateTimeoutMsec = 24 * 60 * 60 * 1000;
    const int RetryUpdateTimeoutMsec = 1 * 1000;
}

voip_masks::MaskManager::MaskManager(Ui::core_dispatcher& _dispatcher, QObject* _parent)
    : QObject(_parent)
    , dispatcher_(_dispatcher)
{
    connect(&dispatcher_, &Ui::core_dispatcher::im_created, this, &MaskManager::onImCreated, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskListLoaded, this, &MaskManager::onMaskListLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskPreviewLoaded, this, &MaskManager::onPreviewLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskModelLoaded, this, &MaskManager::onModelLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskLoaded, this, &MaskManager::onLoaded, Qt::QueuedConnection);
    connect(&dispatcher_, &Ui::core_dispatcher::maskRetryUpdate, this, &MaskManager::onRetryUpdate, Qt::QueuedConnection);

    updateTimer_ = new QTimer(this);
    updateTimer_->setTimerType(Qt::VeryCoarseTimer);
    connect(updateTimer_, &QTimer::timeout, this, &MaskManager::onUpdateTimeout);
    connect(updateTimer_, &QTimer::timeout, updateTimer_, &QTimer::stop);
}

const voip_masks::MaskList& voip_masks::MaskManager::getAvailableMasks() const
{
    return masks_;
}

void voip_masks::MaskManager::onImCreated()
{
    loadMaskList();
}

void voip_masks::MaskManager::onMaskListLoaded(QList<QString> maskList)
{
    MaskList tmp;
    for (const auto& id :maskList)
    {
        auto mask = new Mask(id, this);
        connect(&dispatcher_, &Ui::core_dispatcher::maskLoadingProgress, mask, &Mask::onLoadingProgress, Qt::QueuedConnection);
        tmp.push_back(mask);
    }
    masks_.swap(tmp);

    qDeleteAll(tmp.begin(), tmp.end());

    previewSeqList_.clear();
    maskSeqList_.clear();

    loadPreviews();
}

void voip_masks::MaskManager::onPreviewLoaded(quint64 _seq, const QString& _localPath)
{
    const auto it = previewSeqList_.find(_seq);
    if (it != previewSeqList_.end())
    {
        Mask* mask = *it;
        mask->setPreview(_localPath);
        previewSeqList_.erase(it);
    }

    if (previewSeqList_.empty())
    {
        loadModel();
    }
}

void voip_masks::MaskManager::onModelLoaded()
{
    loadMasks();

    updateTimer_->start(UpdateTimeoutMsec);
}

void voip_masks::MaskManager::onLoaded(quint64 _seq, const QString& _localPath)
{
    const auto it = maskSeqList_.find(_seq);
    if (it != maskSeqList_.end())
    {
        Mask* mask = *it;
        mask->setJsonPath(_localPath);
        maskSeqList_.erase(it);
    }
}

void voip_masks::MaskManager::onUpdateTimeout()
{
    loadMaskList();
}

void voip_masks::MaskManager::onRetryUpdate()
{
    qDeleteAll(masks_.begin(), masks_.end());
    masks_.clear();

    previewSeqList_.clear();
    maskSeqList_.clear();

    updateTimer_->start(RetryUpdateTimeoutMsec);
}

void voip_masks::MaskManager::loadMaskList()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    dispatcher_.post_message_to_core("masks/get_id_list", collection.get());
}

void voip_masks::MaskManager::loadPreviews()
{
    for (auto mask : masks_)
    {
        const auto seq = postMessageToCore("masks/preview/get", mask->id());
        previewSeqList_[seq] = mask;
    }
}

void voip_masks::MaskManager::loadModel()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    dispatcher_.post_message_to_core("masks/model/get", collection.get());
}

void voip_masks::MaskManager::loadMasks()
{
    for (auto mask : masks_)
    {
        const auto seq = postMessageToCore("masks/get", mask->id());
        mask->setProgressSeq(seq);
        maskSeqList_[seq] = mask;
    }
}

quint64 voip_masks::MaskManager::postMessageToCore(const QString& _message, const QString& _maskId) const
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_qstring("mask_id", _maskId);
    return dispatcher_.post_message_to_core(_message, collection.get());
}
