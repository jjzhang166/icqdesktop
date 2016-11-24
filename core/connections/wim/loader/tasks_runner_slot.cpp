#include "stdafx.h"

#include "tasks_runner_slot.h"

CORE_WIM_NS_BEGIN

std::ostream& operator<<(std::ostream &oss, const tasks_runner_slot arg)
{
    assert(arg > tasks_runner_slot::min);
    assert(arg < tasks_runner_slot::max);

    switch(arg)
    {
        case tasks_runner_slot::generic: return (oss << "generic");
        case tasks_runner_slot::images_a: return (oss << "images/a");
        case tasks_runner_slot::images_b: return (oss << "images/b");
        case tasks_runner_slot::images_c: return (oss << "images/c");
        case tasks_runner_slot::metainfo_a: return (oss << "metainfo/a");
        case tasks_runner_slot::metainfo_b: return (oss << "metainfo/b");
        case tasks_runner_slot::snap_metainfo_a: return (oss << "snap_metainfo/a");
        case tasks_runner_slot::snap_metainfo_b: return (oss << "snap_metainfo/b");
        case tasks_runner_slot::previews_a: return (oss << "previews/a");
        case tasks_runner_slot::previews_b: return (oss << "previews/b");
        case tasks_runner_slot::previews_c: return (oss << "previews/c");
        default:
            assert(!"unknown slot");
    }

    return oss;
}

const tasks_runner_slots& get_all_tasks_runner_slots()
{
    static tasks_runner_slots slots;

    if (slots.empty())
    {
        slots.reserve(16);
        slots.emplace_back(tasks_runner_slot::generic);
        slots.emplace_back(tasks_runner_slot::images_a);
        slots.emplace_back(tasks_runner_slot::images_b);
        slots.emplace_back(tasks_runner_slot::images_c);
        slots.emplace_back(tasks_runner_slot::metainfo_a);
        slots.emplace_back(tasks_runner_slot::metainfo_b);
        slots.emplace_back(tasks_runner_slot::snap_metainfo_a);
        slots.emplace_back(tasks_runner_slot::snap_metainfo_b);
        slots.emplace_back(tasks_runner_slot::previews_a);
        slots.emplace_back(tasks_runner_slot::previews_b);
        slots.emplace_back(tasks_runner_slot::previews_c);
    }

    return slots;
}

CORE_WIM_NS_END