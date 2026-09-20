#!/bin/bash
set -e

sudo tcpdump -i lo0 -w rpc_capture.pcap &
TCPDUMP_PID=$!
sleep 1

./portmapper > portmapper.out 2>&1 &
PM_PID=$!
sleep 1

./demo_service > service.out 2>&1 &
SVC_PID=$!
sleep 1

./onc_client > client.out 2>&1

sleep 1
kill -9 $PM_PID $SVC_PID 2>/dev/null
sudo kill $TCPDUMP_PID
sleep 1

echo "===== portmapper output ====="
cat portmapper.out
echo ""
echo "===== service output ====="
cat service.out
echo ""
echo "===== client output ====="
cat client.out
