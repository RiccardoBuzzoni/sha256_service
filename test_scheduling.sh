#!/bin/bash

echo "=== Pulizia ==="
killall server 2>/dev/null
./cleanup.sh
touch /tmp/sha256_ipc

echo "=== Crea file di test ==="
dd if=/dev/zero of=file_1k.txt   bs=1K   count=1 2>/dev/null
dd if=/dev/zero of=file_10k.txt  bs=10K  count=1 2>/dev/null
dd if=/dev/zero of=file_100k.txt bs=100K count=1 2>/dev/null

# === Test FCFS ===
echo "=== TEST FCFS: ordine di arrivo (grande → medio → piccolo) ==="
./server --sched fcfs &
SERVER_PID=$!
sleep 1

echo "[Invio] file_100k.txt (100 KB)"
./client file_100k.txt

echo "[Invio] file_10k.txt (10 KB)"
./client file_10k.txt

echo "[Invio] file_1k.txt (1 KB)"
./client file_1k.txt

echo "Stato dopo invio:"
./admin get_status

sleep 2
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

# === Test SRTF ===
echo
echo "=== TEST SRTF: elabora dal più piccolo ==="
./server --sched srtf &
SERVER_PID=$!
sleep 1

echo "[Invio] file_100k.txt (100 KB)"
./client file_100k.txt

echo "[Invio] file_10k.txt (10 KB)"
./client file_10k.txt

echo "[Invio] file_1k.txt (1 KB)"
./client file_1k.txt

echo "Stato dopo invio:"
./admin get_status

sleep 2
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "✅ Test completati."