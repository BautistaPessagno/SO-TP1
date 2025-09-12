// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    int direction; // 0-7 representing 8 possible directions
    int player_id;
} move_message;

int send_move(int pipe_fd, int direction, int player_id) {
    move_message msg;
    msg.direction = direction;
    msg.player_id = player_id;
    
    ssize_t bytes_written = write(pipe_fd, &msg, sizeof(move_message));
    if (bytes_written != sizeof(move_message)) {
        perror("write to pipe");
        return -1;
    }
    return 0;
}

int receive_move(int pipe_fd, int *direction, int *player_id) {
    move_message msg;
    
    ssize_t bytes_read = read(pipe_fd, &msg, sizeof(move_message));
    if (bytes_read != sizeof(move_message)) {
        if (bytes_read == 0) {
            return 0; // EOF
        }
        perror("read from pipe");
        return -1;
    }
    
    *direction = msg.direction;
    *player_id = msg.player_id;
    return 1;
}
