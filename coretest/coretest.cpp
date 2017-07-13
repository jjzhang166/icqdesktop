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
		{;
		/*	core::ifptr<core::icore_factory> factory(core_face->get_factory());
			core::coll_helper helper1(factory->create_collection(), true);
			helper1.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper1.set_value_as_int64("from", 6166903176776646656);
			helper1.set_value_as_int64("count", 300);
			core_connector->receive(L"get_history_from", 1002, helper1.get());*/

		/*	
			core::coll_helper helper(factory->create_collection(), true);
			helper.set_value_as_string("contact", "500110402");
			helper.set_value_as_string("message", "test test super test");
			core_connector->receive(L"send_message", 1001, helper.get());*/

		/*	core::ifptr<core::icore_factory> factory(core_face->get_factory());
			core::coll_helper helper(factory->create_collection(), true);
			helper.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper.set_value_as_string("file", "c:/projects/DSC_9814.JPG");
			core_connector->receive(L"files/upload", 1001, helper.get());*/

		/*	core::ifptr<core::icore_factory> factory(core_face->get_factory());
			core::coll_helper helper(factory->create_collection(), true);
			helper.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper.set_value_as_string("url", "http://files.icq.net/get/0DuDuuZtUzah1ev8R3kAOa55f0562b1ah");
			core_connector->receive(L"files/download", 1001, helper.get());*/

		/*	core::ifptr<core::icore_factory> factory(core_face->get_factory());
			core::coll_helper helper(factory->create_collection(), true);
			//helper.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper.set_value_as_string("url", "https://retina.news.mail.ru/prev670x400/pic/92/e9/image23272862_4cd9498398e6eb14c00ce1ec28fc5659.jpg");
			core_connector->receive(L"preview/download", 1001, helper.get());*/

			

		/*	core::coll_helper helper1(factory->create_collection(), true);
			helper1.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper1.set_value_as_string("file", "c:/projects/magent.exe");
			core_connector->receive(L"files/upload", 1002, helper1.get());	

			core::coll_helper helper2(factory->create_collection(), true);
			helper2.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper2.set_value_as_string("file", "c:/projects/magent_rfrtoken.exe");
			core_connector->receive(L"files/upload", 1003, helper2.get());	*/
 
		}
		else if (WM_HISTORY_RECEIVED == msg.message)
		{
			core::ifptr<core::icore_factory> factory(core_face->get_factory());
			core::coll_helper helper1(factory->create_collection(), true);
			helper1.set_value_as_string("contact", "koptyakov@corp.mail.ru");
			helper1.set_value_as_int64("id", 6167257567413141504);
			core_connector->receive("get_messages_buddy", 1002, helper1.get());
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

