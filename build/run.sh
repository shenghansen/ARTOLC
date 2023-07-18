#!/bin/bash

# 设置循环次数
n=10

  # 循环访问x的值，范围为0到3
for x in 0 1 2 3; do
    # 循环执行命令
    for i in $(seq 1 $n); do
    # 执行命令
    echo "n=100000000,第$i次执行，x=$x\n" >> result.txt
    ./artolc 100000000 $x >> result.txt 2>&1
  done
done