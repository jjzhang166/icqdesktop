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

	public:
		explicit SearchModel(QObject *parent);

		int rowCount(const QModelIndex &parent = QModelIndex()) const override;
		QVariant data(const QModelIndex &index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex &index) const override;
		void setFocus() override;
		const QStringList& GetPattern() const;
		void emitChanged(int first, int last) override;
	private:
		std::vector<ContactItem> Match_;
		QStringList SearchPatterns_;
		bool SearchRequested_;
	};

	SearchModel* GetSearchModel();
}