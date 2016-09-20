#pragma once

#include "../corelib/namespaces.h"

#define CORE_TOOLS_NS_BEGIN CORE_NS_BEGIN namespace tools {
#define CORE_TOOLS_NS_END } CORE_NS_END

#define CORE_WIM_NS_BEGIN CORE_NS_BEGIN namespace wim {
#define CORE_WIM_NS_END } CORE_NS_END

#define PREVIEW_PROXY_NS preview_proxy
#define CORE_WIM_PREVIEW_PROXY_NS_BEGIN CORE_WIM_NS_BEGIN namespace PREVIEW_PROXY_NS {
#define CORE_WIM_PREVIEW_PROXY_NS_END CORE_WIM_NS_END }

#define SNAPS_NS snaps
#define CORE_WIM_SNAPS_NS_BEGIN CORE_WIM_NS_BEGIN namespace SNAPS_NS {
#define CORE_WIM_SNAPS_NS_END CORE_WIM_NS_END }

#define CORE_ARCHIVE_NS_BEGIN CORE_NS_BEGIN namespace archive {
#define CORE_ARCHIVE_NS_END } CORE_NS_END

#define CORE_CONFIGURATION_NS_BEGIN CORE_NS_BEGIN namespace configuration {
#define CORE_CONFIGURATION_NS_END } CORE_NS_END

#define CORE_DISK_CACHE_NS disk_cache
#define CORE_DISK_CACHE_NS_BEGIN CORE_NS_BEGIN namespace CORE_DISK_CACHE_NS {
#define CORE_DISK_CACHE_NS_END } CORE_NS_END

#define PLATFORM_NS platform
#define PLATFORM_NS_BEGIN namespace PLATFORM_NS
#define PLATFORM_NS_END }