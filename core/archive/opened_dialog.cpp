#include "stdafx.h"
#include "opened_dialog.h"

using namespace core;
using namespace archive;

opened_dialog::opened_dialog()
	: first_msg_id_(-1)
{
}

void opened_dialog::set_first_msg_id(int64_t _id)
{
	first_msg_id_ = _id;
}

int64_t opened_dialog::get_first_msg_id() const
{
	return first_msg_id_;
}