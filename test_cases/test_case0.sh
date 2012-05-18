#!/bin/sh
cd ../bin
# resource_id lock_type timeout sleep_time iterations
./DLM-client 100 4 0 5 3
