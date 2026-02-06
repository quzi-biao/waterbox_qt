cmake_minimum_required(VERSION 3.16)
project(WaterBoxQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Charts)

# 查找 OpenSSL
find_package(OpenSSL REQUIRED)

# Snap7 Simple
include_directories(${CMAKE_SOURCE_DIR}/third_party/snap7_minimal)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${OPENSSL_INCLUDE_DIR}
)

set(SOURCES
    src/main.cpp
    src/core/interfaces/IPLCClient.h
    src/core/interfaces/IDataReporter.h
    src/core/interfaces/ISmartController.h
    src/core/ConfigManager.cpp
    src/core/ConfigManager.h
    src/plc/PLCClient.cpp
    src/plc/PLCClient.h
    src/plc/S7Protocol.cpp
    src/plc/S7Protocol.h
    src/plc/ModbusProtocol.cpp
    src/plc/ModbusProtocol.h
    src/database/DatabaseManager.cpp
    src/database/DatabaseManager.h
    src/network/HttpReporter.cpp
    src/network/HttpReporter.h
    src/network/RemoteConfigReceiver.cpp
    src/network/RemoteConfigReceiver.h
    src/network/HttpSender.cpp
    src/network/HttpSender.h
    src/network/HttpSendData.cpp
    src/network/HttpSendData.h
    src/network/ServiceResponseHandle.cpp
    src/network/ServiceResponseHandle.h
    src/network/CommandHandler.cpp
    src/network/CommandHandler.h
    src/common/RSAUtils.cpp
    src/common/RSAUtils.h
    src/common/AESUtils.cpp
    src/common/AESUtils.h
    src/common/NetworkUtils.cpp
    src/common/NetworkUtils.h
    src/monitor/SystemMonitor.cpp
    src/monitor/SystemMonitor.h
    src/entity/MetricIndicator.cpp
    src/entity/MetricIndicator.h
    src/entity/KeyValue.cpp
    src/entity/KeyValue.h
    src/algorithm/PressureController.cpp
    src/algorithm/PressureController.h
    src/core/DataCollector.cpp
    src/core/DataCollector.h
    src/core/DataSender.cpp
    src/core/DataSender.h
    src/simulator/PLCSimulator.cpp
    src/simulator/PLCSimulator.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/ConfigDialog.cpp
    src/ui/ConfigDialog.h
    src/ui/SystemConfigWidget.cpp
    src/ui/SystemConfigWidget.h
    src/ui/PLCAddressConfigWidget.cpp
    src/ui/PLCAddressConfigWidget.h
    src/ui/DataViewWidget.cpp
    src/ui/DataViewWidget.h
    src/ui/HistoryChartWidget.cpp
    src/ui/HistoryChartWidget.h
    third_party/snap7_minimal/snap7_simple.cpp
    third_party/snap7_minimal/snap7_simple.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Charts
    OpenSSL::SSL
    OpenSSL::Crypto
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY zerotier/
    DESTINATION share/waterbox/zerotier
)
