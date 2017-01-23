#ifndef THE_GAME_OF_LIFE_GAME_OF_LIFE_H
#define THE_GAME_OF_LIFE_GAME_OF_LIFE_H

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>


typedef enum {DIR_ULEFT = 0, DIR_UP, DIR_URIGHT, DIR_LEFT, DIR_CENTER,
    DIR_RIGHT, DIR_DLEFT, DIR_DOWN, DIR_DRIGHT} dir;

void prepare_coltype(int blockwidth, int blockheight);

void get_ranks(int cur_rank, MPI_Comm cartesian, int ranks[]);

//int equal(char *u, char *v, int w, int h);
//
//void show(void *u, int w, int h);

char* game_of_life(int blockwidth, int blockheight, int neighbour_ranks[], MPI_Comm cartesian, char local_buffer[], char local_bufferR[]);

#endif //THE_GAME_OF_LIFE_GAME_OF_LIFE_H
