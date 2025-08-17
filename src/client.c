#include "sha256_ipc.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File troppo grande (max 10MB)\n");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    unsigned char* buffer = malloc(file_size);
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    int msg_id = msgget(MSG_KEY, 0666);
    int shm_id = shmget(SHM_KEY, MAX_FILE_SIZE, 0666);
    int sem_id = semget(SEM_KEY, 3, 0666);

    if (msg_id == -1 || shm_id == -1 || sem_id == -1) {
        perror("Accesso alle risorse IPC fallito");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    // 1. Aspetta che il buffer sia libero
    sem_wait_op(sem_id, SEM_BUFFER);

    // 2. Carica in memoria condivisa
    void* shm_ptr = shmat(shm_id, NULL, 0);
    memcpy(shm_ptr, buffer, file_size);
    shmdt(shm_ptr);

    // 3. Segnala che il buffer è pieno
    sem_signal_op(sem_id, SEM_BUFFER);

    // 4. Invia richiesta
    struct msg_buffer msg = {0};
    msg.msg_type = MSG_REQUEST;
    msg.client_pid = getpid();
    strcpy(msg.filename, filename);
    msg.file_size = file_size;
    msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);

    // 5. Ricevi risposta
    struct msg_buffer resp;
    msgrcv(msg_id, &resp, sizeof(resp) - sizeof(long), MSG_RESPONSE, 0);
    if (resp.client_pid == getpid()) {
        printf("SHA-256: %s\n", resp.hash);
    }

    free(buffer);
    return 0;
}