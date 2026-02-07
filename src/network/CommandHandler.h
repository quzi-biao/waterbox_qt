#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QObject>
#include <QJsonObject>
#include <QString>

class HttpSender;
class ConfigManager;

class CommandHandler : public QObject {
    Q_OBJECT
    
public:
    explicit CommandHandler(QObject* parent = nullptr);
    ~CommandHandler();
    
    void initialize(HttpSender* sender);
    
    void handleCommand(const QJsonObject& cmd);
    
signals:
    void settingUpdated();
    void specialSettingUpdated();
    
private:
    void handleReadSetting(const QJsonObject& cmd);
    void handleReadVersion(const QJsonObject& cmd);
    void handleWriteSetting(const QJsonObject& cmd);
    void handleWriteSpecialSetting(const QJsonObject& cmd);
    void handleReadSpecialSetting(const QJsonObject& cmd);
    void handleEndPress(const QJsonObject& cmd);
    void handleWritePlcData(const QJsonObject& cmd);
    
    QJsonObject getSettings();
    QString getVersion();
    
    HttpSender* m_sender;
};

#endif
