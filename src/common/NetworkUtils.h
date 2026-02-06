#ifndef NETWORKUTILS_H
#define NETWORKUTILS_H

#include <QString>
#include <QStringList>

class NetworkUtils {
public:
    static QStringList getMacAddresses();
    
    static QString getFirstMacAddress();
    
    static QString generateUniqueId(const QString& macAddress, const QString& staticString);
    
    static QString generateUUID();
    
private:
    NetworkUtils() = delete;
};

#endif
