#!/bin/bash
echo "Pulizia risorse IPC..."
KEY=$(ftok /tmp/sha256_ipc S 2>/dev/null || echo "")
if [ -n "$KEY" ]; then
    ipcs -q | grep $KEY > /dev/null && ipcrm -Q $KEY && echo "Coda rimossa"
    ipcs -m | grep $KEY > /dev/null && ipcrm -M $KEY && echo "Memoria condivisa rimossa"
    ipcs -s | grep $KEY > /dev/null && ipcrm -s $KEY && echo "Semafori rimossi"
fi
echo "Pulizia completata."