#include "MainWindow.h"
#include "ConfigDialog.h"
#include "DataViewWidget.h"
#include "HistoryChartWidget.h"
#include "SystemConfigWidget.h"
#include "PLCAddressConfigWidget.h"
#include "plc/PLCClient.h"
#include "simulator/PLCSimulator.h"
#include "core/DataCollector.h"
#include "core/DataSender.h"
#include "algorithm/PressureController.h"
#include "network/HttpReporter.h"
#include "network/RemoteConfigReceiver.h"
#include "network/HttpSender.h"
#include "network/ServiceResponseHandle.h"
#include "network/CommandHandler.h"
#include "monitor/SystemMonitor.h"
#include "database/DatabaseManager.h"
#include "core/ConfigManager.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_systemRunning(false),
      m_simulatorMode(false),
      m_pressureTestMode(false),
      m_collectorThread(nullptr) {
    
    setWindowTitle("WaterBox Qt - 水务监控系统");
    resize(1200, 800);
    
    initializeSystem();
    setupUI();
    createMenuBar();
    createToolBar();
    createStatusBar();
    
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_statusTimer->start(1000);
    
    // PLC 连接状态检查定时器（基于数据读取成功）
    m_plcCheckTimer = new QTimer(this);
    connect(m_plcCheckTimer, &QTimer::timeout, this, &MainWindow::checkPLCConnection);
    m_plcCheckTimer->start(5000); // 每5秒检查一次
    
    // 启动后自动连接 PLC
    QTimer::singleShot(500, this, &MainWindow::autoConnectPLC);
}

MainWindow::~MainWindow() {
    if (m_systemRunning) {
        onStopSystem();
    }
    
    // 停止并清理工作线程
    if (m_collectorThread) {
        m_collectorThread->quit();
        m_collectorThread->wait();
    }
    
    // 删除在工作线程中的对象
    if (m_plcClient) {
        m_plcClient->deleteLater();
    }
    
    if (m_collector) {
        m_collector->deleteLater();
    }
    
    if (m_sender) {
        m_sender->deleteLater();
    }
}

void MainWindow::initializeSystem() {
    DatabaseManager::instance()->initialize();
    ConfigManager::instance()->load();
    
    // 创建数据采集工作线程
    m_collectorThread = new QThread(this);
    
    // 将 PLCClient 和 DataCollector 都移动到工作线程
    m_plcClient = new PLCClient();
    m_plcClient->moveToThread(m_collectorThread);
    
    m_simulator = new PLCSimulator(this);
    
    m_collector = new DataCollector(m_plcClient);
    m_collector->moveToThread(m_collectorThread);
    
    // 使用 Qt::QueuedConnection 确保跨线程信号槽安全
    connect(m_collector, &DataCollector::dataCollected, 
            this, &MainWindow::onDataCollected, Qt::QueuedConnection);
    connect(m_collector, &DataCollector::collectionError,
            this, [this](const QString& error) {
                m_plcConnectionLabel->setText("● PLC: 读取失败");
                m_plcConnectionLabel->setStyleSheet("QLabel { padding: 5px; color: #ff0000; font-weight: bold; }");
                m_reconnectBtn->setVisible(true);
                qWarning() << "数据采集错误:" << error;
            }, Qt::QueuedConnection);
    
    m_collectorThread->start();
    
    // 先创建 HttpSender
    m_httpSender = new HttpSender(this);
    m_httpSender->initialize();
    
    // 创建数据发送器，保持在主线程（因为要使用 HttpSender）
    m_sender = new DataSender(m_httpSender, this);
    m_sender->setDataCollector(m_collector);
    
    m_pressureController = new PressureController(m_plcClient, this);
    
    m_responseHandle = new ServiceResponseHandle(this);
    m_responseHandle->initialize(m_httpSender);
    
    connect(m_httpSender, &HttpSender::responseReceived,
            m_responseHandle, &ServiceResponseHandle::addResponse);
    
    m_systemMonitor = new SystemMonitor(this);
    m_systemMonitor->initialize(m_httpSender);
    m_systemMonitor->start();
    
    RemoteConfigReceiver* configReceiver = new RemoteConfigReceiver(this);
    connect(m_httpSender, &HttpSender::responseReceived,
            [configReceiver](const QByteArray& data) {
                configReceiver->processResponse(QString::fromUtf8(data));
            });
    connect(configReceiver, &RemoteConfigReceiver::plcWriteCommand,
            [this](const QString& address, const QVariant& value) {
                QMetaObject::invokeMethod(m_plcClient, "writeData", Qt::QueuedConnection,
                                          Q_ARG(QString, address),
                                          Q_ARG(QVariant, value));
            });
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    m_tabWidget = new QTabWidget(this);
    
    m_dataView = new DataViewWidget(this);
    m_historyChart = new HistoryChartWidget(this);
    m_systemConfigWidget = new SystemConfigWidget(this);
    connect(m_systemConfigWidget, &SystemConfigWidget::configSaved, this, &MainWindow::onConfigUpdated);
    
    m_plcAddressConfigWidget = new PLCAddressConfigWidget(this);
    m_plcAddressConfigWidget->setHttpSender(m_httpSender);
    
    // 连接 CommandHandler 的信号到 PLCAddressConfigWidget，当服务端下发配置时自动刷新界面
    if (m_responseHandle && m_responseHandle->commandHandler()) {
        connect(m_responseHandle->commandHandler(), &CommandHandler::specialSettingUpdated,
                m_plcAddressConfigWidget, &PLCAddressConfigWidget::reloadConfig);
    }
    
    m_tabWidget->addTab(m_dataView, "实时数据");
    m_tabWidget->addTab(m_historyChart, "历史曲线");
    m_tabWidget->addTab(m_systemConfigWidget, "系统配置");
    m_tabWidget->addTab(m_plcAddressConfigWidget, "PLC地址配置");
    
    mainLayout->addWidget(m_tabWidget);
    
    setCentralWidget(centralWidget);
}

