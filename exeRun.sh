#!/bin/bash

# 指定要运行的可执行文件所在的目录
function process_files {
    local dir="$1"

    # 遍历指定目录下的所有可执行文件，并逐个运行
    for executable in "$dir"/*; do
        if [[ -x "$executable" && -f "$executable" && ("$executable" == *.exe)]]; then
            # 提取可执行文件的名称
            filename=$(basename "$executable")

            # 检查是否存在与可执行文件同名的 .in 文件
            input_file="${executable::-7}.in"

            if [[ -f "$input_file" ]]; then
                echo "Running executable: $executable"
                timeout 5m cat "$input_file" | ./"$executable" > "${executable%.exe}.ans"
            else
                echo "Running executable: $executable"
                timeout 5m ./"$executable" > "${executable%.exe}.ans"
            fi
            echo $? > "$ret.tmp"
            echo >> "${executable%.exe}.ans"
            cat "$ret.tmp" >> "${executable%.exe}.ans"
        elif [[ -d "$executable" ]]; then
                # 如果是文件夹，则递归调用函数处理子文件夹中的文件
                process_files "$executable"
        fi
    done
}

process_files "$1"

