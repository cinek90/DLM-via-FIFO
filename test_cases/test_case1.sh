#!/bin/bash
cd ../bin
# resource_id lock_type timeout(ms) sleep_time(s) iterations
./DLM-client 100 4 6000 5 3 &
arr[0]=$!
./DLM-client 100 3 6000 5 3 &
arr[1]=$!
for pid in $arr
do
	wait $pid
done
