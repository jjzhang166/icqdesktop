#pragma once

#include "../corelib/collection_helper.h"

namespace Ui
{
	class my_info : public QObject
	{
		Q_OBJECT

	Q_SIGNALS:
		void received();

	private:
		QString aimId_;
		QString displayId_;
		QString friendlyName_;
		QString state_;
		QString userType_;
		QString phoneNumber_;
		uint32_t flags_;

	public:
		my_info();
		void unserialize(core::coll_helper* _collection);

		QString aimId() const { return aimId_; };
		QString displayId() const { return displayId_; };
		QString friendlyName() const {return friendlyName_; };
		QString state() const { return state_; };
		QString userType() const { return userType_; };
		QString phoneNumber() const { return phoneNumber_; };
		uint32_t flags() const { return flags_; };
	};

	my_info* MyInfo();
}