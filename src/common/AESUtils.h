#ifndef AESUTILS_H
#define AESUTILS_H

#include <QString>
#include <QByteArray>

class AESUtils {
public:
    static QString generateKey();
    
    static QString encrypt(const QString& key, const QString& data);
    static QString decrypt(const QString& key, const QString& data);
    
private:
    AESUtils() = delete;
};

#endif
