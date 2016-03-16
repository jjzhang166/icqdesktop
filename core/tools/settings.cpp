#include "stdafx.h"
#include "settings.h"

using namespace core;
using namespace tools;

settings::settings()
{
}


settings::~settings()
{
}

void core::tools::settings::serialize(binary_stream& bstream) const
{
	tlvpack pack;

	for (auto iter = values_.begin(); iter != values_.end(); iter++)
		pack.push_child(iter->second);

	pack.serialize(bstream);
}

bool core::tools::settings::unserialize(binary_stream& bstream)
{
	tlvpack pack;
	if (!pack.unserialize(bstream))
		return false;

	auto val = pack.get_first();
	while (val)
	{
		values_[val->get_type()] = val;
		val = pack.get_next();
	}

	return true;
}

