#ifndef COMMON_H
#define COMMON_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Chiavi IPC
#define KEY_MSG_QUEUE 12345
#define KEY_SHM       12346
#define KEY_SEM       12347

// Dimensione massima file
#define MAX_FILE_SIZE 65536

// Tipo di messaggio
#define MSG_REQUEST   1
#define MSG_RESPONSE  2
#define MSG_CONFIG    3
#define MSG_STATUS    4

// Algoritmi di scheduling
#define SCHED_FCFS    0
#define SCHED_SIZE    1

// Struttura messaggio
typedef struct {
    long mtype;
    int pid;
    int file_size;
    char filename[256];
} message_t;

// Struttura per stato del server
typedef struct {
    int active_processes;
    int pending_requests;
    int max_processes;
} server_status_t;

// Prototipi
void sha256_hash(const unsigned char* data, size_t len, char* output);

#endif