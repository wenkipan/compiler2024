#!/bin/bash

# 定义函数 process_files
function process_files {
    local dir="$1"

    # 遍历当前文件夹中的文件和子文件夹
    for file in "$dir"/*; do
        if [[ -f "$file" && "$file" == *.ll ]]; then
            # 如果是 .ll 文件，则进行操作
            echo "Processing file: $file"

            # 获取输出文件名（去除后缀 .ll，并加上 .bc 后缀）
            output_file="${file%.ll}.bc" 

            # 将 .ll 文件转换为 .bc 文件
           # script -c "llvm-as $file -o $output_file"  
           echo "Processing file: $file" 1>>error.log
           llvm-as "$file" -o "$output_file" 1>>error.log 2>>error.log

            output_file1="${output_file%.bc}.s" 

            # 使用 llc 将 .bc 文件编译成目标平台的汇编代码
            llc "$output_file" -o "$output_file1"

            output_file2="${output_file1%.s}.exe"

            # 使用 gcc 将 .s 文件编译成可执行文件
            gcc "$output_file1" -o "$output_file2" -L./lib -l:sylib.o 2>/dev/null
        elif [[ -d "$file" ]]; then
            # 如果是文件夹，则递归调用函数处理子文件夹中的文件
            process_files "$file"
        fi
    done
}

# 调用函数 process_files，传入要处理的文件夹路径
process_files "$1"
