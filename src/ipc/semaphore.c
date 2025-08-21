#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/game.h"
#include "../include/game_semaphore.h"

// Abrir semáforos nombrados creados por master
semaphore_struct* open_semaphore_memory() {
    semaphore_struct *sem_state = calloc(1, sizeof(semaphore_struct));
    if (!sem_state) return NULL;

    sem_state->A = sem_open(SEM_NAME_A, 0);
    sem_state->B = sem_open(SEM_NAME_B, 0);
    sem_state->C = sem_open(SEM_NAME_C, 0);
    sem_state->D = sem_open(SEM_NAME_D, 0);
    sem_state->E = sem_open(SEM_NAME_E, 0);
    if (sem_state->A == SEM_FAILED || sem_state->B == SEM_FAILED || sem_state->C == SEM_FAILED || sem_state->D == SEM_FAILED || sem_state->E == SEM_FAILED) {
        perror("open_semaphore_memory: sem_open core");
        return NULL;
    }
    char name[32];
    for (int i = 0; i < 9; i++) {
        snprintf(name, sizeof(name), SEM_NAME_G_PREFIX "%d", i);
        sem_state->G[i] = sem_open(name, 0);
        if (sem_state->G[i] == SEM_FAILED) { perror("open_semaphore_memory: sem_open G"); return NULL; }
    }
    return sem_state;
}

void close_semaphore_memory(semaphore_struct *sem_state) {
    if (!sem_state) return;
    sem_close(sem_state->A);
    sem_close(sem_state->B);
    sem_close(sem_state->C);
    sem_close(sem_state->D);
    sem_close(sem_state->E);
    for (int i = 0; i < 9; i++) if (sem_state->G[i]) sem_close(sem_state->G[i]);
    free(sem_state);
}

int wait_for_turn(semaphore_struct *sem_state, int player_id) {
    if (!sem_state || player_id < 0 || player_id >= 9) return -1;
    if (sem_wait(sem_state->G[player_id]) == -1) {
        perror("wait_for_turn: sem_wait");
        return -1;
    }
    return 0;
}

int acquire_read_access(semaphore_struct *sem_state) {
    if (!sem_state) return -1;
    if (sem_wait(sem_state->D) == -1) return -1; // Simplificar: serializar lectores en macOS
    return 0;
}

int release_read_access(semaphore_struct *sem_state) {
    if (!sem_state) return -1;
    if (sem_post(sem_state->D) == -1) return -1;
    return 0;
}
