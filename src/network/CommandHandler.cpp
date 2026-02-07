#include "CommandHandler.h"
#include "HttpSender.h"
#include "core/ConfigManager.h"
#include "database/DatabaseManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDebug>

CommandHandler::CommandHandler(QObject* parent)
    : QObject(parent),
      m_sender(nullptr) {
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::initialize(HttpSender* sender) {
    m_sender = sender;
}

void CommandHandler::handleCommand(const QJsonObject& cmd) {
    QString cmdType = cmd.value("cmd").toString();
    
    if (cmdType.isEmpty()) {
        return;
    }
    
    qInfo() << "处理命令:" << cmdType;
    
    if (cmdType == "readSetting") {
        handleReadSetting(cmd);
    } else if (cmdType == "readVersion") {
        handleReadVersion(cmd);
    } else if (cmdType == "writeSetting") {
        handleWriteSetting(cmd);
    } else if (cmdType == "writeSpecialSetting") {
        handleWriteSpecialSetting(cmd);
    } else if (cmdType == "readSpecialSetting") {
        handleReadSpecialSetting(cmd);
    } else if (cmdType == "endPress") {
        handleEndPress(cmd);
    } else if (cmdType == "writePlcData") {
        handleWritePlcData(cmd);
    } else {
        qWarning() << "未知命令:" << cmdType;
    }
}

void CommandHandler::handleReadSetting(const QJsonObject& cmd) {
    QJsonObject settings = getSettings();
    QString jsonStr = QJsonDocument(settings).toJson(QJsonDocument::Compact);
    
    if (m_sender && m_sender->isInitialized()) {
        m_sender->sendCommand("readSetting", jsonStr);
        qInfo() << "发送设置信息到服务端";
    }
}

void CommandHandler::handleReadVersion(const QJsonObject& cmd) {
    QString version = getVersion();
    
    if (m_sender && m_sender->isInitialized()) {
        m_sender->sendCommand("readVersion", version);
        qInfo() << "发送版本信息到服务端: " << version;
    }
}

void CommandHandler::handleWriteSetting(const QJsonObject& cmd) {
    qInfo() << "处理写入设置命令, 完整内容:" << QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    
    QJsonValue settingValue = cmd.value("setting");
    QJsonObject settingObj;
    
    if (settingValue.isObject()) {
        settingObj = settingValue.toObject();
    } else if (settingValue.isString()) {
        QString settingStr = settingValue.toString();
        if (settingStr.isEmpty()) {
            qWarning() << "设置内容为空";
            return;
        }
        QJsonDocument doc = QJsonDocument::fromJson(settingStr.toUtf8());
        if (!doc.isObject()) {
            qWarning() << "设置格式错误";
            return;
        }
        settingObj = doc.object();
    } else {
        qWarning() << "设置内容为空";
        return;
    }
    ConfigManager* config = ConfigManager::instance();
    
    // 遍历所有设置项并更新到 ConfigManager
    for (auto it = settingObj.begin(); it != settingObj.end(); ++it) {
        QString key = it.key();
        QJsonValue value = it.value();
        
        // 跳过 updatetime 和 id 等系统字段
        if (key == "updatetime" || key == "id") {
            continue;
        }
        
        // 根据值类型设置配置
        if (value.isBool()) {
            config->set(key, value.toBool());
        } else if (value.isDouble()) {
            config->set(key, value.toDouble());
        } else if (value.isString()) {
            config->set(key, value.toString());
        } else if (value.isNull()) {
            // 跳过 null 值
            continue;
        }
    }
    
    // 保存配置
    config->save();
    qInfo() << "配置更新成功";
    
    // 通知 UI 刷新
    emit settingUpdated();
    
    // 返回更新后的配置
    if (m_sender && m_sender->isInitialized()) {
        QJsonObject updatedSettings = getSettings();
        QString jsonStr = QJsonDocument(updatedSettings).toJson(QJsonDocument::Compact);
        m_sender->sendCommand("writeSetting", jsonStr);
        qInfo() << "发送更新后的设置到服务端";
    }
}

void CommandHandler::handleWriteSpecialSetting(const QJsonObject& cmd) {
    qInfo() << "处理写入特殊设置命令";
    qInfo() << "完整的 cmd 内容:" << QJsonDocument(cmd).toJson(QJsonDocument::Compact);
    
    // setting 字段本身就是数组，直接获取
    QJsonValue settingValue = cmd.value("setting");
    if (!settingValue.isArray()) {
        qWarning() << "特殊设置格式错误，应该是数组";
        return;
    }
    
    QJsonArray settingArray = settingValue.toArray();
    if (settingArray.isEmpty()) {
        qWarning() << "特殊设置内容为空";
        return;
    }
    
    qInfo() << "setting 数组大小:" << settingArray.size();
    DatabaseManager* db = DatabaseManager::instance();
    QJsonArray updatedArray;
    
    for (const QJsonValue& val : settingArray) {
        if (!val.isObject()) {
            continue;
        }
        
        QJsonObject kvObj = val.toObject();
        QString key = kvObj.value("itemKey").toString();
        QString value = kvObj.value("itemValue").toString();
        QString type = kvObj.value("itemType").toString();
        
        if (key.isEmpty()) {
            continue;
        }
        
        // 保存到数据库的 KV 存储
        db->saveKeyValue(key, value);
        
        qInfo() << "保存特殊设置:" << key << "=" << value << "类型:" << type;
        
        // 构建返回的 KeyValue 对象
        QJsonObject updatedKv;
        updatedKv["itemKey"] = key;
        updatedKv["itemValue"] = value;
        updatedKv["itemType"] = type;
        updatedArray.append(updatedKv);
    }
    
    // 返回更新后的配置
    if (m_sender && m_sender->isInitialized()) {
        QString jsonStr = QJsonDocument(updatedArray).toJson(QJsonDocument::Compact);
        m_sender->sendCommand("writeSpecialSetting", jsonStr);
        qInfo() << "发送更新后的特殊设置到服务端";
    }
    
    // 发送信号通知配置已更新
    emit specialSettingUpdated();
    qInfo() << "特殊设置已更新，通知界面刷新";
}

void CommandHandler::handleReadSpecialSetting(const QJsonObject& cmd) {
    qInfo() << "处理读取特殊设置命令";
    
    DatabaseManager* db = DatabaseManager::instance();
    QJsonArray settingsArray;
    
    // 读取 METRIC_INDICATOR_KEY
    QVariant indicatorValue = db->getKeyValue("METRIC_INDICATOR_KEY");
    if (!indicatorValue.isNull()) {
        QJsonObject kv;
        kv["itemKey"] = "METRIC_INDICATOR_KEY";
        kv["itemValue"] = indicatorValue.toString();
        kv["itemType"] = "java.util.List";
        settingsArray.append(kv);
    }
    
    // 返回特殊设置
    if (m_sender && m_sender->isInitialized()) {
        QString jsonStr = QJsonDocument(settingsArray).toJson(QJsonDocument::Compact);
        m_sender->sendCommand("readSpecialSetting", jsonStr);
        qInfo() << "发送特殊设置到服务端";
    }
}

void CommandHandler::handleEndPress(const QJsonObject& cmd) {
    qInfo() << "处理末端压力命令";
    // TODO: 实现末端压力处理逻辑
}

void CommandHandler::handleWritePlcData(const QJsonObject& cmd) {
    qInfo() << "处理写入PLC数据命令";
    // TODO: 实现PLC数据写入逻辑
}

QJsonObject CommandHandler::getSettings() {
    ConfigManager* config = ConfigManager::instance();
    QJsonObject settings;
    
    // 读取所有配置项，只有存在的才返回
    QVariant value;
    
    // 基本配置
    if (!(value = config->get("serviceAddress")).isNull()) settings["serviceAddress"] = value.toString();
    if (!(value = config->get("tag")).isNull()) settings["tag"] = value.toString();
    
    // PLC 配置
    if (!(value = config->get("plcServer")).isNull()) settings["plcServer"] = value.toBool();
    if (!(value = config->get("plcLocalHost")).isNull()) settings["plcLocalHost"] = value.toString();
    if (!(value = config->get("plcPort")).isNull()) settings["plcPort"] = value.toInt();
    if (!(value = config->get("plcProtocol")).isNull()) settings["plcProtocol"] = value.toString();
    if (!(value = config->get("plcSimulate")).isNull()) settings["plcSimulate"] = value.toBool();
    if (!(value = config->get("plcCtrl")).isNull()) settings["plcCtrl"] = value.toBool();
    if (!(value = config->get("plcValveReadAddress")).isNull()) settings["plcValveReadAddress"] = value.toString();
    if (!(value = config->get("plcValveWriteAddress")).isNull()) settings["plcValveWriteAddress"] = value.toString();
    
    // 采样配置
    if (!(value = config->get("sampleInterval")).isNull()) settings["sampleInterval"] = value.toInt();
    
    // 水箱参数
    if (!(value = config->get("hlimit")).isNull()) settings["hlimit"] = value.toDouble();
    if (!(value = config->get("llimit")).isNull()) settings["llimit"] = value.toDouble();
    if (!(value = config->get("climit")).isNull()) settings["climit"] = value.toDouble();
    if (!(value = config->get("slimit")).isNull()) settings["slimit"] = value.toDouble();
    if (!(value = config->get("floatValveLimit")).isNull()) settings["floatValveLimit"] = value.toDouble();
    if (!(value = config->get("boxLong")).isNull()) settings["boxLong"] = value.toDouble();
    if (!(value = config->get("boxWide")).isNull()) settings["boxWide"] = value.toDouble();
    if (!(value = config->get("boxHigh")).isNull()) settings["boxHigh"] = value.toDouble();
    if (!(value = config->get("boxNum")).isNull()) settings["boxNum"] = value.toInt();
    
    // 水质参数
    if (!(value = config->get("temperature")).isNull()) settings["temperature"] = value.toDouble();
    if (!(value = config->get("toc")).isNull()) settings["toc"] = value.toDouble();
    if (!(value = config->get("cl0")).isNull()) settings["cl0"] = value.toDouble();
    if (!(value = config->get("cl1")).isNull()) settings["cl1"] = value.toDouble();
    if (!(value = config->get("clsave")).isNull()) settings["clsave"] = value.toDouble();
    if (!(value = config->get("wh")).isNull()) settings["wh"] = value.toDouble();
    if (!(value = config->get("wflow")).isNull()) settings["wflow"] = value.toDouble();
    if (!(value = config->get("press")).isNull()) settings["press"] = value.toDouble();
    
    // 阀门控制
    if (!(value = config->get("valveAutoControl")).isNull()) settings["valveAutoControl"] = value.toBool();
    if (!(value = config->get("isAdjustableValve")).isNull()) settings["isAdjustableValve"] = value.toBool();
    if (!(value = config->get("valveScheduleInterval")).isNull()) settings["valveScheduleInterval"] = value.toLongLong();
    
    // 算法参数
    if (!(value = config->get("a")).isNull()) settings["a"] = value.toDouble();
    if (!(value = config->get("b")).isNull()) settings["b"] = value.toDouble();
    if (!(value = config->get("c")).isNull()) settings["c"] = value.toDouble();
    if (!(value = config->get("d")).isNull()) settings["d"] = value.toDouble();
    if (!(value = config->get("e")).isNull()) settings["e"] = value.toDouble();
    if (!(value = config->get("f")).isNull()) settings["f"] = value.toDouble();
    if (!(value = config->get("g")).isNull()) settings["g"] = value.toDouble();
    if (!(value = config->get("iterateCount")).isNull()) settings["iterateCount"] = value.toInt();
    if (!(value = config->get("useWaterLevelCalWaterUse")).isNull()) settings["useWaterLevelCalWaterUse"] = value.toBool();
    
    // 余氯控制
    if (!(value = config->get("clControl")).isNull()) settings["clControl"] = value.toBool();
    if (!(value = config->get("hasClDevice")).isNull()) settings["hasClDevice"] = value.toBool();
    if (!(value = config->get("clAuxiliaryValue")).isNull()) settings["clAuxiliaryValue"] = value.toDouble();
    if (!(value = config->get("clAuxiliaryMin")).isNull()) settings["clAuxiliaryMin"] = value.toDouble();
    if (!(value = config->get("clAuxiliaryMax")).isNull()) settings["clAuxiliaryMax"] = value.toDouble();
    if (!(value = config->get("clAuxiliaryAlias")).isNull()) settings["clAuxiliaryAlias"] = value.toDouble();
    if (!(value = config->get("whCorrect")).isNull()) settings["whCorrect"] = value.toDouble();
    
    // 设备配置
    if (!(value = config->get("hasFlowMeter")).isNull()) settings["hasFlowMeter"] = value.toBool();
    if (!(value = config->get("senseProduct")).isNull()) settings["senseProduct"] = value.toString();
    
    // 泵组监控
    if (!(value = config->get("openMetric")).isNull()) settings["openMetric"] = value.toBool();
    if (!(value = config->get("pumpMetricReadInterval")).isNull()) settings["pumpMetricReadInterval"] = value.toLongLong();
    if (!(value = config->get("regionNumber")).isNull()) settings["regionNumber"] = value.toInt();
    if (!(value = config->get("pumpNumber")).isNull()) settings["pumpNumber"] = value.toInt();
    
    // 压力控制
    if (!(value = config->get("openPressCtrl")).isNull()) settings["openPressCtrl"] = value.toBool();
    if (!(value = config->get("minPress")).isNull()) settings["minPress"] = value.toDouble();
    if (!(value = config->get("defaultPress")).isNull()) settings["defaultPress"] = value.toDouble();
    if (!(value = config->get("maxPress")).isNull()) settings["maxPress"] = value.toDouble();
    if (!(value = config->get("pressIncreaseStep")).isNull()) settings["pressIncreaseStep"] = value.toDouble();
    if (!(value = config->get("pressIncreaseInterval")).isNull()) settings["pressIncreaseInterval"] = value.toLongLong();
    if (!(value = config->get("ctrlPressPlcAddress")).isNull()) settings["ctrlPressPlcAddress"] = value.toString();
    if (!(value = config->get("endPressAddress")).isNull()) settings["endPressAddress"] = value.toString();
    if (!(value = config->get("endPressDeviceCode")).isNull()) settings["endPressDeviceCode"] = value.toString();
    if (!(value = config->get("endPressReim")).isNull()) settings["endPressReim"] = value.toBool();
    if (!(value = config->get("endPressReimRate")).isNull()) settings["endPressReimRate"] = value.toDouble();
    if (!(value = config->get("endPressStandard")).isNull()) settings["endPressStandard"] = value.toDouble();
    if (!(value = config->get("endPressAvgTime")).isNull()) settings["endPressAvgTime"] = value.toLongLong();
    if (!(value = config->get("pressCtrlType")).isNull()) settings["pressCtrlType"] = value.toInt();
    
    // 其他配置
    if (!(value = config->get("dirtyFilter")).isNull()) settings["dirtyFilter"] = value.toBool();
    if (!(value = config->get("useSpecialControl")).isNull()) settings["useSpecialControl"] = value.toBool();
    if (!(value = config->get("specialTime")).isNull()) settings["specialTime"] = value.toString();
    if (!(value = config->get("scheduleType")).isNull()) settings["scheduleType"] = value.toString();
    if (!(value = config->get("productType")).isNull()) settings["productType"] = value.toInt();
    if (!(value = config->get("sendType")).isNull()) settings["sendType"] = value.toString();
    if (!(value = config->get("rtuProtocol")).isNull()) settings["rtuProtocol"] = value.toString();
    
    settings["updatetime"] = QDateTime::currentMSecsSinceEpoch();
    
    return settings;
}

QString CommandHandler::getVersion() {
    // 返回 Qt 版本标识
    return "box-qt-" + QCoreApplication::applicationVersion();
}
