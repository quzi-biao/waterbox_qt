cmake_minimum_required(VERSION 3.16)
project(WaterBoxQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt（优先 Qt5，回退 Qt6）
find_package(Qt5 QUIET COMPONENTS Core Widgets Network Sql Charts)
if(Qt5_FOUND)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "使用 Qt5: ${Qt5Core_VERSION}")
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql Charts)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "使用 Qt6: ${Qt6Core_VERSION}")
endif()

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
    src/core/StressTestDataGenerator.cpp
    src/core/StressTestDataGenerator.h
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
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Charts
    OpenSSL::SSL
    OpenSSL::Crypto
    ${SNAP7_LIBRARY}
    z
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY zerotier/
    DESTINATION share/waterbox/zerotier
)

install(FILES packaging/waterbox-qt.desktop
    DESTINATION share/applications
)

# CPack DEB 打包配置
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "waterbox-qt")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WaterBox Qt 水务智能监控系统")
set(CPACK_PACKAGE_CONTACT "quzi-biao")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "quzi-biao")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/packaging/postinst;${CMAKE_SOURCE_DIR}/packaging/postrm")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
