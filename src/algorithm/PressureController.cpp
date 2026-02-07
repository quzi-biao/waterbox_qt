#include "PressureController.h"
#include "interfaces/IPLCClient.h"
#include "database/DatabaseManager.h"
#include "ConfigManager.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath>

PressureController::PressureController(IPLCClient* plcClient, QObject* parent)
    : QObject(parent),
      m_plcClient(plcClient),
      m_enabled(false),
      m_pressCtrlType(0),
      m_defaultPressure(0.3),
      m_minPressure(0.2),
      m_maxPressure(0.5),
      m_pressIncreaseStep(0.002),
      m_pressIncreaseInterval(60),
      m_hst(19.0),
      m_h0(15.0),
      m_rate(4.0),
      m_qs(45.0),
      m_lastWriteTime(0),
      m_lastCtrlPress(0),
      m_lastWritePress(0) {
    reloadConfig();
}

void PressureController::reloadConfig() {
    ConfigManager* config = ConfigManager::instance();
    
    m_ctrlPressPlcAddress = config->get("ctrlPressPlcAddress", "").toString();
    m_flowPlcAddress = config->get("flowPlcAddress", "").toString();
    m_pressCtrlType = config->get("pressCtrlType", 0).toInt();
    m_defaultPressure = config->get("defaultPress", 0.3).toDouble();
    m_minPressure = config->get("minPress", 0.2).toDouble();
    m_maxPressure = config->get("maxPress", 0.5).toDouble();
    m_pressIncreaseStep = config->get("pressIncreaseStep", 0.002).toDouble();
    
    int interval = config->get("pressIncreaseInterval", 60).toInt();
    m_pressIncreaseInterval = (interval < 10) ? 60 : interval;
    
    // 解析压力计算公式 JSON
    QString pressCalcStr = config->get("pressCalculator", "").toString();
    if (!pressCalcStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(pressCalcStr.toUtf8());
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_hst = obj.value("hst").toString("19").toDouble();
            m_h0 = obj.value("h0").toString("15").toDouble();
            m_rate = obj.value("rate").toString("4").toDouble();
            m_qs = obj.value("qs").toString("45").toDouble();
        }
    }
    
    qInfo() << "压力控制配置加载: 类型=" << m_pressCtrlType
            << "默认=" << m_defaultPressure << "范围=[" << m_minPressure << "," << m_maxPressure << "]"
            << "步长=" << m_pressIncreaseStep << "间隔=" << m_pressIncreaseInterval << "s"
            << "公式参数: hst=" << m_hst << "h0=" << m_h0 << "rate=" << m_rate << "qs=" << m_qs
            << "写入地址=" << (m_ctrlPressPlcAddress.isEmpty() ? "未设置" : m_ctrlPressPlcAddress)
            << "流量地址=" << (m_flowPlcAddress.isEmpty() ? "未设置" : m_flowPlcAddress);
}

QMap<QString, QVariant> PressureController::readData() {
    QMap<QString, QVariant> data;
    return data;
}

QMap<QString, QVariant> PressureController::processData(const QMap<QString, QVariant>& inputData) {
    QMap<QString, QVariant> controlData;
    
    // 检查必要配置
    if (m_defaultPressure <= 0 || m_minPressure <= 0 || m_maxPressure <= 0) {
        qWarning() << "压力控制配置未完成，暂不启用";
        return controlData;
    }
    
    // pressCtrlType != 1 时需要 pressCalculator
    if (m_pressCtrlType != 1 && m_qs <= 0) {
        qWarning() << "压力计算公式未配置，暂不启用";
        return controlData;
    }
    
    // 检查时间间隔
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_lastWriteTime > 0 &&
        (currentTime - m_lastWriteTime) < m_pressIncreaseInterval * 1000) {
        return controlData;
    }
    
    double currentFlow = 0.0;
    if (!m_flowPlcAddress.isEmpty()) {
        currentFlow = inputData.value(m_flowPlcAddress, 0.0).toDouble();
    } else {
        currentFlow = inputData.value("flow", 0.0).toDouble();
    }
    
    qInfo() << "节能压力控制开始调度, 当前流量:" << currentFlow;
    
    double dstPress = getDstPress(currentFlow);
    double writePress = calWritePress(dstPress);
    
    if (m_ctrlPressPlcAddress.isEmpty()) {
        qInfo() << "压力计算结果(仅计算): 目标压力=" << dstPress << "写入压力=" << writePress;
    }else{
        qInfo() << "压力计算结果: 目标压力=" << dstPress << "写入压力=" << writePress;
    }
    
    controlData["target_pressure"] = dstPress;
    controlData["write_pressure"] = writePress;
    controlData["control_address"] = m_ctrlPressPlcAddress;
    
    m_lastCtrlPress = dstPress;
    m_lastWritePress = writePress;
    
    emit pressureCalculated(dstPress, writePress);
    
    return controlData;
}

