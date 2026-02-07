# WaterBox Qt - 水务监控系统

基于 Qt 的水务监控桌面应用程序，用于 PLC 数据采集、智能控制和远程监控。

## 功能特性

### 核心功能

1. **PLC 连接**
   - 支持多种协议（S7、Modbus）
   - 可配置 PLC 地址和端口
   - 自动重连机制

2. **数据采集与上报**
   - 可配置指标（MetricIndicator），支持名称、单位、数据类型
   - 统一数据采集间隔配置（`pumpMetricReadInterval`），最小 1000ms
   - 自动数据修正和异常检测
   - 本地数据持久化（SQLite）

3. **智能压力控制**
   - 末端节能压力控制算法（公式：`press = (hst + h0 + sq) / 100`）
   - 可配置控制压力写入地址（`ctrlPressPlcAddress`）和流量数据地址（`flowPlcAddress`）
   - 压力计算参数：建筑高差(hst)、最不利点出流高差(h0)、水头损失(rate)、设计秒流量(qs)
   - 步进式压力调节，支持最小/最大压力限制
   - 基于近 30 分钟历史数据的平均流量计算

4. **远程配置**
   - HTTP 协议接收云端配置
   - 支持远程写入设置（`writeSetting` 命令）
   - 支持远程 PLC 写入
   - 配置更新后自动通知 UI 刷新

5. **数据可视化**
   - 实时数据卡片展示（名称、数值、单位、地址）
   - 历史数据曲线图
   - 多地址数据对比

### 高级功能

6. **压测模式**
   - 系统配置界面一键开启/关闭
   - 不访问 PLC，自动生成水务场景模拟数据（10 个指标：出水压力、进水压力、瞬时流量、累计流量、水箱液位、泵电流、泵频率、末端压力、泵运行状态、水温）
   - 采集间隔最小可设置为 100ms
   - 压测数据标记 `is_stress_test`，不会上报到服务端
   - 实时显示压测数据量，支持一键清理所有压测数据
   - 独立的 `StressTestDataGenerator` 类，与 `DataCollector` 解耦

7. **PLC 模拟器**
   - 内置数据模拟器
   - 模拟真实运行场景

8. **ZeroTier 集成**
   - 内置 ZeroTier 安装包
   - 自定义 planet 文件
   - 远程访问支持

## 系统架构

```
waterbox_qt/
├── src/
│   ├── core/                        # 核心模块
│   │   ├── interfaces/              # 接口定义（IPLCClient, ISmartController）
│   │   ├── ConfigManager            # 配置管理（单例）
│   │   ├── DataCollector            # 数据采集（工作线程）
│   │   ├── DataSender               # 数据上报
│   │   └── StressTestDataGenerator  # 压测数据生成器
│   ├── plc/                         # PLC 通信
│   │   ├── PLCClient                # PLC 客户端
│   │   ├── S7Protocol               # S7 协议
│   │   └── ModbusProtocol           # Modbus 协议
│   ├── database/                    # 数据库
│   │   └── DatabaseManager          # SQLite 管理（线程安全）
│   ├── network/                     # 网络通信
│   │   ├── HttpSender               # HTTP 发送
│   │   ├── HttpReporter             # HTTP 上报
│   │   ├── ServiceResponseHandle    # 服务端响应处理
│   │   ├── CommandHandler           # 远程命令处理
│   │   └── RemoteConfigReceiver     # 远程配置接收
│   ├── algorithm/                   # 算法模块
│   │   └── PressureController       # 压力控制器
│   ├── entity/                      # 数据实体
│   │   ├── MetricIndicator          # 指标定义（id, address, name, unit, dataType）
│   │   └── KeyValue                 # 键值对
│   ├── simulator/                   # 模拟器
│   │   └── PLCSimulator             # PLC 模拟器
│   └── ui/                          # 用户界面
│       ├── MainWindow               # 主窗口
│       ├── ConfigDialog             # 配置对话框
│       ├── SystemConfigWidget       # 系统配置（PLC、水箱、压力控制、压测模式）
│       ├── PLCAddressConfigWidget   # PLC 地址配置
│       ├── DataViewWidget           # 实时数据视图
│       └── HistoryChartWidget       # 历史图表
├── zerotier/                        # ZeroTier 资源
├── build_deb.sh                     # Ubuntu DEB 打包脚本
└── CMakeLists.txt
```

## 编译构建

### 依赖要求

- Qt 6.x（Core, Widgets, Network, Sql, Charts）
- OpenSSL
- CMake 3.16+
- C++17 编译器

