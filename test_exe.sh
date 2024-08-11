#!/bin/bash
TIMEOUT="timeout 150s"

test_file=$1

# 获取文件名和扩展名
base_name="${test_file%.*}"

# 初始化计数器
for i in {0..7}; do
    input_file="${base_name}.in"
    time_file="${base_name}_run_$i.time"
    if [ "$i" -eq 0 ]; then
        run_out_file="${base_name}.output"
        if [ -f "$input_file" ]; then
            cat "$input_file" | $TIMEOUT qemu-arm "$test_file" > "$run_out_file" 2> "$time_file"
        else
            # 直接执行 qemu-arm，没有输入文件
            $TIMEOUT qemu-arm "$test_file" > "$run_out_file" 2> "$time_file"
        fi
    else
            if [ -f "$input_file" ]; then
            cat "$input_file" | $TIMEOUT qemu-arm "$test_file"  2> "$time_file"
        else
            # 直接执行 qemu-arm，没有输入文件
            $TIMEOUT qemu-arm "$test_file"  2> "$time_file"
        fi
    fi

    

    

    # 等待 qemu-arm 执行完成
    wait $!
    output=$?

    # 确保输出文件以换行符结束
    if [ "$i" -eq 0 ]; then
        sed -i -e '$a\' "$run_out_file"
        echo "$output" >> "$run_out_file"
    fi
done

exit 0
