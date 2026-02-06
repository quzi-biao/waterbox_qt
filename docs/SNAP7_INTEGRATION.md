# 集成 Snap7 库到 Qt 项目

## 方案说明

Java 项目使用的是 `iot-communication` 库，Qt 项目推荐使用 **Snap7** 库，这是一个成熟的开源 S7 通信库。

## 安装步骤

### 1. 下载 Snap7

```bash
# macOS
brew install snap7

# 或者从源码编译
git clone https://github.com/SCADACS/snap7.git
cd snap7/build/unix
make -f x86_64_linux.mk all
sudo make -f x86_64_linux.mk install
```

### 2. 修改 CMakeLists.txt

在 `/Users/zhengbiaoxie/Workspace/water/waterbox_qt/CMakeLists.txt` 中添加：

```cmake
# 查找 Snap7 库
find_library(SNAP7_LIBRARY NAMES snap7 PATHS /usr/local/lib /usr/lib)
find_path(SNAP7_INCLUDE_DIR snap7.h PATHS /usr/local/include /usr/include)

if(SNAP7_LIBRARY AND SNAP7_INCLUDE_DIR)
    message(STATUS "Found Snap7: ${SNAP7_LIBRARY}")
    include_directories(${SNAP7_INCLUDE_DIR})
    target_link_libraries(WaterBoxQt ${SNAP7_LIBRARY})
else()
    message(WARNING "Snap7 not found, using built-in S7 protocol")
endif()
```

### 3. 使用 Snap7 API

修改 `S7Protocol.cpp` 中的 `readWithType` 方法：

```cpp
#ifdef USE_SNAP7
#include <snap7.h>

QVariant S7Protocol::readWithType(const QString& address, int dataType) {
    S7Object client = Cli_Create();
    
    // 连接到 PLC
    int result = Cli_ConnectTo(client, 
        m_host.toStdString().c_str(), 
        0,  // Rack
        1   // Slot
    );
    
    if (result != 0) {
        Cli_Destroy(&client);
        return QVariant();
    }
    
    // 解析地址 (VD1600 -> DB1, offset 1600)
    int dbNumber = 1;  // V 区域对应 DB1
    int offset = parseOffset(address);
    int length = getLengthForDataType(dataType);
    
    byte buffer[8];
    result = Cli_DBRead(client, dbNumber, offset, length, buffer);
    
    if (result == 0) {
        // 根据数据类型解析
        QVariant value = parseBuffer(buffer, dataType);
        Cli_Destroy(&client);
        return value;
    }
    
    Cli_Destroy(&client);
    return QVariant();
}
#endif
```

## 简化方案（推荐）

由于集成第三方库需要编译配置，建议使用 **模拟器模式** 进行开发测试：

1. 在配置中启用"模拟器"模式
2. PLCSimulator 会生成模拟数据
3. 等实际部署时再连接真实 PLC

## 当前状态

当前代码已经实现了基本的 S7 协议，但可能需要根据实际 PLC 型号调整。

建议：
1. 先使用模拟器模式测试整体流程
2. 确认数据采集、保存、上报功能正常
3. 再处理真实 PLC 连接问题