void MainWindow::createMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    
    QMenu* fileMenu = menuBar->addMenu("文件");
    fileMenu->addAction("退出", this, &QMainWindow::close);
    
    setMenuBar(menuBar);
}

void MainWindow::createToolBar() {
    QToolBar* toolBar = new QToolBar(this);
    
    // PLC 连接状态显示
    m_plcConnectionLabel = new QLabel("● PLC: 未连接", this);
    m_plcConnectionLabel->setStyleSheet("QLabel { padding: 5px; color: #ff0000; font-weight: bold; }");
    
    m_reconnectBtn = new QPushButton("重新连接", this);
    m_reconnectBtn->setVisible(false);
    connect(m_reconnectBtn, &QPushButton::clicked, this, &MainWindow::autoConnectPLC);
    
    toolBar->addWidget(m_plcConnectionLabel);
    toolBar->addWidget(m_reconnectBtn);
    
    addToolBar(toolBar);
}

void MainWindow::createStatusBar() {
    m_statusLabel = new QLabel("就绪", this);
    m_plcStatusLabel = new QLabel("PLC: 未连接", this);
    m_dataCountLabel = new QLabel("数据: 0", this);
    
    statusBar()->addWidget(m_statusLabel);
    statusBar()->addPermanentWidget(m_plcStatusLabel);
    statusBar()->addPermanentWidget(m_dataCountLabel);
}

void MainWindow::onConnectPLC() {
    ConfigManager* config = ConfigManager::instance();
    QString host = config->get("plcLocalHost", config->plcHost()).toString();
    int port = config->get("plcPort", config->plcPort()).toInt();
    QString protocol = config->get("plcProtocol", config->plcProtocol()).toString();
    
    bool simulate = config->get("plcSimulate", false).toBool();
    
    if (simulate) {
        if (m_simulator->connect(host, port)) {
            m_collector->deleteLater();
            m_collector = new DataCollector(m_simulator, this);
            connect(m_collector, &DataCollector::dataCollected, 
                    this, &MainWindow::onDataCollected);
            
            m_pressureController->deleteLater();
            m_pressureController = new PressureController(m_simulator, this);
            
            m_plcConnectionLabel->setText("● PLC: 已连接 (模拟器)");
            m_plcConnectionLabel->setStyleSheet("QLabel { padding: 5px; color: #00cc00; font-weight: bold; }");
            m_reconnectBtn->setVisible(false);
            
            // 自动启动系统
            onStartSystem();
        }
    } else {
        // 线程安全地设置协议和连接（异步方式）
        QMetaObject::invokeMethod(m_plcClient, "setProtocol", Qt::QueuedConnection,
                                  Q_ARG(QString, protocol));
        
        QMetaObject::invokeMethod(m_plcClient, "connect", Qt::QueuedConnection,
                                  Q_ARG(QString, host),
                                  Q_ARG(int, port));
        
        // 显示连接中状态
        m_plcConnectionLabel->setText(QString("● PLC: 连接中 (%1:%2)").arg(host).arg(port));
        m_plcConnectionLabel->setStyleSheet("QLabel { padding: 5px; color: #ffaa00; font-weight: bold; }");
        m_reconnectBtn->setVisible(false);
        
        // 延迟启动系统，给 PLC 连接时间
        QTimer::singleShot(1000, this, &MainWindow::onStartSystem);
    }
}

