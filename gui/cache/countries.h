#pragma once

namespace Ui
{
	namespace countries
	{
		struct country
		{
			const int		phone_code_;
			const int		region_;
			const int		type_;
			const QString	name_;
			const QString	code_;
			
			country(
				int _phone_code,
				int _region,
				int _type,
				const QString& _name, 
				const QString& _code)
				:
				phone_code_(_phone_code),
				region_(_region),
				type_(_type),
				name_(_name),
				code_(_code)
			{

			}
		};

		typedef std::vector<country> countries_list;

		const countries_list& get();
	}
		
}


