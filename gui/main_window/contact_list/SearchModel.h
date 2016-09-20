#pragma once

#include "ContactItem.h"
#include "../../types/contact.h"
#include "AbstractSearchModel.h"

namespace Logic
{
    class SearchModel : public AbstractSearchModel
    {
        Q_OBJECT
    Q_SIGNALS:
        void results();

    public Q_SLOTS:
        void searchPatternChanged(QString) override;
        void searchResult(QStringList);

    private Q_SLOTS:
        void avatarLoaded(QString);
        void contactRemoved(QString _contact);

    public:
        explicit SearchModel(QObject* _parent);

        int rowCount(const QModelIndex& _parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& _index, int _role) const override;
        Qt::ItemFlags flags(const QModelIndex& _index) const override;
        void setFocus() override;
        const QStringList& getPattern() const;
        void emitChanged(int _first, int _last) override;

    private:
        bool less(const ContactItem& _first, const ContactItem& _second);

        std::vector<ContactItem> match_;
        QStringList searchPatterns_;
        bool searchRequested_;
    };

    SearchModel* getSearchModel();
}