#include "qmpreferences.h"

QMPreferences::QMPreferences(const QString & fileName)
: fileName_(fileName)
{
	assert(fileName.length());
}

QMPreferences::~QMPreferences()
{
}

bool QMPreferences::load()
{
	// SP: open the file
	QFile dataFile(fileName_);
	
	if (dataFile.open(QFile::ReadOnly) == false) {
        return false;
	}
		
	// SP: read data from
	QByteArray data = dataFile.readAll();
	if (data.length()) {
		QDataStream readStream(&data, QIODevice::ReadOnly);
		readStream >> *this;
	}
    
//    QMPreferenceHash hash = getHash();
//    
//    for (QMPreferenceHash::Iterator i = hash.begin(); i != hash.end(); i++)
//    {
//        qDebug() << i.key() << " : " << i.value().size();
//    }
	
	return true;
}
QMPreferenceHash QMPreferences::getHash() {
    QMPreferenceHash hash(*this);
    hash.detach();
    return hash;
}
