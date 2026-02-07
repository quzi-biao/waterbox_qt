#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="WaterBoxQt"
APP_VERSION="1.0.0"

# ===== 参数解析 =====
OSS_ENDPOINT=""
OSS_ACCESS_KEY_ID=""
OSS_ACCESS_KEY_SECRET=""
OSS_BUCKET=""
OSS_UPLOAD_DIR="releases"

usage() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --endpoint <endpoint>       OSS Endpoint（如 oss-cn-hangzhou.aliyuncs.com）"
    echo "  --key-id <accessKeyId>      OSS AccessKey ID"
    echo "  --key-secret <secret>       OSS AccessKey Secret"
    echo "  --bucket <bucketName>       OSS Bucket 名称"
    echo "  --upload-dir <dir>          OSS 上传目录（默认: releases）"
    echo "  -h, --help                  显示帮助"
    echo ""
    echo "示例:"
    echo "  $0 --endpoint oss-cn-hangzhou.aliyuncs.com --key-id LTAI5xxx --key-secret jX7xxx --bucket my-bucket"
    echo ""
    echo "如果不提供 OSS 参数，则只编译打包不上传。"
    exit 0
}

while [ $# -gt 0 ]; do
    case "$1" in
        --endpoint)     OSS_ENDPOINT="$2"; shift 2 ;;
        --key-id)       OSS_ACCESS_KEY_ID="$2"; shift 2 ;;
        --key-secret)   OSS_ACCESS_KEY_SECRET="$2"; shift 2 ;;
        --bucket)       OSS_BUCKET="$2"; shift 2 ;;
        --upload-dir)   OSS_UPLOAD_DIR="$2"; shift 2 ;;
        -h|--help)      usage ;;
        *)              echo "未知参数: $1"; usage ;;
    esac
done

OSS_ENABLED=false
if [ -n "$OSS_ENDPOINT" ] && [ -n "$OSS_ACCESS_KEY_ID" ] && [ -n "$OSS_ACCESS_KEY_SECRET" ] && [ -n "$OSS_BUCKET" ]; then
    OSS_ENABLED=true
fi

upload_to_oss() {
    local FILE_PATH="$1"
    local FILE_NAME=$(basename "$FILE_PATH")
    local OSS_KEY="${OSS_UPLOAD_DIR}/${FILE_NAME}"
    local CONTENT_TYPE="application/octet-stream"
    local DATE=$(date -u "+%a, %d %b %Y %H:%M:%S GMT")

    echo ""
    echo "[上传] 正在上传到阿里云 OSS..."

    # 构造签名字符串
    local STRING_TO_SIGN="PUT\n\n${CONTENT_TYPE}\n${DATE}\n/${OSS_BUCKET}/${OSS_KEY}"
    local SIGNATURE=$(printf "%b" "$STRING_TO_SIGN" | openssl dgst -sha1 -hmac "$OSS_ACCESS_KEY_SECRET" -binary | base64)

    # 上传
    local OSS_URL="https://${OSS_BUCKET}.${OSS_ENDPOINT}/${OSS_KEY}"
    HTTP_CODE=$(curl -s -o /dev/null -w "%{http_code}" -X PUT \
        -H "Date: ${DATE}" \
        -H "Content-Type: ${CONTENT_TYPE}" \
        -H "Authorization: OSS ${OSS_ACCESS_KEY_ID}:${SIGNATURE}" \
        --data-binary "@${FILE_PATH}" \
        "${OSS_URL}")

    if [ "$HTTP_CODE" = "200" ]; then
        echo ""
        echo "========================================="
        echo "上传成功！"
        echo "下载链接: ${OSS_URL}"
        echo "========================================="
    else
        echo "上传失败，HTTP 状态码: ${HTTP_CODE}"
        # 打印详细错误
        curl -s -X PUT \
            -H "Date: ${DATE}" \
            -H "Content-Type: ${CONTENT_TYPE}" \
            -H "Authorization: OSS ${OSS_ACCESS_KEY_ID}:${SIGNATURE}" \
            --data-binary "@${FILE_PATH}" \
            "${OSS_URL}"
        echo ""
        echo "文件仍保留在本地: ${FILE_PATH}"
    fi
}

echo "=== WaterBox Qt 打包脚本 ==="

# ===== 安装依赖 =====
echo "[1/3] 安装编译依赖..."
sudo apt update
sudo apt install -y build-essential cmake dpkg-dev file \
    qtbase5-dev libqt5charts5-dev libqt5sql5-sqlite \
    libssl-dev zlib1g-dev

# ===== 编译 =====
echo "[2/3] 编译项目..."
cd "$SCRIPT_DIR"
rm -rf build && mkdir -p build && cd build
cmake ..
make -j$(nproc)

# ===== 打包 =====
echo "[3/3] 生成 DEB 包..."
cpack -G DEB

DEB_FILE=$(ls *.deb 2>/dev/null | head -1)
if [ -z "$DEB_FILE" ]; then
    echo "错误: 未找到 .deb 文件"
    exit 1
fi

DEB_PATH="$(pwd)/$DEB_FILE"
echo ""
echo "========================================="
echo "打包成功！"
echo "文件: $DEB_PATH"
echo "安装命令: sudo dpkg -i $DEB_FILE"
echo "========================================="

# 上传到阿里云 OSS
if [ "$OSS_ENABLED" = true ]; then
    upload_to_oss "$DEB_PATH"
else
    echo ""
    echo "未配置 OSS 参数，跳过上传。"
    echo "添加 --endpoint --key-id --key-secret --bucket 参数可自动上传。"
fi
