#include "RSAUtils.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <QDebug>

RSAUtils::KeyPair RSAUtils::generateKeyPair(int keySize) {
    KeyPair result;
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) {
        qWarning() << "Failed to create EVP_PKEY_CTX";
        return result;
    }
    
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        qWarning() << "Failed to init keygen";
        EVP_PKEY_CTX_free(ctx);
        return result;
    }
    
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, keySize) <= 0) {
        qWarning() << "Failed to set key size";
        EVP_PKEY_CTX_free(ctx);
        return result;
    }
    
    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        qWarning() << "Failed to generate key";
        EVP_PKEY_CTX_free(ctx);
        return result;
    }
    
    BIO* pubBio = BIO_new(BIO_s_mem());
    BIO* privBio = BIO_new(BIO_s_mem());
    
    if (i2d_PUBKEY_bio(pubBio, pkey) > 0) {
        char* pubData = nullptr;
        long pubLen = BIO_get_mem_data(pubBio, &pubData);
        result.publicKey = QByteArray(pubData, pubLen);
    }
    
    if (i2d_PrivateKey_bio(privBio, pkey) > 0) {
        char* privData = nullptr;
        long privLen = BIO_get_mem_data(privBio, &privData);
        result.privateKey = QByteArray(privData, privLen);
    }
    
    BIO_free(pubBio);
    BIO_free(privBio);
    EVP_PKEY_free(pkey);
    EVP_PKEY_CTX_free(ctx);
    
    return result;
}

QByteArray RSAUtils::encryptByPublicKey(const QByteArray& data, const QByteArray& publicKey) {
    const unsigned char* keyData = reinterpret_cast<const unsigned char*>(publicKey.constData());
    EVP_PKEY* pkey = d2i_PUBKEY(nullptr, &keyData, publicKey.size());
    
    if (!pkey) {
        qWarning() << "Failed to load public key";
        return QByteArray();
    }
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    size_t outLen;
    if (EVP_PKEY_encrypt(ctx, nullptr, &outLen, 
                         reinterpret_cast<const unsigned char*>(data.constData()), 
                         data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    QByteArray result(outLen, 0);
    if (EVP_PKEY_encrypt(ctx, reinterpret_cast<unsigned char*>(result.data()), &outLen,
                         reinterpret_cast<const unsigned char*>(data.constData()),
                         data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    result.resize(outLen);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return result;
}

QByteArray RSAUtils::encryptByPrivateKey(const QByteArray& data, const QByteArray& privateKey) {
    const unsigned char* keyData = reinterpret_cast<const unsigned char*>(privateKey.constData());
    EVP_PKEY* pkey = d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &keyData, privateKey.size());
    
    if (!pkey) {
        qWarning() << "Failed to load private key";
        return QByteArray();
    }
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    if (EVP_PKEY_sign_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    size_t outLen;
    if (EVP_PKEY_sign(ctx, nullptr, &outLen,
                      reinterpret_cast<const unsigned char*>(data.constData()),
                      data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    QByteArray result(outLen, 0);
    if (EVP_PKEY_sign(ctx, reinterpret_cast<unsigned char*>(result.data()), &outLen,
                      reinterpret_cast<const unsigned char*>(data.constData()),
                      data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    result.resize(outLen);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return result;
}

QByteArray RSAUtils::decryptByPrivateKey(const QByteArray& data, const QByteArray& privateKey) {
    const unsigned char* keyData = reinterpret_cast<const unsigned char*>(privateKey.constData());
    EVP_PKEY* pkey = d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &keyData, privateKey.size());
    
    if (!pkey) {
        qWarning() << "Failed to load private key";
        return QByteArray();
    }
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    size_t outLen;
    if (EVP_PKEY_decrypt(ctx, nullptr, &outLen,
                         reinterpret_cast<const unsigned char*>(data.constData()),
                         data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    QByteArray result(outLen, 0);
    if (EVP_PKEY_decrypt(ctx, reinterpret_cast<unsigned char*>(result.data()), &outLen,
                         reinterpret_cast<const unsigned char*>(data.constData()),
                         data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    result.resize(outLen);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return result;
}

QByteArray RSAUtils::decryptByPublicKey(const QByteArray& data, const QByteArray& publicKey) {
    const unsigned char* keyData = reinterpret_cast<const unsigned char*>(publicKey.constData());
    EVP_PKEY* pkey = d2i_PUBKEY(nullptr, &keyData, publicKey.size());
    
    if (!pkey) {
        qWarning() << "Failed to load public key";
        return QByteArray();
    }
    
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    if (EVP_PKEY_verify_recover_init(ctx) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    size_t outLen;
    if (EVP_PKEY_verify_recover(ctx, nullptr, &outLen,
                                reinterpret_cast<const unsigned char*>(data.constData()),
                                data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    QByteArray result(outLen, 0);
    if (EVP_PKEY_verify_recover(ctx, reinterpret_cast<unsigned char*>(result.data()), &outLen,
                                reinterpret_cast<const unsigned char*>(data.constData()),
                                data.size()) <= 0) {
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return QByteArray();
    }
    
    result.resize(outLen);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    
    return result;
}
