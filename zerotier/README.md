# ZeroTier 集成说明

## 目录结构

```
zerotier/
├── install.sh          # 安装脚本
├── planet              # 自定义 planet 文件
└── zerotier-one/       # ZeroTier 安装包（离线）
```

## 安装步骤

### Ubuntu 安装

```bash
cd zerotier
sudo ./install.sh
```

### 手动安装

1. 解压安装包
```bash
tar -xzf zerotier-one.tar.gz
```

2. 安装
```bash
sudo dpkg -i zerotier-one_*.deb
```

3. 复制 planet 文件
```bash
sudo cp planet /var/lib/zerotier-one/
```

4. 重启服务
```bash
sudo systemctl restart zerotier-one
```

## 加入网络

```bash
sudo zerotier-cli join <NETWORK_ID>
```

## 查看状态

```bash
sudo zerotier-cli status
sudo zerotier-cli listnetworks
```

## 注意事项

- planet 文件包含自定义根服务器配置
- 安装包为离线版本，无需联网下载
- 确保防火墙允许 UDP 9993 端口
