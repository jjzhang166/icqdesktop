#include "stdafx.h"
#include "loader_task.h"
#include "../wim_packet.h"

using namespace core;
using namespace wim;


loader_task::loader_task(const std::string& _id, const wim_packet_params& _params)
    :   id_(_id),
        wim_params_(new wim_packet_params(_params)),
        error_(0)
{
    assert(!id_.empty());
}


loader_task::~loader_task()
{
}

const wim_packet_params& loader_task::get_wim_params()
{
    return *wim_params_;
}

const std::string& loader_task::get_id() const
{
    assert(!id_.empty());

    return id_;
}

void loader_task::set_last_error(int32_t _error)
{
    error_ = _error;
}

int32_t loader_task::get_last_error() const
{
    return error_;
}

void loader_task::resume(loader& _loader)
{
    assert(false);
}