#pragma once

#ifdef _DEBUG
#define __ENABLE_LOG
#endif //_DEBUG

#ifdef __ENABLE_LOG
	#define __LOG(x) { x }
	#define __WRITE_LOG(type, area, params)									\
	{																		\
		QString fmt;														\
		fmt.reserve(512);													\
																			\
		QTextStream out(&fmt);												\
		out << __FUNCTION__ << ", " __FILE__ ", line " __LINEA__ "\n"			\
			<< params;														\
																			\
		Log::type((area), fmt);												\
	}
#else
	#define __LOG(x) {}
	#define __WRITE_LOG(type, area, params) {}
#endif

#define __TRACE(area, params) __WRITE_LOG(trace, (area), params)
#define __INFO(area, params) __WRITE_LOG(info, (area), params)
#define __WARN(area, params) __WRITE_LOG(warn, (area), params)
#define __ERROR(area, params) __WRITE_LOG(error, (area), params)

#define __LOGP(id, value) #id "=<" << (value) << ">\n"

namespace Log
{

	void trace(const QString& area, const QString& text);

	void info(const QString& area, const QString& text);

	void warn(const QString& area, const QString& text);

	void error(const QString& area, const QString& text);

}

inline QTextStream& operator <<(QTextStream &lhs, const QUrl &uri)
{
    return (lhs << uri.toString());
}

inline QTextStream& operator <<(QTextStream &lhs, const QSize &size)
{
    return (lhs << "(" << size.width() << "," << size.height() << ")");
}

inline QTextStream& operator <<(QTextStream &lhs, const QSizeF &size)
{
    return (lhs << "(" << size.width() << "," << size.height() << ")");
}

inline QTextStream& operator <<(QTextStream &lhs, const QRect &rect)
{
    return (lhs << "(" << rect.x() << "," << rect.y() << "," << rect.width() << "," << rect.height() << ")");
}