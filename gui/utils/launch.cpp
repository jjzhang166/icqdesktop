#include "stdafx.h"
#include "launch.h"
#include "application.h"
#include "../types/message.h"
#include "../types/contact.h"
#include "../types/chat.h"
#include "../main_window/history_control/MessagesModel.h"

#ifdef _WIN32
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QICOPlugin);
Q_IMPORT_PLUGIN(QWindowsAudioPlugin);
#endif //_WIN32

#ifdef __APPLE__
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
#endif

int launch::main(int argc, char *argv[])
{
	Utils::Application app(argc, argv);

	if (!app.isMainInstance())
	{
		app.switchInstance();
		return 0;
	}

    if (app.updating())
        return 0;

	if (app.init())
	{
		qRegisterMetaType<std::shared_ptr<Data::ContactList>>("std::shared_ptr<Data::ContactList>");
		qRegisterMetaType<std::shared_ptr<Data::MessageBuddies>>("std::shared_ptr<Data::MessageBuddies>");
		qRegisterMetaType<std::shared_ptr<Data::ChatInfo>>("std::shared_ptr<Data::ChatInfo>");
		qRegisterMetaType<Data::DlgState>("Data::DlgState");
		qRegisterMetaType<Logic::MessageKey>("Logic::MessageKey");
		qRegisterMetaType<QList<Logic::MessageKey>>("QList<Logic::MessageKey>");
		qRegisterMetaType<QScroller::State>("QScroller::State");
		qRegisterMetaType<Ui::MessagesBuddiesOpt>("Ui::MessagesBuddiesOpt");
		qRegisterMetaType<QSystemTrayIcon::ActivationReason>("QSystemTrayIcon::ActivationReason");
	}
	else
	{
		return 1;
	}

    QT_TRANSLATE_NOOP("QWidgetTextControl", "&Undo");
    QT_TRANSLATE_NOOP("QWidgetTextControl", "&Redo");
    QT_TRANSLATE_NOOP("QWidgetTextControl", "Cu&t");
    QT_TRANSLATE_NOOP("QWidgetTextControl", "&Copy");
    QT_TRANSLATE_NOOP("QWidgetTextControl", "&Paste");
    QT_TRANSLATE_NOOP("QWidgetTextControl", "Delete");
    QT_TRANSLATE_NOOP("QWidgetTextControl", "Select All");

    return app.exec();
}