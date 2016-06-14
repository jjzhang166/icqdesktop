#include "stdafx.h"

#include "../cache/countries.h"
#include "../gui_settings.h"
#include "../utils/gui_coll_helper.h"
#include "../core_dispatcher.h"

#ifdef __APPLE__
#include "mac_support.h"
#endif

namespace Utils
{
	void Translator::init()
	{
        QString lang = getLang();
		Ui::get_gui_settings()->set_value<QString>(settings_language, lang);

		QLocale::setDefault(QLocale(lang));
		static QTranslator translator;
		translator.load(lang, ":/translations");
		QApplication::installTranslator(&translator);
        updateLocale();
	}

    void Translator::updateLocale()
    {
        QLocale locale(getLang());
        QString localeStr = locale.name();
        localeStr.replace("_", "-");

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("locale", localeStr.toLower());
        Ui::GetDispatcher()->post_message_to_core("set_locale", collection.get());
    }

	
	QString Translator::getCurrentPhoneCode()
	{
		Ui::countries::countries_list cntrs = Ui::countries::get();

		QString result = "+";

#ifdef __APPLE__
        QString searchedCounry = MacSupport::currentRegion().right(2).toLower();;
#else
        QString searchedCounry = QLocale::system().name().right(2).toLower();
#endif

		for (const auto& country : cntrs)
		{
			if (country.code_ == searchedCounry)
			{
				result += QVariant(country.phone_code_).toString();
				break;
			}
		}

		return result;
	}

    QString Translator::getCurrentLang()
    {
        return getLang();
    }

	QString Translator::getLang()
	{
		return Ui::get_gui_settings()->get_value<QString>(settings_language, translate::translator_base::getLang());
	}

	Translator* GetTranslator()
	{
		static std::unique_ptr<Translator> translator(new Translator());
		return translator.get();
	}
}