void MainWindow::onDisconnectPLC() {
    if (m_simulatorMode) {
        m_simulator->disconnect();
    } else {
        // PLCClient 在工作线程中，必须通过 QueuedConnection 断开
        QMetaObject::invokeMethod(m_plcClient, "disconnect", Qt::QueuedConnection);
    }
}

void MainWindow::autoConnectPLC() {
    // 验证 PLC 地址（与 onConnectPLC 一致）
    ConfigManager* config = ConfigManager::instance();
    QString host = config->get("plcLocalHost", config->plcHost()).toString();
    if (host.isEmpty()) {
        m_plcConnectionLabel->setText("● PLC: 地址未配置");
        m_plcConnectionLabel->setStyleSheet("QLabel { padding: 5px; color: #ff9900; font-weight: bold; }");
        m_reconnectBtn->setVisible(false);
        qWarning() << "PLC 地址未配置，无法连接";
        return;
    }
    
    onConnectPLC();
}

void MainWindow::onConfigUpdated() {
    
    // 先停止数据采集和发送
    if (m_systemRunning) {
        onStopSystem();
    }
    
    // 延迟断开 PLC，等待工作线程完成当前操作
    QTimer::singleShot(500, this, [this]() {
        onDisconnectPLC();
        
        // 重新加载 DataViewWidget 的配置（在主线程）
        m_dataView->loadMetricIndicators();
        
        // 重新加载历史曲线的地址列表
        m_historyChart->loadAddressList();
        
        // 延迟重新连接，确保断开完成
        QTimer::singleShot(1000, this, &MainWindow::autoConnectPLC);
    });
}

void MainWindow::checkPLCConnection() {
    // 连接状态由数据采集结果决定
    // DataCollector 会在采集成功时更新状态
}

void MainWindow::onStartSystem() {
    // 直接启动数据采集，如果 PLC 未连接，DataCollector 会发出 collectionError 信号
    // 线程安全地启动数据采集和发送
    QMetaObject::invokeMethod(m_collector, "startCollection", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_sender, "start", Qt::QueuedConnection);
    // 压力控制器暂时禁用
    // m_pressureController->setEnabled(true);
    
    m_systemRunning = true;
    
    m_statusLabel->setText("系统运行中");
    
    // 立即读取并显示未上报数据量
    QList<PLCDataRecord> unuploaded = DatabaseManager::instance()->getUnuploadedData(10000);
    m_dataCountLabel->setText(QString("未上报: %1").arg(unuploaded.size()));
    
}

void MainWindow::onStopSystem() {
    // 线程安全地停止数据采集和发送
    QMetaObject::invokeMethod(m_collector, "stopCollection", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_sender, "stop", Qt::QueuedConnection);
    // 压力控制器暂时禁用
    // m_pressureController->setEnabled(false);
    
    m_systemRunning = false;
    
    m_statusLabel->setText("系统已停止");
}

void MainWindow::updateStatusBar() {
    // 连接状态由数据采集成功/失败信号更新
    // m_plcStatusLabel 已经在 onDataCollected 和 collectionError 中更新
    
    // 显示未上报数据量（DatabaseManager 已有互斥锁保护）
    // 查询所有未上报数据以获取准确数量
    QList<PLCDataRecord> unuploaded = DatabaseManager::instance()->getUnuploadedData(10000);
    m_dataCountLabel->setText(QString("未上报: %1").arg(unuploaded.size()));
}

void MainWindow::onDataCollected(const QMap<QString, QVariant>& data) {
    m_dataView->updateData(data);
    
    // 数据采集成功，说明 PLC 连接正常
    if (!data.isEmpty() && data.size() > 1) {  // 排除只有时间戳的情况
        QString host = ConfigManager::instance()->plcHost();
        int port = ConfigManager::instance()->plcPort();
        m_plcConnectionLabel->setText(QString("● PLC: 已连接 (%1:%2)").arg(host).arg(port));
        m_plcConnectionLabel->setStyleSheet("QLabel { padding: 5px; color: #00cc00; font-weight: bold; }");
        m_plcStatusLabel->setText("PLC: 已连接");  // 同时更新右下角状态
        m_reconnectBtn->setVisible(false);
    }
    
    // 压力控制器暂时禁用
    // if (m_pressureController->isEnabled()) {
    //     QMap<QString, QVariant> controlData = m_pressureController->processData(data);
    //     if (!controlData.isEmpty()) {
    //         m_pressureController->writeToPLC(controlData);
    //     }
    // }
}
