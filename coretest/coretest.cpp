// coretest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "../corelib/core_face.h"
#include "../corelib/collection_helper.h"

#include <atomic>
#include <windows.h>
#include <thread>  

void gui_thread_func();

std::thread gui_thread(gui_thread_func);

#define WM_CONTACTLIST_RECEIVED		WM_USER + 1
#define WM_HISTORY_RECEIVED		WM_USER + 2

class gui_connector : public core::iconnector
{
	std::atomic<int>	ref_count_;

	// ibase interface
	virtual int addref() override
	{
		return ++ref_count_;
	}

	virtual int release() override
	{
		if (0 == (--ref_count_))
			delete this;

		return ref_count_;
	}

	// iconnector interface
	virtual void link(iconnector*, const common::core_gui_settings&) override
	{

	}
	virtual void unlink() override
	{

	}

	virtual void receive(const char* _message, int64_t _seq, core::icollection* _collection) override
	{
		wprintf(L"receive message = %s\r\n", _message);

		std::string message(_message);

		if (message == "contactlist")
			::PostThreadMessage(::GetThreadId(gui_thread.native_handle()), WM_CONTACTLIST_RECEIVED, 0, 0);
		else if (message == "get_history_result")
			::PostThreadMessage(::GetThreadId(gui_thread.native_handle()), WM_HISTORY_RECEIVED, 0, 0);

	}

public:
	gui_connector() : ref_count_(1) {}
};


void gui_thread_func()
{
	auto corelib = ::LoadLibrary(L"corelib.dll");
	if (!corelib)
		return;

	typedef bool (*get_core_instance_function)(core::icore_interface**);

	get_core_instance_function get_core_instance = (get_core_instance_function) ::GetProcAddress(corelib, "get_core_instance");
	core::icore_interface* core_face = nullptr;
	if (!get_core_instance(&core_face))
		return;

	auto core_connector = core_face->get_core_connector();
	if (!core_connector)
		return;

	gui_connector connector;
	core_connector->link(&connector, common::core_gui_settings());
		
	MSG msg = {0};
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (WM_CONTACTLIST_RECEIVED == msg.message)
		{
		}
	}

	core_connector->unlink();
	core_connector->release();
	core_face->release();

	::FreeLibrary(corelib);
}


int _tmain(int argc, _TCHAR* argv[])
{
	getwchar();

	::PostThreadMessage(::GetThreadId(gui_thread.native_handle()), WM_QUIT, 0, 0);
		
	gui_thread.join();

	return 0;
}

