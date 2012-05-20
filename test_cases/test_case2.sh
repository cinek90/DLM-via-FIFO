#!/bin/bash
cd ../bin
# resource_id lock_type timeout(ms) sleep_time(s) iterations
echo "testowanie funkcji try_lock"
./DLM-client 100 4 -2 2 3 &
arr[0]=$!
./DLM-client 100 3 -2 2 3 &
arr[1]=$!
for pid in $arr
do
        wait $pid
done
