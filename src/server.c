#include "common.h"
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_PROCESSES 10

int msgid, shmid, semid;
int max_processes = 3; // Limite dinamico
int current_processes = 0;
int scheduling_policy = SCHED_SIZE; // Default: per dimensione

// Struttura per richieste in coda
typedef struct {
    message_t msg;
    time_t timestamp;
} request_queue_t;

request_queue_t queue[100];
int queue_count = 0;

// Funzione per comparare richieste per dimensione (usata in qsort)
int compare_by_size(const void *a, const void *b) {
    return ((request_queue_t*)a)->msg.file_size - ((request_queue_t*)b)->msg.file_size;
}

// Funzione per comparare richieste FCFS
int compare_by_time(const void *a, const void *b) {
    return ((request_queue_t*)a)->timestamp - ((request_queue_t*)b)->timestamp;
}

// Funzione per ordinare la coda
void sort_queue() {
    if (scheduling_policy == SCHED_SIZE)
        qsort(queue, queue_count, sizeof(request_queue_t), compare_by_size);
    else
        qsort(queue, queue_count, sizeof(request_queue_t), compare_by_time);
}

// Funzione per elaborare un file
void process_request(message_t *msg) {
    // Attacca memoria condivisa
    void* shm_ptr = shmat(shmid, NULL, 0);
    if (shm_ptr == (void*)-1) {
        perror("shmat");
        return;
    }

    // Leggi file
    char* file_data = (char*)shm_ptr;
    char hash[65];
    sha256_hash((unsigned char*)file_data, msg->file_size, hash);

    // Stacca memoria
    shmat(shmid, NULL, 0);

    // Invia risposta
    message_t response;
    response.mtype = MSG_RESPONSE;
    response.pid = msg->pid;
    strcpy(response.filename, hash);

    if (msgsnd(msgid, &response, sizeof(response) - sizeof(long), 0) == -1)
        perror("msgsnd response");
}

// Handler per terminare i figli
void sigchld_handler(int sig) {
    (void)sig;  // Indica che il parametro è volontariamente non usato
    while (waitpid(-1, NULL, WNOHANG) > 0)
        current_processes--;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <scheduling_policy> [max_processes]\n", argv[0]);
        exit(1);
    }

    scheduling_policy = !strcmp(argv[1], "fcfs") ? SCHED_FCFS : SCHED_SIZE;
    if (argc > 2) max_processes = atoi(argv[2]);
    if (max_processes <= 0 || max_processes > MAX_PROCESSES) max_processes = 3;

    signal(SIGCHLD, sigchld_handler);

    // Crea IPC
    msgid = msgget(KEY_MSG_QUEUE, 0666 | IPC_CREAT);
    shmid = shmget(KEY_SHM, MAX_FILE_SIZE, 0666 | IPC_CREAT);
    semid = semget(KEY_SEM, 1, 0666 | IPC_CREAT);
    semctl(semid, 0, SETVAL, 1); // Inizializza semaforo a 1

    printf("[SERVER] Avviato con policy: %s, max processes: %d\n",
           scheduling_policy == SCHED_FCFS ? "FCFS" : "SIZE", max_processes);

    struct sembuf sem_op;

    while (1) {
        message_t msg;
        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            if (errno == EINTR) continue;
            perror("msgrcv");
            break;
        }

        if (msg.mtype == MSG_CONFIG) {
            // Cambia limite processi
            max_processes = msg.file_size;
            printf("[SERVER] Limite processi aggiornato a %d\n", max_processes);
            continue;
        }

        if (msg.mtype == MSG_STATUS) {
            // Invia stato
            server_status_t status = { current_processes, queue_count, max_processes };
            message_t resp;
            resp.mtype = MSG_RESPONSE;
            resp.pid = msg.pid;
            memcpy(resp.filename, &status, sizeof(server_status_t));
            msgsnd(msgid, &resp, sizeof(resp) - sizeof(long), 0);
            continue;
        }

        // Richiesta normale
        queue[queue_count].msg = msg;
        queue[queue_count].timestamp = time(NULL);
        queue_count++;
        sort_queue();

        // Elabora se possibile
        while (queue_count > 0 && current_processes < max_processes) {
            message_t next = queue[0].msg;  // Estrai il messaggio dalla coda
            memmove(&queue[0], &queue[1], (queue_count - 1) * sizeof(request_queue_t));
            queue_count--;

            sem_op.sem_op = -1; semop(semid, &sem_op, 1); // P()

            pid_t pid = fork();
            if (pid == 0) {
                // Figlio: elabora
                process_request(&next);
                exit(0);
            } else if (pid > 0) {
                current_processes++;
            } else {
                perror("fork");
            }

            sem_op.sem_op = 1; semop(semid, &sem_op, 1); // V()
        }
    }

    // Cleanup
    msgctl(msgid, IPC_RMID, NULL);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    return 0;
}