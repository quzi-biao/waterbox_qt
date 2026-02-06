# WaterBox Qt - 水务监控系统

基于 Qt 的水务监控桌面应用程序，用于 PLC 数据采集、智能控制和远程监控。

## 功能特性

### 核心功能

1. **PLC 连接**
   - 支持多种协议（S7、Modbus）
   - 可配置 PLC 地址和端口
   - 自动重连机制

2. **数据采集与上报**
   - 可配置数据 Schema
   - 自动数据修正和异常检测
   - 智能上报频率调整
   - 本地数据持久化（SQLite）

3. **智能控制**
   - 末端节能压力控制算法
   - 基于历史数据的智能决策
   - 可配置控制参数

4. **远程配置**
   - HTTP 协议接收云端配置
   - 动态更新系统参数
   - 支持远程 PLC 写入

5. **数据可视化**
   - 实时数据展示
   - 历史数据曲线图
   - 多地址数据对比

### 高级功能

6. **PLC 模拟器**
   - 内置数据模拟器
   - 模拟真实运行场景
   - 支持压力测试

7. **压力测试**
   - 可调整采集间隔（最小 0.5s）
   - 系统性能测试
   - 数据吞吐量监控

8. **ZeroTier 集成**
   - 内置 ZeroTier 安装包
   - 自定义 planet 文件
   - 远程访问支持

## 系统架构

```
waterbox_qt/
├── src/
│   ├── core/              # 核心模块
│   │   ├── interfaces/    # 接口定义
│   │   ├── ConfigManager  # 配置管理
│   │   ├── DataCollector  # 数据采集
│   │   └── DataSender     # 数据上报
│   ├── plc/               # PLC 通信
│   │   ├── PLCClient      # PLC 客户端
│   │   ├── S7Protocol     # S7 协议
│   │   └── ModbusProtocol # Modbus 协议
│   ├── database/          # 数据库
│   │   └── DatabaseManager # SQLite 管理
│   ├── network/           # 网络通信
│   │   ├── HttpReporter   # HTTP 上报
│   │   └── RemoteConfigReceiver # 远程配置
│   ├── algorithm/         # 算法模块
│   │   └── PressureController # 压力控制
│   ├── simulator/         # 模拟器
│   │   └── PLCSimulator   # PLC 模拟器
│   └── ui/                # 用户界面
│       ├── MainWindow     # 主窗口
│       ├── ConfigDialog   # 配置对话框
│       ├── DataViewWidget # 数据视图
│       └── HistoryChartWidget # 历史图表
└── zerotier/              # ZeroTier 资源
```

## 编译构建

### 依赖要求

- Qt 6.x
- CMake 3.16+
- C++17 编译器

### Ubuntu 编译

```bash
# 安装依赖
sudo apt install qt6-base-dev qt6-charts-dev cmake build-essential

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
brew install qt@6

# 编译
cd waterbox_qt
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6 ..
make -j$(sysctl -n hw.ncpu)

# 运行
./WaterBoxQt
```

## 使用说明

### 基本配置

1. **PLC 配置**
   - 打开"文件" -> "配置"
   - 设置 PLC 地址、端口和协议
   - 配置数据地址映射

2. **数据上报配置**
   - 设置上报 URL
   - 配置上报间隔
   - 选择上报协议

3. **控制器配置**
   - 设置控制压力地址
   - 配置末端压力地址
   - 设置流量监测地址

### 系统操作

1. **连接 PLC**
   - 点击"连接 PLC"按钮
   - 等待连接成功提示

2. **启动系统**
   - 点击"启动系统"
   - 系统开始自动采集和上报

3. **读取数据**
   - 点击"读取数据"按钮
   - 查看当前 PLC 数据

4. **查看历史**
   - 切换到"历史曲线"标签
   - 选择地址和时间范围
   - 点击"加载数据"

### 测试功能

1. **模拟器模式**
   - 点击"模拟器"按钮
   - 系统使用模拟数据运行
   - 适合开发和测试

2. **压力测试**
   - 点击"压力测试"按钮
   - 系统以高频率运行
   - 监控性能指标

## 数据流程

```
PLC 设备 → 数据采集 → 数据修正 → 本地存储 → 数据上报 → 云端
                ↓
            智能控制 → PLC 写入
```

每 10 秒（可配置）：
1. 从 PLC 读取数据
2. 数据修正和异常检测
3. 保存到本地数据库
4. 上报到云端
5. 执行智能控制算法
6. 写入控制值到 PLC

## 配置文件

配置文件位置：`~/.local/share/WaterBoxQt/config.json`

示例配置：
```json
{
  "plc_host": "192.168.1.100",
  "plc_port": 102,
  "plc_protocol": "S7",
  "report_url": "http://server.com/api/data",
  "collect_interval": 10000,
  "report_interval": 10000,
  "ctrl_pressure_addr": "V100",
  "end_pressure_addr": "V200",
  "flow_addr": "V300"
}
```

## 数据库

数据库位置：`~/.local/share/WaterBoxQt/waterbox.db`

### 表结构

**plc_data** - PLC 数据记录
- id: 主键
- timestamp: 时间戳
- address: PLC 地址
- raw_value: 原始值
- corrected_value: 修正值
- uploaded: 是否已上报

**kv_storage** - 键值存储
- key: 键
- value: 值

## 智能控制算法

末端节能压力控制算法：
1. 读取末端压力和流量
2. 计算昨日平均流量
3. 计算 7 日平均流量
4. 根据流量变化调整目标压力
5. 平滑写入控制压力

## ZeroTier 集成

ZeroTier 资源位于 `zerotier/` 目录：
- 安装包（离线）
- planet 文件
- 配置脚本

## 许可证

Copyright © 2024 WaterAI

## 联系方式

技术支持：support@waterai.com
