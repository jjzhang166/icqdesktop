#pragma once

#include "../../gui.shared/translator_base.h"

namespace Utils
{
	class Translator : public translate::translator_base
	{
	public:
		virtual void init() override;
		QString getCurrentPhoneCode();
        QString getCurrentLang();

        void updateLocale();
		
	private:

		virtual QString getLang() override;
	};

	Translator* GetTranslator();
}