#include "common.h"

int main() {
    int msgid = msgget(KEY_MSG_QUEUE, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    // Richiedi stato
    message_t msg;
    msg.mtype = MSG_STATUS;
    msg.pid = getpid();

    if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd status");
        exit(1);
    }

    message_t response;
    if (msgrcv(msgid, &response, sizeof(response) - sizeof(long), MSG_RESPONSE, 0) == -1) {
        perror("msgrcv status");
        exit(1);
    }

    server_status_t* status = (server_status_t*)response.filename;
    printf("[MONITOR] Stato server:\n");
    printf("  Processi attivi: %d\n", status->active_processes);
    printf("  Richieste in coda: %d\n", status->pending_requests);
    printf("  Limite massimo: %d\n", status->max_processes);
    return 0;
}