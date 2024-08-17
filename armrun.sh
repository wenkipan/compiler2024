#!/bin/bash
# 为方便测试一个.sy 文件的正确性
#./run.sh *.sy: 运行我们的文件
#./run.sh *.sy arm: 用交叉编译器运行
LIB_DIR=lib
LIB_FILE=sysy
INCLUDE=sylib.h


cat tmp1.s
arm-linux-gnueabihf-gcc -static ./tmp1.s -L$LIB_DIR -l$LIB_FILE -o ./tmp.out || exit 1
qemu-arm ./tmp.out<tst.in.txt || exit 1
echo "exit: $?"