#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QMutex>
#include <QJsonObject>

class ConfigManager : public QObject {
    Q_OBJECT
    
public:
    static ConfigManager* instance();
    
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void set(const QString& key, const QVariant& value);
    
    bool load();
    bool save();
    
    void updateFromRemote(const QJsonObject& remoteConfig);
    
    QString plcHost() const;
    int plcPort() const;
    QString plcProtocol() const;
    
    QString reportUrl() const;
    QString reportProtocol() const;
    
    int collectInterval() const;
    int reportInterval() const;
    
    QJsonObject dataSchema() const;
    QMap<QString, bool> addressWritePermissions() const;
    
signals:
    void configChanged(const QString& key);
    void plcAddressWriteRequired(const QString& address, const QVariant& value);
    
private:
    ConfigManager();
    ~ConfigManager();
    
    static ConfigManager* m_instance;
    QMap<QString, QVariant> m_config;
    mutable QMutex m_mutex;
    
    QString m_configFile;
};

#endif
