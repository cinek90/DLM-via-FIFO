#!/bin/bash
cd ../bin
# resource_id lock_type timeout(ms) sleep_time(s) iterations
./DLM-client 100 3 600 2 3 &
arr[0]=$!
./DLM-client 100 3 600 2 3 &
arr[1]=$!
./DLM-client 100 3 600 2 3 &
arr[2]=$!
./DLM-client 100 3 6000 2 2 &
arr[3]=$!
./DLM-client 100 3 6000 2 3 &
arr[4]=$!
./DLM-client 1000 3 6000 2 2 &
arr[5]=$!
./DLM-client 1000 3 600 2 2 &
arr[6]=$!
./DLM-client 1000 3 600 3 2 &
arr[7]=$!
for pid in $arr
do
	wait $pid
done