### Ubuntu 编译

```bash
# 安装依赖
sudo apt install -y build-essential cmake \
    qt6-base-dev libqt6charts6-dev libqt6sql6-sqlite \
    libssl-dev zlib1g-dev

# 编译
cd waterbox_qt
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行
./WaterBoxQt
```

### macOS 编译

```bash
# 安装 Qt
brew install qt@6 openssl

# 编译
cd waterbox_qt
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 ..
make -j$(sysctl -n hw.ncpu)

# 运行
./WaterBoxQt
```

### 打包 DEB 安装包

```bash
# 一键打包（Ubuntu）
./build_deb.sh

# 或手动打包
cd build
cpack -G DEB

# 安装
sudo dpkg -i waterbox-qt-1.0.0-x86_64.deb
```

## 使用说明

### 系统配置

在"系统配置"标签页中可配置：

- **云端配置** - 服务端地址
- **PLC 配置** - 地址、端口、协议、数据采集间隔
- **水箱参数** - 长、宽、高、数量
- **压力控制** - 启用开关、控制压力地址、流量数据地址、压力范围、步进参数、压力计算公式参数（hst, h0, rate, qs）
- **压测模式** - 启用开关、压测数据量显示、清理按钮

### PLC 地址配置

在"PLC 地址配置"标签页中管理监控指标（MetricIndicator）：
- 每个指标包含：地址、名称、单位、数据类型（INT16/INT32/UINT16/UINT32/FLOAT32/FLOAT64/BYTE/BOOL）
- 配置存储在数据库 `kv_storage` 表中（key: `METRIC_INDICATOR_KEY`）

### 压测模式

1. 在"系统配置"中勾选"启用压测模式"
2. 可将采集间隔调低至 100ms
3. 系统自动生成 10 个水务场景模拟指标数据
4. 压测数据不会上报到服务端
5. 点击"清理压测数据"可删除所有压测记录

### 系统操作

1. **启动** - 应用启动后自动连接 PLC 并开始采集
2. **实时数据** - "实时数据"标签页展示各指标卡片
3. **历史曲线** - "历史曲线"标签页查看趋势图

## 数据流程

```
PLC 设备 → DataCollector → 数据修正 → DatabaseManager → DataSender → 云端
                ↓                          ↓
         PressureController          压测数据过滤
                ↓                    (is_stress_test=1 不上报)
           PLC 写入

压测模式：
StressTestDataGenerator → DataCollector → DatabaseManager (标记 is_stress_test)
                                ↓
                          DataViewWidget (实时展示)
```

## 配置文件

配置文件位置：`~/.local/share/WaterBoxQt/config.json`

主要配置项：
```json
{
  "plcLocalHost": "192.168.1.11",
  "plcPort": 102,
  "plcProtocol": "S7",
  "serviceAddress": "http://up.waters-ai.work",
  "pumpMetricReadInterval": 10000,
  "openPressCtrl": false,
  "ctrlPressPlcAddress": "",
  "flowPlcAddress": "",
  "minPress": 0.2,
  "defaultPress": 0.3,
  "maxPress": 0.5,
  "stressTestMode": false
}
```

## 数据库

数据库位置：`~/.local/share/WaterBoxQt/waterbox.db`

### 表结构

**plc_data** - PLC 数据记录
- id: 主键
- timestamp: 时间戳（毫秒）
- address: PLC 地址
- raw_value: 原始值
- corrected_value: 修正值
- uploaded: 是否已上报（0/1）
- is_stress_test: 是否为压测数据（0/1）

**kv_storage** - 键值存储
- key: 键（如 `METRIC_INDICATOR_KEY`、`METRIC_INDICATOR_KEY_MOCK`）
- value: 值（JSON 格式）

## 智能控制算法

末端节能压力控制算法：

```
目标压力 = (hst + h0 + sq) / 100
其中 sq = (s / (qs²)) × flow²
     s = rate - rateDelta
```

控制流程：
1. 读取当前流量（从 `flowPlcAddress` 配置的地址）
2. 计算近 30 分钟平均流量
3. 根据公式计算目标压力
4. 步进式调节写入压力（每次增减 `pressIncreaseStep`）
5. 压力值限制在 `[minPress, maxPress]` 范围内
6. 写入到 `ctrlPressPlcAddress` 配置的 PLC 地址

## ZeroTier 集成

ZeroTier 资源位于 `zerotier/` 目录：
- 安装包（离线）
- planet 文件
- 配置脚本

## 许可证

Copyright © 2024 WaterAI

## 联系方式

技术支持：support@waterai.com
