#pragma once

namespace installer
{
	namespace logic
	{
        installer::error shutdown_8x();
		installer::error export_from_8x_and_uninstall();
        installer::error store_exported_account();
        installer::error store_exported_settings();
    }
}
