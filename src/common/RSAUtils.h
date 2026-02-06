#ifndef RSAUTILS_H
#define RSAUTILS_H

#include <QByteArray>
#include <QString>
#include <QPair>

class RSAUtils {
public:
    struct KeyPair {
        QByteArray publicKey;
        QByteArray privateKey;
    };
    
    static KeyPair generateKeyPair(int keySize = 1024);
    
    static QByteArray encryptByPublicKey(const QByteArray& data, const QByteArray& publicKey);
    static QByteArray encryptByPrivateKey(const QByteArray& data, const QByteArray& privateKey);
    
    static QByteArray decryptByPublicKey(const QByteArray& data, const QByteArray& publicKey);
    static QByteArray decryptByPrivateKey(const QByteArray& data, const QByteArray& privateKey);
    
private:
    RSAUtils() = delete;
};

#endif
