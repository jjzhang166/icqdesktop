#include "stdafx.h"

#include "translator_base.h"

namespace translate
{
	void translator_base::init()
	{
		QString lang = getLang();

		QLocale::setDefault(QLocale(lang));
		static QTranslator translator;
		translator.load(lang, ":/translations");

		QApplication::installTranslator(&translator);
	}

	QString translator_base::formatDate(const QDate& target, bool currentYear)
	{
		QString format = currentYear ? getCurrentYearDateFormat() : getOtherYearsDateFormat();
		QString result = QLocale().toString(target, format);
		if (getLang() == "pt")
			result = result.arg("de");

		return result;
	}

	QString translator_base::getNumberString(int number, const QString& one, const QString& two, const QString& five, const QString& twentyOne)
	{
		if (number == 1)
			return one;

		QString result;
		int strCase = number % 10;
		if (strCase == 1 && number % 100 != 11)
		{
			result = twentyOne;
		}
		else if (strCase > 4 || strCase == 0 || (number % 100 > 10 && number % 100 < 20))
		{
			result = five;
		}
		else
		{
			result = two;
		}

		return result;
	}

	QString translator_base::getCurrentYearDateFormat()
	{
		QString lang = getLang();
		if (lang == "ru")
			return QString("d MMM");
		else if (lang == "de")
			return QString("d. MMM");
		else if (lang == "pt")
			return QString("d %1 MMMM");
		else if (lang == "uk")
			return QString("d MMM");
		else if (lang == "cs")
			return QString("d. MMM");
		
		return QString("MMM d");
	}

	QString translator_base::getOtherYearsDateFormat()
	{
		QString lang = getLang();
		if (lang == "ru")
			return QString("d MMM yyyy");
		else if (lang == "de")
			return QString("d. MMM yyyy");
		else if (lang == "pt")
			return QString("d %1 MMMM %1 yyyy");
		else if (lang == "uk")
			return QString("d MMM yyyy");
		else if (lang == "cs")
			return QString("d. MMM yyyy");

		return QString("MMM d, yyyy");
	}

    const QList<QString> &translator_base::getLanguages()
    {
        static QList<QString> clist;
        if (clist.isEmpty())
        {
            clist.push_back("ru");
            clist.push_back("en");
            clist.push_back("uk");
            clist.push_back("de");
            clist.push_back("pt");
            clist.push_back("cs");
        }
        return clist;
    }

	QString translator_base::getLang()
	{
		QString lang;
		
		QString localLang = QLocale::system().name();
		if (localLang.isEmpty())
			lang = "en";
		else
			lang = localLang.left(2);

		if (lang != "ru" && lang != "en" && lang != "uk" && lang != "de" && lang != "pt" && lang != "cs")
			lang = "en";
		
		return lang;
	}
}