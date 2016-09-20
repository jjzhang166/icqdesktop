#pragma once

#include "../../../namespaces.h"

CORE_WIM_NS_BEGIN

enum class tasks_runner_slot
{
    min,

    generic,
    images,
    metainfo_a,
    metainfo_b,
    snap_metainfo_a,
    snap_metainfo_b,
    previews_a,
    previews_b,
    previews_c,

    max
};

typedef std::vector<tasks_runner_slot> tasks_runner_slots;

std::ostream& operator<<(std::ostream &oss, const tasks_runner_slot arg);

const tasks_runner_slots& get_all_tasks_runner_slots();

CORE_WIM_NS_END

namespace std
{
    template<>
    struct hash<core::wim::tasks_runner_slot>
    {
        std::size_t operator()(const core::wim::tasks_runner_slot &v) const
        {
            return (std::size_t)v;
        }
    };
}