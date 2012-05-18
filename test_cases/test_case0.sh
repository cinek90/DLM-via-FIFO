#!/bin/sh
cd ../bin
# resource_id lock_type timeout(ms) sleep_time(s) iterations
./DLM-client 100 4 0 5 3
