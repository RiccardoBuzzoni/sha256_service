#include "common.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    char* filename = argv[1];
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size > MAX_FILE_SIZE) {
        fprintf(stderr, "File troppo grande\n");
        fclose(fp);
        exit(1);
    }

    unsigned char* buffer = malloc(file_size);
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    // Ottieni IPC
    int msgid = msgget(KEY_MSG_QUEUE, 0666);
    int shmid = shmget(KEY_SHM, MAX_FILE_SIZE, 0666);
    void* shm_ptr = shmat(shmid, NULL, 0);

    // Copia file in memoria condivisa
    memcpy(shm_ptr, buffer, file_size);
    shmdt(shm_ptr);
    free(buffer);

    // Invia richiesta
    message_t msg;
    msg.mtype = MSG_REQUEST;
    msg.pid = getpid();
    msg.file_size = file_size;
    strcpy(msg.filename, filename);

    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("[CLIENT] Richiesta inviata per %s (%d bytes)\n", filename, file_size);

    // Ricevi risposta
    message_t response;
    if (msgrcv(msgid, &response, sizeof(response) - sizeof(long), MSG_RESPONSE, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }

    printf("[CLIENT] Hash SHA-256: %s\n", response.filename);
    return 0;
}