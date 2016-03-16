#ifndef QMPREFERENCES_H
#define QMPREFERENCES_H

#include "stdafx.h"

typedef QHash<QString,QByteArray> QMPreferenceHash;


class QMPreferences
: public QMPreferenceHash
{
	
public:
	explicit QMPreferences(const QString & fileName);
	virtual ~QMPreferences();
private:
	inline QString compoundKeyForKeyInTable(const QString & key, const QString & table) {
		return table+"::"+key;
	}
public:

	bool load();
	
	template<class T> T get(const QString & key)
	{
		QByteArray & value = (*this)[key];
		return fromData<T>(&value);
	}
	
	template<class T> T get(const QString & table, const QString & key)
	{
		QByteArray & value = (*this)[compoundKeyForKeyInTable(key,table)];
		return fromData<T>(&value);
	}
    QMPreferenceHash getHash();

private:
    inline QString fileName(){return fileName_;}

	template<class T> inline T fromData(QByteArray * value)
	{
		T result;
		if (value) {
			QDataStream stream(value, QIODevice::ReadOnly);
			stream >> result;
		}
		return result;
	}

private:
	template<class T> inline T basicTypeFromData(QByteArray * value)
	{
		T result = 0;
		if (value) {
			QDataStream stream(value, QIODevice::ReadOnly);
			stream >> result;
		}
		return result;
	}

    QString fileName_;
};

template<> inline int QMPreferences::fromData<int>(QByteArray * value)
{
	return basicTypeFromData<int>(value);
}

template<> inline bool QMPreferences::fromData<bool>(QByteArray * value)
{
	return basicTypeFromData<bool>(value);
}

//template<> inline long QMPreferences::fromData<long>(QByteArray * value)
//{
//	return basicTypeFromData<long>(value);
//}

template<> inline unsigned int QMPreferences::fromData<unsigned int>(QByteArray * value)
{
	return basicTypeFromData<unsigned int>(value);
}


#endif // QMPREFERENCES_H
