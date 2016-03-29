#include "stdafx.h"
#include "ThemesModel.h"
#include "ThemeWidget.h"
#include "../../../utils/utils.h"
#include "../.././../core_dispatcher.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../../theme_settings.h"
#include "../../../../common.shared/themes_constants.h"
#include "ThemesWidget.h"
#include "../../../utils/log/log.h"
#include "../../../cache/themes/themes.h"

namespace Ui
{
    ThemesModel::ThemesModel(ThemesWidget* _themes_widget) : QObject(_themes_widget), themes_widget_(_themes_widget), target_contact_("")
    {
        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        int themes_scale = (int)ThemesScale100;
        if (Utils::is_mac_retina())
        {
            themes_scale = ThemesScaleRetina;
        }
        else
        {
            double scale_coefficient = Utils::get_scale_coefficient();
            if (scale_coefficient == 1.0)
            {
                themes_scale = ThemesScale100;
            }
            else if (scale_coefficient == 1.25)
            {
                themes_scale = ThemesScale125;
            }
            else if (scale_coefficient == 1.5)
            {
                themes_scale = ThemesScale150;
            }
            else if (scale_coefficient == 2.0)
            {
                themes_scale = ThemesScale200;
            }
        }
        collection.set_value_as_int("themes_scale", themes_scale);
        GetDispatcher()->post_message_to_core("themes/meta/get", collection.get());
        connect(GetDispatcher(), SIGNAL(on_themes_meta()), this, SLOT(on_themes_meta()));
    }
    
    void ThemesModel::on_themes_meta()
    {
        themes::themes_dict themes_dict = themes::loaded_themes();
        
        std::vector<themes::themePtr> themesVector;
    
        for (auto it = themes_dict.begin(); it != themes_dict.end(); ++it)
        {
            auto theme = it->second;
            if (theme)
            {
                themesVector.push_back(theme);
            }
        }
        std::sort(themesVector.begin(), themesVector.end(),[](themes::themePtr a, themes::themePtr b){ return a->get_position() < b->get_position(); });
        
        for (auto it = themesVector.begin(); it != themesVector.end(); ++it)
        {
            auto theme = *it;
            themes_widget_->onThemeGot(theme);
        }
    }
    
    void ThemesModel::themeSelected(int _theme_id)
    {
        get_qt_theme_settings()->themeSelected(_theme_id, target_contact_);
    }
    
    void ThemesModel::set_target_contact(QString _aimId)
    {
        target_contact_ = _aimId;
    }
}
