#include "stdafx.h"
#include "chat_params.h"

using namespace core;
using namespace wim;

chat_params::chat_params()
{
    //
}

chat_params::~chat_params()
{
    //
}

void chat_params::set_name(const std::string& _name)
{
    name_.reset(_name);
}

void chat_params::set_avatar(const std::string& _avatar)
{
    avatar_.reset(_avatar);
}

void chat_params::set_about(const std::string& _about)
{
    about_.reset(_about);
}

void chat_params::set_public(bool _public)
{
    public_.reset(_public);
}

void chat_params::set_join(bool _approved)
{
    approved_.reset(_approved);
}

void chat_params::set_joiningByLink(bool _joiningByLink)
{
    joiningByLink_.reset(_joiningByLink);
}

void chat_params::set_readOnly(bool _readOnly)
{
    readOnly_.reset(_readOnly);
}

void chat_params::set_ageGate(bool _ageGate)
{
    ageGate_.reset(_ageGate);
}

chat_params *chat_params::create(core::coll_helper _params)
{
    auto result = new (std::nothrow) chat_params();
    if (_params.is_value_exist("name"))
        result->set_name(_params.get_value_as_string("name"));
    if (_params.is_value_exist("avatar"))
        result->set_avatar(_params.get_value_as_string("avatar"));
    if (_params.is_value_exist("about"))
        result->set_about(_params.get_value_as_string("about"));
    if (_params.is_value_exist("public"))
        result->set_public(_params.get_value_as_bool("public"));
    if (_params.is_value_exist("approved"))
        result->set_join(_params.get_value_as_bool("approved"));
    if (_params.is_value_exist("link"))
        result->set_joiningByLink(_params.get_value_as_bool("link"));
    if (_params.is_value_exist("ro"))
        result->set_readOnly(_params.get_value_as_bool("ro"));
    if (_params.is_value_exist("age"))
        result->set_ageGate(_params.get_value_as_bool("age"));
    return result;
}
