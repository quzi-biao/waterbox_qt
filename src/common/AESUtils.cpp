#include "AESUtils.h"
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <QDebug>

QString AESUtils::generateKey() {
    unsigned char key[16];
    if (RAND_bytes(key, sizeof(key)) != 1) {
        qWarning() << "Failed to generate random key";
        return QString();
    }
    
    return QByteArray(reinterpret_cast<char*>(key), sizeof(key)).toHex();
}

QString AESUtils::encrypt(const QString& keyHex, const QString& data) {
    QByteArray keyBytes = QByteArray::fromHex(keyHex.toUtf8());
    if (keyBytes.size() != 16) {
        qWarning() << "Invalid key size";
        return QString();
    }
    
    QByteArray plainText = data.toUtf8();
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return QString();
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, 
                           reinterpret_cast<const unsigned char*>(keyBytes.constData()), 
                           nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QString();
    }
    
    EVP_CIPHER_CTX_set_padding(ctx, 1);
    
    int maxOutLen = plainText.size() + AES_BLOCK_SIZE;
    QByteArray cipherText(maxOutLen, 0);
    int outLen = 0;
    
    if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(cipherText.data()), 
                          &outLen, 
                          reinterpret_cast<const unsigned char*>(plainText.constData()), 
                          plainText.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QString();
    }
    
    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(cipherText.data()) + outLen, 
                            &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QString();
    }
    
    cipherText.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);
    
    return QString::fromUtf8(cipherText.toHex());
}

QString AESUtils::decrypt(const QString& keyHex, const QString& dataHex) {
    QByteArray keyBytes = QByteArray::fromHex(keyHex.toUtf8());
    if (keyBytes.size() != 16) {
        qWarning() << "Invalid key size";
        return QString();
    }
    
    QByteArray cipherText = QByteArray::fromHex(dataHex.toUtf8());
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return QString();
    }
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr,
                           reinterpret_cast<const unsigned char*>(keyBytes.constData()),
                           nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QString();
    }
    
    EVP_CIPHER_CTX_set_padding(ctx, 1);
    
    QByteArray plainText(cipherText.size() + AES_BLOCK_SIZE, 0);
    int outLen = 0;
    
    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plainText.data()),
                          &outLen,
                          reinterpret_cast<const unsigned char*>(cipherText.constData()),
                          cipherText.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QString();
    }
    
    int finalLen = 0;
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plainText.data()) + outLen,
                            &finalLen) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return QString();
    }
    
    plainText.resize(outLen + finalLen);
    EVP_CIPHER_CTX_free(ctx);
    
    return QString::fromUtf8(plainText);
}
