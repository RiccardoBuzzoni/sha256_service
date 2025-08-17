#include "sha256_ipc.h"
#include <sys/wait.h>
#include <signal.h>

#define MAX_QUEUE 100

typedef struct {
    pid_t client_pid;
    char filename[256];
    size_t file_size;
} request_t;

typedef struct {
    request_t req;
    int priority;
} queued_request_t;

queued_request_t request_queue[MAX_QUEUE];
int queue_count = 0;
int max_workers = 1;  // modificabile da admin
int current_workers = 0;

int sem_id, shm_id, msg_id;
char* shm_ptr;
char scheduling_policy[10];  // "fcfs" o "srtf"

void enqueue_request(request_t req) {
    if (queue_count >= MAX_QUEUE) return;

    int pos = queue_count;
    if (strcmp(scheduling_policy, "srtf") == 0) {
        for (int i = 0; i < queue_count; i++) {
            if (req.file_size < request_queue[i].req.file_size) {
                pos = i;
                break;
            }
        }
        for (int i = queue_count; i > pos; i--) {
            request_queue[i] = request_queue[i-1];
        }
    }
    request_queue[pos].req = req;
    request_queue[pos].priority = req.file_size;
    queue_count++;
    printf("[Server]: Richiesta aggiunta - '%s' (%d bytes)\n", req.filename, req.file_size);
}

request_t dequeue_request() {
    if (queue_count == 0) {
        request_t empty = {0};
        return empty;
    }
    request_t req = request_queue[0].req;
    for (int i = 1; i < queue_count; i++) {
        request_queue[i-1] = request_queue[i];
    }
    queue_count--;
    return req;
}

void handle_request(request_t req) {
    printf("[Worker %d] Inizio: '%s' (%d bytes)\n", getpid(), req.filename, req.file_size);

    sem_wait_op(sem_id, SEM_BUFFER);  // aspetta che il buffer sia pieno

    void* data = shmat(shm_id, NULL, 0);
    if (data == (void*)-1) {
        perror("shmat");
        return;
    }
    char hash[65];
    compute_sha256((unsigned char*)data, req.file_size, hash);
    shmdt(data);

    sem_signal_op(sem_id, SEM_BUFFER);  // libera il buffer

    struct msg_buffer msg = {0};
    msg.msg_type = MSG_RESPONSE;
    msg.client_pid = req.client_pid;
    strcpy(msg.hash, hash);
    msgsnd(msg_id, &msg, sizeof(msg) - sizeof(long), 0);

    printf("[Worker %d] Completato: '%s' → %s\n", getpid(), req.filename, hash);
}

void sigchld_handler() {
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        current_workers--;
        printf("[Worker terminato]: PID %d, attivi: %d\n", pid, current_workers);
    }
}

void sigint_handler() {
    if(semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }
    if(msgctl(msg_id, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(EXIT_FAILURE);
    }
    if(shmdt(shm_ptr) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    if(shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {

    // Verifica del passaggio corretto per i parametri
    if (argc != 3 || strcmp(argv[1], "--sched") != 0) {
        fprintf(stderr, "Usage: %s --sched <fcfs|srtf>\n", argv[0]);
        exit(1);
    }
    strcpy(scheduling_policy, argv[2]);

    // Inizializzazione delle primitive System V per la comunicazione client-server
    shm_id = init_shm();
    msg_id = init_msg_queue();
    sem_id = init_semaphores();

    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("shmat server");
        exit(EXIT_FAILURE);
    }

    struct msg_buffer msg;

    // Corpo del server
    printf("[Server] processo server avviato: %d\n", getpid()); fflush(stdout);
    while (1) {

        msgrcv(msg_id, &msg, sizeof(msg) - sizeof(long), 0, 0);

        if (msg.msg_type == MSG_REQUEST) {
            request_t req;

            // Viene creata la struttura dati della richiesta

            req.client_pid = msg.client_pid;
            strcpy(req.filename, msg.filename);
            req.file_size = msg.file_size;

            sem_wait_op(sem_id, SEM_QUEUE);
            enqueue_request(req);
            sem_signal_op(sem_id, SEM_QUEUE);

            if (current_workers < max_workers) {
                pid_t pid = fork();
                if (pid == 0) {
                    sem_wait_op(sem_id, SEM_QUEUE);
                    request_t my_req = dequeue_request();
                    sem_signal_op(sem_id, SEM_QUEUE);
                    if (my_req.file_size > 0) {
                        handle_request(my_req);
                    }
                    exit(0);
                } else if (pid > 0) {
                    current_workers++;
                }
            }
        }
        else if (msg.msg_type == MSG_ADMIN) {
            struct msg_buffer resp = {0};
            resp.msg_type = MSG_RESPONSE;
            resp.client_pid = msg.client_pid;

            if (msg.cmd == CMD_SET_LIMIT) {
                max_workers = msg.data;
                resp.data = max_workers;
                msgsnd(msg_id, &resp, sizeof(resp) - sizeof(long), 0);
            }
            else if (msg.cmd == CMD_GET_STATUS) {
                sem_wait_op(sem_id, SEM_QUEUE);
                resp.cmd = CMD_GET_STATUS;
                resp.data = max_workers;
                resp.file_size = queue_count;
                sem_signal_op(sem_id, SEM_QUEUE);
                msgsnd(msg_id, &resp, sizeof(resp) - sizeof(long), 0);
            }
        }
    }
}