#include "sha256_ipc.h"
#include <openssl/sha.h>  // Richiesto per SHA256_CTX

void compute_sha256(const unsigned char* data, size_t len, char* output) {
    SHA256_CTX ctx;
    unsigned char hash[32];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash, &ctx);

    for (int i = 0; i < 32; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';
}

int init_shm() {
    int shm_id = shmget(SHM_KEY, MAX_FILE_SIZE, 0666 | IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }
    return shm_id;
}

int init_msg_queue() {
    int msg_id = msgget(MSG_KEY, 0666 | IPC_CREAT);
    if (msg_id == -1) {
        perror("msgget");
        exit(1);
    }
    return msg_id;
}

int init_semaphores() {
    int sem_id = semget(SEM_KEY, 3, 0666 | IPC_CREAT);
    if (sem_id == -1) {
        perror("semget");
        exit(1);
    }

    unsigned short vals[] = {1, 1, 1};
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.array = vals;
    if (semctl(sem_id, 0, SETALL, arg) == -1) {
        perror("semctl SETALL");
        exit(1);
    }
    return sem_id;
}

void sem_wait_op(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, -1, SEM_UNDO};
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop wait");
    }
}

void sem_signal_op(int sem_id, int sem_num) {
    struct sembuf op = {sem_num, 1, SEM_UNDO};
    if (semop(sem_id, &op, 1) == -1) {
        perror("semop signal");
    }
}