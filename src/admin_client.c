#include "sha256_ipc.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <set_limit N> | get_status\n", argv[0]);
        exit(1);
    }

    int msg_id = msgget(MSG_KEY, 0666);
    if (msg_id == -1) {
        perror("msgget");
        exit(1);
    }

    struct msg_buffer msg = {0};
    msg.msg_type = MSG_ADMIN;
    msg.client_pid = getpid();

    if (strcmp(argv[1], "set_limit") == 0 && argc == 3) {
        msg.cmd = CMD_SET_LIMIT;
        msg.data = atoi(argv[2]);
        msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);

        struct msg_buffer resp;
        msgrcv(msg_id, &resp, sizeof(resp) - sizeof(long), MSG_RESPONSE, 0);
        if (resp.client_pid == getpid()) {
            printf("Limite aggiornato a: %d worker\n", resp.data);
        }
    }
    else if (strcmp(argv[1], "get_status") == 0) {
        msg.cmd = CMD_GET_STATUS;
        msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);

        struct msg_buffer resp;
        msgrcv(msg_id, &resp, sizeof(resp) - sizeof(long), MSG_RESPONSE, 0);
        if (resp.client_pid == getpid() && resp.cmd == CMD_GET_STATUS) {
            printf("Stato del server:\n");
            printf("  Richieste in coda: %zu\n", resp.file_size);
            printf("  Limite massimo worker: %d\n", resp.data);
        }
    }
    else {
        fprintf(stderr, "Comando non valido.\n");
        exit(1);
    }

    return 0;
}