// Minimal IPC for semaphores over shared memory (unnamed semaphores)
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include "../include/game.h"
#include "../include/game_semaphore.h"

// Map the shared semaphore segment created by master
semaphore_struct* open_semaphore_memory() {
    int fd = shm_open(SHM_SEM, O_RDWR, 0666);
    if (fd == -1) {
        perror("open_semaphore_memory: shm_open(SHM_SEM)");
        return NULL;
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("open_semaphore_memory: fstat");
        close(fd);
        return NULL;
    }

    if ((size_t)sb.st_size < sizeof(semaphore_struct)) {
        fprintf(stderr, "open_semaphore_memory: shm size too small\n");
        close(fd);
        return NULL;
    }

    semaphore_struct *sem_state = mmap(NULL, sizeof(semaphore_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sem_state == MAP_FAILED) {
        perror("open_semaphore_memory: mmap");
        close(fd);
        return NULL;
    }
    close(fd);
    return sem_state;
}

void close_semaphore_memory(semaphore_struct *sem_state) {
    if (!sem_state) return;
    if (munmap(sem_state, sizeof(semaphore_struct)) == -1) {
        perror("close_semaphore_memory: munmap");
    }
}

int wait_for_turn(semaphore_struct *sem_state, int player_id) {
    if (!sem_state || player_id < 0 || player_id >= 9) return -1;
    if (sem_wait(&sem_state->player_turn_sem[player_id]) == -1) {
        perror("wait_for_turn: sem_wait");
        return -1;
    }
    return 0;
}

int acquire_read_access(semaphore_struct *sem_state) {
    if (!sem_state) return -1;
    if (sem_wait(&sem_state->reader_count_mutex) == -1) return -1;
    sem_state->reader_count++;
    if (sem_state->reader_count == 1) {
        if (sem_wait(&sem_state->game_state_mutex) == -1) {
            // rollback F and E on failure
            sem_state->reader_count--;
            sem_post(&sem_state->reader_count_mutex);
            return -1;
        }
    }
    if (sem_post(&sem_state->reader_count_mutex) == -1) return -1;
    return 0;
}

int release_read_access(semaphore_struct *sem_state) {
    if (!sem_state) return -1;
    if (sem_wait(&sem_state->reader_count_mutex) == -1) return -1;
    if (sem_state->reader_count > 0) sem_state->reader_count--;
    if (sem_state->reader_count == 0) {
        if (sem_post(&sem_state->game_state_mutex) == -1) {
            sem_post(&sem_state->reader_count_mutex);
            return -1;
        }
    }
    if (sem_post(&sem_state->reader_count_mutex) == -1) return -1;
    return 0;
}
