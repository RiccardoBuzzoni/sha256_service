#ifndef SHA256_IPC_H
#define SHA256_IPC_H

#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <openssl/sha.h>  // Per SHA256_CTX

#define MAX_FILE_SIZE (10 * 1024 * 1024)  // 10 MB
#define SHM_KEY ftok("/tmp/sha256_ipc", 'S')
#define MSG_KEY ftok("/tmp/sha256_ipc", 'M')
#define SEM_KEY ftok("/tmp/sha256_ipc", 'E')

// Tipi di messaggio
#define MSG_REQUEST  1
#define MSG_RESPONSE 2
#define MSG_ADMIN    3

// Comandi admin
#define CMD_SET_LIMIT 100
#define CMD_GET_STATUS 101

// Semafori
#define SEM_WRITE     0  // accesso in scrittura (mutua esclusione)
#define SEM_BUFFER    1  // 1 = libero, 0 = pieno
#define SEM_QUEUE     2  // mutua esclusione sulla coda

struct msg_buffer {
    long msg_type;
    pid_t client_pid;
    char filename[256];
    size_t file_size;
    char hash[65];
    int cmd;
    int data;
};

// Funzioni di utilità
void compute_sha256(const unsigned char* data, size_t len, char* output);
int init_shm();
int init_msg_queue();
int init_semaphores();
void sem_wait_op(int sem_id, int sem_num);
void sem_signal_op(int sem_id, int sem_num);

#endif