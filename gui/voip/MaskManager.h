#pragma once

#include "Masks.h"

namespace Ui
{
    class core_dispatcher;
}

namespace voip_masks
{
    class MaskManager
        : public QObject
    {
        Q_OBJECT
    public:
        explicit MaskManager(Ui::core_dispatcher& _dispatcher, QObject* _parent);

        const MaskList& getAvailableMasks() const;

    private slots:
        void onImCreated();
        void onMaskListLoaded(QList<QString> maskList);
        void onPreviewLoaded(quint64 _seq, const QString& _localPath);
        void onModelLoaded();
        void onLoaded(quint64 _seq, const QString& _localPath);
        void onUpdateTimeout();
        void onRetryUpdate();

    private:
        void loadMaskList();
        void loadPreviews();
        void loadModel();
        void loadMasks();

        quint64 postMessageToCore(const QString& _message, const QString& _maskId) const;

    private:
        Ui::core_dispatcher& dispatcher_;
        MaskList masks_;

        QHash<quint64, Mask*> previewSeqList_;
        QHash<quint64, Mask*> maskSeqList_;

        QTimer* updateTimer_;
    };
}
