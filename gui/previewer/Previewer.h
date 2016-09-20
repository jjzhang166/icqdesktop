#pragma once

namespace core
{
    enum class file_sharing_content_type;
}

namespace Previewer
{

    void ShowMedia(const core::file_sharing_content_type _contentType, const QString& _path);
    void CloseMedia();

}