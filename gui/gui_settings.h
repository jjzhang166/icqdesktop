#pragma once

#include "../corelib/collection_helper.h"

namespace Ui
{
    const int period_for_stats_settings_ms = 1e3 * 60 * 24;

    class qt_gui_settings : public QObject
    {
        Q_OBJECT

Q_SIGNALS:

        void received();
        void changed(QString);

    private:

        struct settings_value
        {
            std::vector<char> data_;
        };

        std::map<QString, settings_value>   values_;

        void post_value_to_core(const QString& _name, const settings_value& _val) const;

        void set_value_simple_data(const QString& _name, const char* _data, int _len, bool _post_to_core = true)
        {
            auto& val = values_[_name];
            if (!_len)
            {
                val.data_.clear();
                return;
            }

            val.data_.resize(_len);
            memcpy(&val.data_[0], _data, _len);

            if (_post_to_core)
                post_value_to_core(_name, val);
            
            emit changed(_name);
        }

        template <class t_>
        void set_value_simple(const QString& _name, const t_& _value)
        {
            set_value_simple_data(_name, (const char*) &_value, sizeof(_value));
        }

        bool get_value_simple_data(const QString& _name, std::vector<char>& _data) const
        {
            auto iter = values_.find(_name);
            if (iter == values_.end())
                return false;

            _data = iter->second.data_;
            return true;
        }

        template <class t_>
        t_ get_value_simple(const QString& _name, const t_& _default_value) const
        {
            std::vector<char> data;
            if (!get_value_simple_data(_name, data))
                return _default_value;

            if (data.size() != sizeof(t_))
            {
                assert(false);
                return _default_value;
            }

            t_ val;
            ::memcpy(&val, &data[0], sizeof(t_));

            return val;
        }

    public:

        template <class t_>
        void set_value(const QString& _name, const t_& _value)
        {
            assert(false); 
        }

        template <class t_>
        t_ get_value(const QString& _name, const t_& _default_value) const
        {
            assert(false);
            return _default_value;
        }
        
        bool contains_value(const QString& _name) const;
                
    private:

        int							shadow_width_;
    public:

        qt_gui_settings();
                
        void unserialize(core::coll_helper _collection);

        void set_shadow_width(int _width);
        int get_shadow_width() const;
        int get_current_shadow_width() const;
    };

    template<> void qt_gui_settings::set_value<QString>(const QString& _name, const QString& _value);
    template<> QString qt_gui_settings::get_value<QString>(const QString& _name, const QString& _default_value) const;
    template<> void qt_gui_settings::set_value<int>(const QString& _name, const int& _value);
    template <> int qt_gui_settings::get_value<int>(const QString& _name, const int& _default_value) const;
    template<> void qt_gui_settings::set_value<double>(const QString& _name, const double& _value);
    template <> double qt_gui_settings::get_value<double>(const QString& _name, const double& _default_value) const;
    template<> void qt_gui_settings::set_value<bool>(const QString& _name, const bool& _value);
    template <> bool qt_gui_settings::get_value<bool>(const QString& _name, const bool& _default_value) const;
    template<> void qt_gui_settings::set_value<std::vector<int32_t>>(const QString& _name, const std::vector<int32_t>& _value);
    template<> std::vector<int32_t> qt_gui_settings::get_value<std::vector<int32_t>>(const QString& _name, const std::vector<int32_t>& _default_value) const;
    template<> void qt_gui_settings::set_value<QRect>(const QString& _name, const QRect& _value);
    template<> QRect qt_gui_settings::get_value<QRect>(const QString& _name, const QRect& _default_value) const;
    
    qt_gui_settings* get_gui_settings();

    const std::string get_account_setting_name(const std::string& setting_name);
}