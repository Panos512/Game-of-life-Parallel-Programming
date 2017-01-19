#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int equal(int *u, int *v, int w, int h){
    int (*array1)[w] = u;
    int (*array2)[w] = v;
    for(int y = 0; y < h; y++)
        for(int x = 0; x < w; x++)
            if(array1[y][x] != array2[y][x]){
                return 0;
            }
    return 1;
}
void show(void *u, int w, int h) {
    int (*univ)[w] = u;
    printf("------------------------------------------------------------\n");
    for(int y = 0; y < h; y++){
        for(int x = 0; x < w; x++)
            printf(univ[y][x] ? "\033[07m  \033[m" : "  ");
        printf("\n");
    }
    printf("------------------------------------------------------------\n");

    fflush(stdout);
}

void evolve(void *u, int w, int h) {
    int (*univ)[w] = u;
    int new[h][w];

    //for each cell inside the grid
    for(int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int n = 0;
            for (int y1 = y - 1; y1 <= y + 1; y1++)
                for (int x1 = x - 1; x1 <= x + 1; x1++) {
                    if (univ[(y1 + h) % h][(x1 + w) % w]) //if neighbor is alive
                        n++;
                }
            if (univ[y][x])
                n--;
            //if the cell has 2 or 3 neighbors and is alive, stays alive
            //if the cell has exactly 3 neighbors, becomes alive
            //if none of the above, cell dies or does not become alive
            //printf("%d,%d is %d and has %d\n", y, x, univ[y][x], n);
            new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
        }
    }
    for(int y = 0; y < h; y++)
        for(int x = 0; x < w; x++)
            univ[y][x] = new[y][x];
}

void game(int w, int h, char* filename) {
    int univ[h][w];
    if(filename != NULL){
        FILE *file;

        file = fopen(filename, "r");
        if (file) {
            for(int x = 0; x < h; ++x ) {
                for(int y = 0; y < w; ++y ) {
                    fscanf( file, "%d", &univ[x][y] );
                }
            }
            fclose(file);
        }

    }else {
        for(int x = 0;  x < w; x++)
            for(int y = 0; y < h; y++)
                univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;

    }
    int previous[h][w];
    for(int y = 0; y < h; y++)
        for(int x = 0; x < w; x++)
            previous[y][x] = univ[y][x];
    show(univ, w, h);
    evolve(univ, w, h);
    while (!equal(previous, univ, w, h)) {
        for(int y = 0; y < h; y++)
            for(int x = 0; x < w; x++)
                previous[y][x] = univ[y][x];
        printf("\n");
        evolve(univ, w, h);
        show(univ, w, h);
        usleep(400000);

    }
}

int main(int argc, char **argv) {
    int w = 0, h = 0;
    if (argc > 1) w = atoi(argv[1]);
    if (argc > 2) h = atoi(argv[2]);
    if (w <= 0) w = 10;
    if (h <= 0) h = 10;
    if(argc < 4)
        game(w, h, NULL);
    else
        game(w, h, argv[3]);
}