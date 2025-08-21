#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../include/game.h"

game* open_shared_memory() {
    int fd = shm_open(SHM_STATE, O_RDONLY, 0666);
    if (fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // Get the size of the shared memory
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return NULL;
    }

    game *game_state = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (game_state == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    close(fd);
    return game_state;
}

void close_shared_memory(game *game_state, size_t size) {
    if (munmap(game_state, size) == -1) {
        perror("munmap");
    }
}
