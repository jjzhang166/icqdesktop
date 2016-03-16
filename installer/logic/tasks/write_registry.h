#pragma once

namespace installer
{
	namespace logic
	{
		installer::error write_registry();
		installer::error clear_registry();
        installer::error write_update_version();
        installer::error write_to_uninstall_key();
	}
}
