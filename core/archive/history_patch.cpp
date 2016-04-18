#include "stdafx.h"

#include "history_patch.h"

CORE_ARCHIVE_NS_BEGIN

    history_patch::history_patch(const type _type, const int64_t _archive_id)
    : type_(_type)
    , archive_id_(_archive_id)
{
    assert(type_ > history_patch::type::min);
    assert(type_ < history_patch::type::max);
    assert(archive_id_ > 0);
}

int64_t history_patch::get_message_id() const
{
    assert(archive_id_ > 0);

    return archive_id_;
}

history_patch::type history_patch::get_type() const
{
    assert(type_ > type::min);
    assert(type_ < type::max);

    return type_;
}

history_patch_uptr history_patch::make_deleted(const int64_t _archive_id)
{
    assert(_archive_id > 0);

    return history_patch_uptr(
        new history_patch(type::deleted, _archive_id)
        );
}

history_patch_uptr history_patch::make_modified(const int64_t _archive_id)
{
    assert(_archive_id > 0);

    return history_patch_uptr(
        new history_patch(type::modified, _archive_id)
        );
}

CORE_ARCHIVE_NS_END