bool PressureController::writeToPLC(const QMap<QString, QVariant>& controlData) {
    if (!m_plcClient || !m_plcClient->isConnected()) {
        return false;
    }
    
    // openPressCtrl 控制是否实际写入 PLC
    if (!m_enabled) {
        return false;
    }
    
    if (m_ctrlPressPlcAddress.isEmpty()) {
        //qWarning() << "节能功能已开启，但 PLC 写入地址未配置";
        return false;
    }
    
    if (!controlData.contains("write_pressure")) {
        return false;
    }
    
    double pressure = controlData["write_pressure"].toDouble();
    
    // 安全校验：压力值必须在合理范围内
    if (pressure <= 0.05 || pressure > 2.0 ||
        pressure < m_minPressure || pressure > m_maxPressure) {
        qWarning() << "写入压力数值非法:" << pressure;
        return false;
    }
    
    bool success = m_plcClient->writeData(m_ctrlPressPlcAddress, pressure);
    
    if (success) {
        m_lastWriteTime = QDateTime::currentMSecsSinceEpoch();
        qInfo() << "写入压力成功:" << m_ctrlPressPlcAddress << "=" << pressure;
    } else {
        qWarning() << "写入压力失败:" << m_ctrlPressPlcAddress << "=" << pressure;
    }
    
    return success;
}

void PressureController::setEnabled(bool enabled) {
    m_enabled = enabled;
}

bool PressureController::isEnabled() const {
    return m_enabled;
}

/**
 * 计算目标压力（对应 Java getDstPress）
 * pressCtrlType == 1: 分时段控压（暂不实现，返回默认压力）
 * 其他: 公式计算
 */
double PressureController::getDstPress(double currentFlow) {
    double press = m_defaultPressure;
    
    if (m_pressCtrlType == 1) {
        // 分时段控压 - 暂使用默认压力
        // TODO: 实现 pressPeriodControlSetting 匹配逻辑
    } else {
        press = calDstPress(currentFlow);
    }
    
    if (press <= 0) {
        press = m_defaultPressure;
    }
    
    // 限制在 [minPress, maxPress] 范围内
    if (press < m_minPressure) {
        return m_minPressure;
    }
    if (press > m_maxPressure) {
        return m_maxPressure;
    }
    
    return press;
}

/**
 * 公式计算目标压力（对应 Java calDstPress + PressCalculator.cal）
 * 公式: press = (hst + h0 + sq) / 100
 * 其中: sq = (s / (qs * qs)) * flow * flow
 *       s = rate（简化版，不含 rateDelta）
 */
double PressureController::calDstPress(double currentFlow) {
    double avgFlow = nearAvgFlow();
    if (avgFlow <= 0) {
        avgFlow = currentFlow;
    }
    
    double s = m_rate;
    double sq = 0;
    if (m_qs > 0) {
        sq = (s / (m_qs * m_qs)) * avgFlow * avgFlow;
    }
    double press = (m_hst + m_h0 + sq) / 100.0;
    
    qInfo() << "公式计算: avgFlow=" << avgFlow << "s=" << s << "sq=" << sq << "press=" << press;
    
    return press;
}

/**
 * 计算写入压力（对应 Java calWritePress）
 * 使用步进方式逐步调整到目标压力
 */
double PressureController::calWritePress(double dstPress) {
    double step = m_pressIncreaseStep;
    if (step <= 0) {
        step = 0.002;
    }
    
    double writePress = m_lastCtrlPress;
    if (writePress <= 0) {
        writePress = m_defaultPressure;
    }
    
    if (qFuzzyCompare(dstPress, writePress)) {
        return writePress;
    }
    
    if (dstPress > writePress) {
        writePress += step;
        if (writePress > dstPress) {
            writePress = dstPress;
        }
    } else {
        writePress -= step;
        if (writePress < dstPress) {
            writePress = dstPress;
        }
    }
    
    return writePress;
}

/**
 * 最近30分钟平均流量（对应 Java nearAvgFlow）
 */
double PressureController::nearAvgFlow() {
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start = now.addSecs(-30 * 60);
    
    QList<PLCDataRecord> records = DatabaseManager::instance()->getUnuploadedData(10000);
    
    if (records.isEmpty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    int count = 0;
    
    for (const PLCDataRecord& record : records) {
        if (record.timestamp >= start && record.correctedValue.isValid()) {
            sum += record.correctedValue.toDouble();
            count++;
        }
    }
    
    return count > 0 ? sum / count : 0.0;
}
