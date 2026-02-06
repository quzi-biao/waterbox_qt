#include "NetworkUtils.h"
#include <QNetworkInterface>
#include <QCryptographicHash>
#include <QUuid>
#include <QDebug>

QStringList NetworkUtils::getMacAddresses() {
    QStringList macList;
    
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {
        QString mac = interface.hardwareAddress();
        if (!mac.isEmpty() && mac != "00:00:00:00:00:00") {
            macList.append(mac);
        }
    }
    
    return macList;
}

QString NetworkUtils::getFirstMacAddress() {
    QStringList macs = getMacAddresses();
    if (macs.isEmpty()) {
        qWarning() << "No MAC address found";
        return QString();
    }
    return macs.first();
}

QString NetworkUtils::generateUniqueId(const QString& macAddress, const QString& staticString) {
    QString combined = macAddress + staticString;
    QByteArray hash = QCryptographicHash::hash(combined.toUtf8(), QCryptographicHash::Md5);
    return QString::fromUtf8(hash.toHex());
}

QString NetworkUtils::generateUUID() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
