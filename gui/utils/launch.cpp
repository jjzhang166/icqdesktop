#include "stdafx.h"
#include "launch.h"
#include "application.h"
#include "../types/message.h"
#include "../types/contact.h"
#include "../types/chat.h"
#include "../types/typing.h"
#include "../main_window/history_control/MessagesModel.h"

#ifdef _WIN32
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
Q_IMPORT_PLUGIN(QICOPlugin);
Q_IMPORT_PLUGIN(QWindowsAudioPlugin);
#endif //_WIN32

#ifndef __linux__
Q_IMPORT_PLUGIN(QTiffPlugin);
#endif //__linux__

#ifdef __APPLE__
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
#endif

const QString urlCommand = "-urlcommand";

launch::CommandLineParser::CommandLineParser(int _argc, char* _argv[])
{
    isUlrCommand_ = false;

    if (_argc <= 0)
    {
        return;
    }

    executable_ = _argv[0];

    for (int i = 1; i < _argc; ++i)
    {
        if (_argv[i] == urlCommand)
        {
            isUlrCommand_ = true;

            ++i;

            if (i >= _argc)
                break;

            urlCommand_ = _argv[i];
        }
    }
}

launch::CommandLineParser::~CommandLineParser()
{

}

bool launch::CommandLineParser::isUrlCommand() const
{
    return isUlrCommand_;
}

const QString& launch::CommandLineParser::getUrlCommand() const
{
    return urlCommand_;
}

const QString& launch::CommandLineParser::getExecutable() const 
{
    return executable_;
}

int launch::main(int _argc, char* _argv[])
{
    Utils::Application app(_argc, _argv);
    
    CommandLineParser cmd_parser(_argc, _argv);

    if (!app.isMainInstance())
    {
        app.switchInstance(cmd_parser);

        return 0;
    }

    if (app.updating())
    {
        return 0;
    }

    if (app.init())
    {
        qRegisterMetaType<std::shared_ptr<Data::ContactList>>("std::shared_ptr<Data::ContactList>");
        qRegisterMetaType<std::shared_ptr<Data::MessageBuddies>>("std::shared_ptr<Data::MessageBuddies>");
        qRegisterMetaType<std::shared_ptr<Data::ChatInfo>>("std::shared_ptr<Data::ChatInfo>");
        qRegisterMetaType<QList<Data::ChatInfo>>("QList<Data::ChatInfo>");
        qRegisterMetaType<QList<Data::ChatMemberInfo>>("QList<Data::ChatMemberInfo>");
        qRegisterMetaType<Data::DlgState>("Data::DlgState");
        qRegisterMetaType<Logic::MessageKey>("Logic::MessageKey");
        qRegisterMetaType<QList<Logic::MessageKey>>("QList<Logic::MessageKey>");
        qRegisterMetaType<QScroller::State>("QScroller::State");
        qRegisterMetaType<Ui::MessagesBuddiesOpt>("Ui::MessagesBuddiesOpt");
        qRegisterMetaType<QSystemTrayIcon::ActivationReason>("QSystemTrayIcon::ActivationReason");
        qRegisterMetaType<Logic::TypingFires>("Logic::TypingFires");
        qRegisterMetaType<int64_t>("int64_t");
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