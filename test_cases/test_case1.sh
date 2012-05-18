#!/bin/sh
cd ../bin
# resource_id lock_type timeout(ms) sleep_time(s) iterations
./DLM-client 100 4 4000 5 3 &
arr[0]=$!
./DLM-client 100 3 3000 5 3 &
arr[1]=$!
for pid in $arr
do
	wait $pid
done
