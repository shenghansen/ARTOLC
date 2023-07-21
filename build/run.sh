#!/bin/bash

# 设置循环次数
n=10
  # 循环访问x的值，范围为0到3
for x in 0 1 2; do
    # 循环执行命令
    for i in $(seq 1 $n); do
    # 执行命令
    echo "n=100000000,第$i次执行，x=$x\n" >> result.txt
    echo "different numa node\n" >> result.txt
    ./test 100000000 $x 0 >> result.txt 2>&1
    echo "same numa node\n" >> result.txt
    ./test 100000000 $x 4 >> result.txt 2>&1
  done
done




# for x in 0 1 2 3; do
#     echo "n=10000000,x=$x" >> result.txt
#    ./test 10000000 $x >> result.txt 2>&1
# done