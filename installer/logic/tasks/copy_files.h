#pragma once

namespace installer
{
    namespace logic
    {
        installer::error copy_files();
        installer::error copy_files_to_updates();
        installer::error copy_files_from_updates();
        installer::error delete_files();
        installer::error delete_self_and_product_folder();
        installer::error delete_self_from_8x();
        installer::error delete_updates();
        installer::error copy_self_from_updates();
        installer::error copy_self_to_icqim_8x();
    }
}
