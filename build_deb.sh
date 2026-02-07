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

# ===== 容器内执行编译和打包 =====
if [ -f /.dockerenv ] || [ "${IN_DOCKER:-0}" = "1" ]; then

    export DEBIAN_FRONTEND=noninteractive

    echo "[1/5] 安装编译依赖..."
    apt update
    apt install -y build-essential cmake \
        qt6-base-dev libqt6charts6-dev libqt6sql6-sqlite \
        libssl-dev zlib1g-dev \
        file wget fuse libfuse2

    echo "[2/5] 编译项目..."
    cd /src
    rm -rf build && mkdir -p build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make -j$(nproc)
    make install DESTDIR=/src/AppDir

    echo "[3/5] 准备 AppDir..."
    mkdir -p /src/AppDir/usr/share/applications
    mkdir -p /src/AppDir/usr/share/icons/hicolor/256x256/apps

    # 创建 .desktop 文件
    cat > /src/AppDir/usr/share/applications/${APP_NAME}.desktop << EOF
[Desktop Entry]
Type=Application
Name=WaterBox Qt
Comment=水务智能监控系统
Exec=${APP_NAME}
Icon=${APP_NAME}
Categories=Utility;
Terminal=false
EOF

    # 复制 desktop 文件到 AppDir 根目录（linuxdeploy 需要）
    cp /src/AppDir/usr/share/applications/${APP_NAME}.desktop /src/AppDir/

    # 创建一个简单的图标（如果没有自定义图标）
    if [ ! -f /src/AppDir/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png ]; then
        # 生成一个占位图标
        apt install -y imagemagick 2>/dev/null || true
        if command -v convert &>/dev/null; then
            convert -size 256x256 xc:#1976D2 \
                -fill white -gravity center -pointsize 48 -annotate 0 "WB" \
                /src/AppDir/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png
        else
            # 最小 PNG（1x1 蓝色像素）作为占位
            printf '\x89PNG\r\n\x1a\n' > /src/AppDir/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png
        fi
    fi
    cp /src/AppDir/usr/share/icons/hicolor/256x256/apps/${APP_NAME}.png /src/AppDir/

    echo "[4/5] 下载 linuxdeploy 并生成 AppImage..."
    cd /src

    # 下载 linuxdeploy
    if [ ! -f linuxdeploy-x86_64.AppImage ]; then
        wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
        chmod +x linuxdeploy-x86_64.AppImage
    fi

    # 下载 Qt 插件
    if [ ! -f linuxdeploy-plugin-qt-x86_64.AppImage ]; then
        wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
        chmod +x linuxdeploy-plugin-qt-x86_64.AppImage
    fi

    # 生成 AppImage
    export QMAKE=/usr/bin/qmake6
    export EXTRA_QT_PLUGINS="sqldrivers;iconengines"
    ./linuxdeploy-x86_64.AppImage \
        --appimage-extract-and-run \
        --appdir AppDir \
        --plugin qt \
        --output appimage

    echo "[5/5] 打包完成"
    APPIMAGE_FILE=$(ls *.AppImage 2>/dev/null | grep -i waterbox | head -1)
    if [ -n "$APPIMAGE_FILE" ]; then
        # 重命名为统一格式
        FINAL_NAME="${APP_NAME}-${APP_VERSION}-x86_64.AppImage"
        mv "$APPIMAGE_FILE" "/src/${FINAL_NAME}"
        echo "生成文件: /src/${FINAL_NAME}"
    else
        echo "错误: 未找到 AppImage 文件"
        exit 1
    fi

else
    # ===== 宿主机：通过 Docker 编译 =====
    if ! command -v docker &>/dev/null; then
        echo "错误: 需要 Docker"
        echo "请安装 Docker: https://docs.docker.com/engine/install/"
        exit 1
    fi

    echo "使用 Docker (Ubuntu 22.04) 编译打包..."
    docker run --rm \
        --privileged \
        -v "$SCRIPT_DIR":/src \
        -e IN_DOCKER=1 \
        ubuntu:22.04 \
        bash /src/build_deb.sh

    APPIMAGE_FILE=$(ls "$SCRIPT_DIR"/*.AppImage 2>/dev/null | head -1)
    if [ -n "$APPIMAGE_FILE" ]; then
        echo ""
        echo "========================================="
        echo "打包成功！"
        echo "文件: $APPIMAGE_FILE"
        echo ""
        echo "使用方式:"
        echo "  chmod +x $(basename $APPIMAGE_FILE)"
        echo "  ./$(basename $APPIMAGE_FILE)"
        echo "========================================="

        # 上传到阿里云 OSS
        if [ "$OSS_ENABLED" = true ]; then
            upload_to_oss "$APPIMAGE_FILE"
        else
            echo ""
            echo "未配置 OSS 参数，跳过上传。"
            echo "添加 --endpoint --key-id --key-secret --bucket 参数可自动上传。"
        fi
    else
        echo "错误: 未找到 AppImage 文件"
        exit 1
    fi
fi
