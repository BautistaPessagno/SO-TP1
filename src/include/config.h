#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Parámetros de configuración globales
extern int delay;
extern int timeout;
extern int seed;
extern char *view_path;

// Funciones de configuración
void print_usage(const char *program_name);
int parse_arguments(int argc, char *argv[], int *width, int *height, int *num_players, char *player_executables[]);
void validate_parameters(int width, int height, int num_players);

#endif // CONFIG_H
