#pragma once

//#ifdef _WIN32
#define crossprocess_mutex_name L"{DB17D844-6D3A-4039-968B-0B1D10B8AA81}"
#define crossprocess_pipe_name "{4FA385C7-06F7-4C20-A23E-6B0AB2C02FD3}"
#define crossprocess_pipe_name_postfix ".client"
#define crossprocess_message_get_process_id "get_process_id"
#define crossprocess_message_get_hwnd_activate "get_hwnd_and_activate"
#define crossprocess_message_shutdown_process "shutdown_process"
#define crossprocess_message_execute_url_command "execute_url_command"

#define application_user_model_id L"ICQ.Client"

#define url_command_join_livechat "chat"
#define url_command_open_profile "people"
#define url_command_app "app"
#define url_command_livechats_home "livechats"
#define url_command_show_public_livechats "show_public_livechats"


//#endif