#!/bin/bash

# 定义函数，递归遍历文件夹中的所有 .ans 和 .out 文件
function compare_files {
    local dir="$1"

    # 遍历当前文件夹中的文件和子文件夹
    for file in "$dir"/*; do
        if [[ -d "$file" ]]; then
            # 如果是文件夹，则递归调用函数处理子文件夹中的文件
            compare_files "$file"
        elif [[ -f "$file" && ("$file" == *.ans)]]; then
            # 如果是 .ans 或 .out 文件，则进行对比
            compare_file="${file::-7}.out"  # 获取文件扩展名

            # 检查对应的文件是否存在
            if [[ -f "$compare_file" ]]; then
                # 对比文件内容并输出不同之处
                diff_result=$(diff -ZBu "$file" "$compare_file")
                if [[ -n "$diff_result" ]]; then
                    echo "$file is different:"
                    # echo "$diff_result"
                else 
                    echo "$file is same"
                fi
            else
                echo "Counterpart file $counterpart_file does not exist for $compare_file"
            fi
        fi
    done
}

# 调用函数，传入要处理的文件夹路径
compare_files "$1"
