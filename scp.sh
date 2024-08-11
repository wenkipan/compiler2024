#!/bin/bash

# 树莓派的用户名和IP地址
USER=pi
RASPBERRY_PI_IP=192.168.112.17  # 树莓派IP地址

# 本地和树莓派的根目录
LOCAL_ROOT_DIR=test/2024
REMOTE_ROOT_DIR=test/2024

# 创建一个临时文件列表
FILE_LIST=$(mktemp)

# 查找本地目录中的所有 .s 文件并将路径保存到临时文件列表中
find "$LOCAL_ROOT_DIR" -type f -name '*.exe' > "$FILE_LIST"

# 计算远程路径并创建对应的远程目录
while read -r FILE; do
    # 计算文件的相对路径
    RELATIVE_PATH="${FILE#$LOCAL_ROOT_DIR/}"
    # 计算远程路径
    REMOTE_PATH="$REMOTE_ROOT_DIR/$(dirname "$RELATIVE_PATH")"
    # 创建远程目录（如果不存在）
    ssh "$USER@$RASPBERRY_PI_IP" "mkdir -p $REMOTE_PATH"
done < "$FILE_LIST"

# 使用 scp 将所有文件统一发送到树莓派
scp $(cat "$FILE_LIST") "$USER@$RASPBERRY_PI_IP:$REMOTE_ROOT_DIR"

# 删除临时文件
rm "$FILE_LIST"
