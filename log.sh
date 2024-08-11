#!/bin/bash

# 要搜索的根目录
ROOT_DIR="test"
log_file="time_$(date '+%Y%m%d_%H%M%S').log"

# 查找所有 _0.time 文件并处理
find "$ROOT_DIR" -type f -name '*_0.time' | while read -r base_file; do
    # 获取文件的前缀
    base_prefix="${base_file%_0.time}"
    
    # 初始化时间数组
    time_values=()

    # 提取时间信息并添加到数组中
    for i in {0..7}; do
        time_file="${base_prefix}_$i.time"
        if [[ -f "$time_file" ]]; then
            if [[ $(grep -oP "TOTAL: (\d+)H-(\d+)M-(\d+)S-(\d+)us" "$time_file") =~ TOTAL:\ ([0-9]+)H-([0-9]+)M-([0-9]+)S-([0-9]+)us ]]; then
                hours="${BASH_REMATCH[1]}"
                minutes="${BASH_REMATCH[2]}"
                seconds="${BASH_REMATCH[3]}"
                microseconds="${BASH_REMATCH[4]}"
                total_microseconds=$(( (hours * 3600 + minutes * 60 + seconds) * 1000000 + microseconds ))
                time_values+=("$total_microseconds")
            fi
        fi
    done

    # 确保有8个时间值
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

        # 输出时间信息到日志文件
        echo "$base_prefix: $total_time" >> "$log_file"
    else
        echo "未找到8个时间文件或提取失败：$base_prefix"
    fi
done

echo "处理完成，结果已保存到 $log_file"
