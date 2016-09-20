#pragma once

#include "../../../namespaces.h"

CORE_NS_BEGIN

enum class file_sharing_content_type;

CORE_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type extractContentTypeFromFileSharingId(const QString &id);

int32_t extractDurationFromFileSharingId(const QString &id);

QSize extractSizeFromFileSharingId(const QString &id);

QString extractIdFromFileSharingUri(const QString &uri);

UI_COMPLEX_MESSAGE_NS_END