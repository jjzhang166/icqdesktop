#include "stdafx.h"
#include "core_settings.h"

#include "tools/system.h"


using namespace core;

core_settings::core_settings(const boost::filesystem::wpath& _path)
	:	file_name_(_path.wstring())
{
}


core_settings::~core_settings()
{
}

void core_settings::init_default()
{
	set_value<std::string>(core_settings_values::csv_device_id, core::tools::system::generate_guid());
}

bool core_settings::save()
{
	core::tools::binary_stream bstream;
	serialize(bstream);
	return bstream.save_2_file(file_name_);
}

bool core_settings::load()
{
	core::tools::binary_stream bstream;
	if (!bstream.load_from_file(file_name_))
		return false;

	return unserialize(bstream);
}