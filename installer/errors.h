#pragma once

namespace installer
{
	enum errorcode
	{
		ok = 0,
		open_files_archive,
		create_product_folder,
		open_file_for_write,
		write_file,
		start_exe,
		terminate_previous,
		open_registry_key,
		set_registry_value,
        get_registry_value,
		create_registry_key,
		copy_installer,
		get_special_folder,
		write_to_uninstall,
		copy_installer_to_temp,
		get_temp_path,
		get_temp_file_name,
		get_module_name,
		start_installer_from_temp,
		invalid_installer_pack,
        create_exported_account_folder,
        create_exported_settings_folder
	};

	class error
	{
		errorcode	error_;
		QString		error_text_;

	public:

		error(errorcode _error = errorcode::ok, const QString& _error_text = "")
			:	error_(_error),
				error_text_(_error_text)
		{

		}

		bool is_ok() const
		{
			return (error_ == errorcode::ok);
		}

		void show() const
		{
			std::wstringstream ss_err;
			ss_err << L"installation error, code = " << error_ << L", (" << (const wchar_t*) error_text_.utf16() << L").";

			::MessageBox(0, ss_err.str().c_str(), L"ICQ installer", MB_ICONERROR);
		}

		QString get_code_string()
		{
			return QString::number(error_);
		}
	};
}
