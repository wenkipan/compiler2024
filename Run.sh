#!/bin/bash

# 要搜索的根目录
ROOT_DIR="test"

# 超时时间设置（以秒为单位）
TIMEOUT="150s"

# 获取当前时间并生成唯一的日志文件名
log_file="time_$(date '+%Y%m%d_%H%M%S').log"

# 最大并行作业数
MAX_JOBS=4  # 你可以根据需要调整

# 定义一个函数来处理每个 .exe 文件
process_file() {
    local exe_file="$1"
    local run_out_file="${exe_file%.*}.output"
    local expected_out_file="${exe_file%.*}.out"
    local input_file="${exe_file%.*}.in"

    # 初始化时间数组
    time_values=()

    # 执行 .exe 文件八次
    for i in {1..8}; do
    time_file="${exe_file%.*}_$i.time"

    # 执行 .exe 文件
    if [ -f "$input_file" ]; then
        # 等待 qemu-arm 完成后再继续
        cat "$input_file" | timeout $TIMEOUT qemu-arm "$exe_file" > "$run_out_file" 2> "$time_file"
    else
        timeout $TIMEOUT qemu-arm "$exe_file" > "$run_out_file" 2> "$time_file"
    fi

    wait $!
    # 确保输出文件以换行符结束
    sed -i -e '$a\' "$run_out_file"
    echo "$?" >> "$run_out_file"

    # 比较生成的 .output 文件与 .out 文件
    if ! diff "$run_out_file" "$expected_out_file" > /dev/null; then
        # 如果文件内容不同，输出 "error"
        echo "Error: Output mismatch for $exe_file" >> "$log_file"
        echo "----------------------------------------" >> "$log_file"
        return
    fi

    # 提取时间信息并添加到数组中
    if [[ $(grep -oP "TOTAL: (\d+)H-(\d+)M-(\d+)S-(\d+)us" "$time_file") =~ TOTAL:\ ([0-9]+)H-([0-9]+)M-([0-9]+)S-([0-9]+)us ]]; then
        hours="${BASH_REMATCH[1]}"
        minutes="${BASH_REMATCH[2]}"
        seconds="${BASH_REMATCH[3]}"
        microseconds="${BASH_REMATCH[4]}"
        total_microseconds=$(( (hours * 3600 + minutes * 60 + seconds) * 1000000 + microseconds ))
        time_values+=("$total_microseconds")
    fi
done


    # 如果有时间值则处理时间
    if [ ${#time_values[@]} -eq 8 ]; then
        # 排序时间数组
        IFS=$'\n' sorted_times=($(sort -n <<<"${time_values[*]}"))
        unset IFS

        # 去掉一个最大值和一个最小值
        sorted_times=("${sorted_times[@]:1:6}")

        # 计算总时间
        total_time=0
        for t in "${sorted_times[@]}"; do
            total_time=$((total_time + t))
        done

        # 输出时间信息
        echo "$exe_file: $total_time" >> "$log_file"
    fi
}

# 查找并处理所有 .exe 文件
find "$ROOT_DIR" -type f -name '*.exe' | while read -r exe_file; do
    # 控制并行作业数
    while [ $(jobs | wc -l) -ge $MAX_JOBS ]; do
        sleep 0.5
    done

    # 处理文件，使用后台作业
    process_file "$exe_file" &
done

# 等待所有后台作业完成
wait

echo "执行完成，结果已保存到 $log_file"
exit 0
