#pragma once

#include <QtNetwork/QLocalServer>

namespace Ui
{
    class MainWindow;
};

class LocalPeer : public QObject
{
    Q_OBJECT

public:
    LocalPeer(QObject *parent = 0, bool other = false);

	void set_main_window(Ui::MainWindow* _wnd);

    unsigned int get_hwnd_and_activate();

    void listen();
	void wait();
    
protected Q_SLOTS:

    void receiveConnection();

protected:

    QString socket_name_;
    QLocalServer* server_;
    Ui::MainWindow* wnd_;
	bool other_;
};
