#!/bin/bash
cd ../bin
# resource_id lock_type timeout(ms) sleep_time(s) iterations
echo "testowanie funkcji try_lock"
./DLM-client 100 1 0 5 3 &
arr[0]=$!
./DLM-client 100 1 -2 5 3 &
arr[1]=$!
for pid in $arr
do
        wait $pid
done
