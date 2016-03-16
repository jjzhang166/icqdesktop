#pragma once
#include <QtCore/qglobal.h>

#ifdef __APPLE__
#   import <QtCore/qdatetime.h>
#else
#   include <QDateTime>
#endif

#undef QT_TR_NOOP
#define QT_TR_NOOP(x) tr(x)
#undef QT_TRANSLATE_NOOP
#define QT_TRANSLATE_NOOP(scope, x) QApplication::translate(scope, x)
#undef QT_TRANSLATE_NOOP3
#define QT_TRANSLATE_NOOP3(scope, x, comment) QApplication::translate(scope, x, comment)

namespace translate
{
	class translator_base
	{
	public:
		virtual void init();
		QString formatDate(const QDate& target, bool currentYear);
		QString getNumberString(int number, const QString& one, const QString& two, const QString& five, const QString& twentyOne);

        const QList<QString> &getLanguages();
                
	protected:

		virtual QString getLang();
		QString getCurrentYearDateFormat();
		QString getOtherYearsDateFormat();
	};
}