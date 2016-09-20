#include "stdafx.h"
#include "messages_data.h"
#include "storage.h"
#include "archive_index.h"

using namespace core;
using namespace archive;

messages_data::messages_data(const std::wstring& _file_name)
    :	storage_(new storage(_file_name))
{
}


messages_data::~messages_data()
{
}


bool messages_data::get_messages(headers_list& _headers, history_block& _messages) const
{
    auto p_storage = storage_.get();
    archive::storage_mode mode;
    mode.flags_.read_ = mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return false;
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    _messages.reserve(_headers.size());

    bool res = true;

    core::tools::binary_stream message_data;

    for (auto iter_header = _headers.begin(); iter_header != _headers.end(); iter_header++)
    {
        message_data.reset();

        const auto &header = *iter_header;
        assert(!header.is_patch());

        if (!storage_->read_data_block(header.get_data_offset(), message_data))
        {
            assert(!"invalid message data");
            res = false;
            continue;
        }

        auto msg = std::make_shared<history_message>();
        if (msg->unserialize(message_data) != 0)
        {
            assert(!"unserialize message error");
            continue;
        }

        if (msg->get_msgid() != header.get_id())
        {
            assert(!"message data invalid");
            continue;
        }

        msg->apply_header_flags(header);

        const auto modifications = get_message_modifications(header);
        msg->apply_modifications(modifications);

        _messages.push_back(msg);
    }

    return res;
}

history_block messages_data::get_message_modifications(const message_header& _header) const
{
    if (!_header.is_modified())
    {
        return history_block();
    }

    history_block modifications;

    core::tools::binary_stream message_data;

    const auto &modification_headers = _header.get_modifications();
    for (const auto &header : modification_headers)
    {
        if (!storage_->read_data_block(header.get_data_offset(), message_data))
        {
            assert(!"invalid modification data");
            continue;
        }

        auto modification = std::make_shared<history_message>();
        if (modification->unserialize(message_data) != 0)
        {
            assert(!"unserialize modification error");
            continue;
        }

        if (modification->get_msgid() != header.get_id())
        {
            assert(!"modification data invalid");
            continue;
        }

        modifications.push_back(modification);
    }

    return modifications;
}

bool messages_data::update(const archive::history_block& _data)
{
    auto p_storage = storage_.get();
    archive::storage_mode mode;
    mode.flags_.write_ = mode.flags_.append_ = true;
    if (!storage_->open(mode))
        return false;
    core::tools::auto_scope lb([p_storage]{p_storage->close();});

    core::tools::binary_stream message_data;

    for (auto iter_hm = _data.begin(); iter_hm != _data.end(); iter_hm++)
    {
        auto msg = *iter_hm;

        message_data.reset();
        msg->serialize(message_data);

        int64_t offset = 0;
        if (!storage_->write_data_block(message_data, offset))
            return false;

        msg->set_data_offset(offset);
        msg->set_data_size(message_data.available());
    }

    return true;
}