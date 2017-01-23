#include "game_of_life.h"

MPI_Datatype type_column;
MPI_Datatype type_column_rgb;

void prepare_coltype(int blockwidth, int blockheight) {
    MPI_Type_vector(blockheight, 1, blockwidth, MPI_CHAR,
                    &type_column);
    MPI_Type_commit(&type_column);


}

void get_ranks(int cur_rank, MPI_Comm cartesian, int ranks[]) {
    int coords[2];

    //Vres tis suntetagmenes mou
    MPI_Cart_coords(cartesian, cur_rank, 2, coords);
    ranks[DIR_CENTER] = cur_rank;

    //Aptis suntetagmenes mou upologise tis suntetagmenes tou kathe geitona
    //kai vres to rank tou
    //O pinakas rank perilamvanei:
    //rank[DIR_ULEFT, DIR_UP, DIR_URIGHT, DIR_LEFT, DIR_CENTER,
    //DIR_RIGHT, DIR_DLEFT, DIR_DOWN, DIR_DRIGHT]
    coords[1]--;
    coords[0]--;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_ULEFT]);
    coords[1]++;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_UP]);
    coords[1]++;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_URIGHT]);
    coords[0]++;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_RIGHT]);
    coords[1] -= 2;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_LEFT]);
    coords[0]++;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_DLEFT]);
    coords[1]++;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_DOWN]);
    coords[1]++;
    MPI_Cart_rank(cartesian, coords, &ranks[DIR_DRIGHT]);
}

//int equal(char *u, char *v, int w, int h){
//    char (*array1)[w] = u;
//    char (*array2)[w] = v;
//    for(int y = 0; y < h; y++)
//        for(int x = 0; x < w; x++)
//            if(array1[y][x] != array2[y][x]){
//                return 0;
//            }
//    return 1;
//}
//
//void show(void *u, int w, int h) {
//    char (*univ)[w] = u;
//    printf("------------------------------------------------------------\n");
//    for(int y = 0; y < h; y++){
//        for(int x = 0; x < w; x++)
//            printf(univ[y][x] ? "\033[07m  \033[m" : "  ");
//        printf("\n");
//    }
//    printf("------------------------------------------------------------\n");
//
//    fflush(stdout);
//}

char* game_of_life(int blockwidth, int blockheight, int ranks[], MPI_Comm cartesian, char local_buffer[], char local_bufferR[]){
    char new[blockheight*blockwidth];

    MPI_Request sendRequests[9], recvRequests[9];
    unsigned char ulCorner, urCorner, dlCorner, drCorner;
    // allocate buffers for receiving the columns of pixels at the left and right and the rows at the top and bottom
    unsigned char *upRow = (char*)malloc(blockwidth), *downRow = (char*)malloc(blockwidth),
            *leftCol = (char*)malloc(blockheight), *rightCol = (char*)malloc(blockheight);

    // get ranks of neighbouring processes
    int rUleft = ranks[DIR_ULEFT], rUp = ranks[DIR_UP], rUright = ranks[DIR_URIGHT], rRight = ranks[DIR_RIGHT],
            rLeft = ranks[DIR_LEFT], rDleft = ranks[DIR_DLEFT], rDown = ranks[DIR_DOWN], rDright = ranks[DIR_DRIGHT];

    // send needed pixels to other processes (top and bottom row, left and right column and the 4 corner pixels)
    MPI_Isend(&local_buffer[0], 1, MPI_CHAR, rUleft, DIR_ULEFT, cartesian, &sendRequests[DIR_ULEFT]);
    MPI_Isend(local_buffer, blockwidth, MPI_CHAR, rUp, DIR_UP, cartesian, &sendRequests[DIR_UP]);
    MPI_Isend(&local_buffer[blockwidth-1], 1, MPI_CHAR, rUright, DIR_URIGHT, cartesian, &sendRequests[DIR_URIGHT]);
    MPI_Isend(local_buffer, 1, type_column, rLeft, DIR_LEFT, cartesian, &sendRequests[DIR_LEFT]);
    MPI_Isend(&local_buffer[blockwidth-1], 1, type_column, rRight, DIR_RIGHT, cartesian, &sendRequests[DIR_RIGHT]);
    MPI_Isend(&local_buffer[(blockheight-1)*blockwidth], 1, MPI_CHAR, rDleft, DIR_DLEFT, cartesian, &sendRequests[DIR_DLEFT]);
    MPI_Isend(&local_buffer[(blockheight-1)*blockwidth], blockwidth, MPI_CHAR, rDown, DIR_DOWN, cartesian, &sendRequests[DIR_DOWN]);
    MPI_Isend(&local_buffer[blockheight*blockwidth-1], 1, MPI_CHAR, rDright, DIR_DRIGHT, cartesian, &sendRequests[DIR_DRIGHT]);

    sendRequests[DIR_CENTER] = MPI_REQUEST_NULL;

    // get needed pixels from other processes
    MPI_Irecv(&ulCorner, 1, MPI_CHAR, rUleft, DIR_DRIGHT, cartesian, &recvRequests[DIR_ULEFT]);
    MPI_Irecv(upRow, blockwidth, MPI_CHAR, rUp, DIR_DOWN, cartesian, &recvRequests[DIR_UP]);
    MPI_Irecv(&urCorner, 1, MPI_CHAR, rUright, DIR_DLEFT, cartesian, &recvRequests[DIR_URIGHT]);
    MPI_Irecv(leftCol, blockheight, MPI_CHAR, rLeft, DIR_RIGHT, cartesian, &recvRequests[DIR_LEFT]);
    MPI_Irecv(rightCol, blockheight, MPI_CHAR, rRight, DIR_LEFT, cartesian, &recvRequests[DIR_RIGHT]);
    MPI_Irecv(&dlCorner, 1, MPI_CHAR, rDleft, DIR_URIGHT, cartesian, &recvRequests[DIR_DLEFT]);
    MPI_Irecv(downRow, blockwidth, MPI_CHAR, rDown, DIR_UP, cartesian, &recvRequests[DIR_DOWN]);
    MPI_Irecv(&drCorner, 1, MPI_CHAR, rDright, DIR_ULEFT, cartesian, &recvRequests[DIR_DRIGHT]);

    recvRequests[DIR_CENTER] = MPI_REQUEST_NULL;

    MPI_Waitall(8, recvRequests, MPI_STATUSES_IGNORE);

    //for inside cells
    for(int i = 1; i < blockheight - 1; i++){
        for(int j = 1; j < blockwidth - 1; j++){
            int offset = i * blockwidth + j;
            int n = 0;
            for(int ii = i - 1; ii <= i + 1; ii++){
                for(int jj = j - 1; jj <= j + 1; jj++){
                    int offset2 = ii * blockwidth + jj;
                    if(local_buffer[offset2])
                        n++;
                }
            }
            if(local_buffer[offset])
                n--;
            local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));
        }
    }

    //for top-row cells (no corners)
    for(int i = 1; i < blockwidth - 1; i++){
        int offset = i;
        int n = 0;
        if(local_buffer[offset - 1])
            n++;
        if(local_buffer[offset + 1])
            n++;
        if(local_buffer[offset + blockwidth - 1])
            n++;
        if(local_buffer[offset + blockwidth])
            n++;
        if(local_buffer[offset + blockwidth + 1])
            n++;
        if(upRow[offset - 1])
            n++;
        if(upRow[offset])
            n++;
        if(upRow[offset + 1])
            n++;

        local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));
    }

    //for bottom-row cells (no corners)
    for(int i = blockwidth * (blockheight - 1) + 1; i < blockwidth * blockheight - 1; i++){
        int lastrow = blockwidth * (blockheight - 1);
        int offset = i;
        int n = 0;
        if(local_buffer[offset - 1])
            n++;
        if(local_buffer[offset + 1])
            n++;
        if(local_buffer[offset - blockwidth - 1])
            n++;
        if(local_buffer[offset - blockwidth])
            n++;
        if(local_buffer[offset - blockwidth + 1])
            n++;
        if(downRow[offset - lastrow - 1])
            n++;
        if(downRow[offset - lastrow])
            n++;
        if(downRow[offset - lastrow + 1])
            n++;

        local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));
    }

    //for left-column cells (no corners)
    for(int i = 1; i < blockheight - 1; i++){
        int offset = i * blockwidth;
        int n = 0;
        if(local_buffer[offset - blockwidth])
            n++;
        if(local_buffer[offset - blockwidth + 1])
            n++;
        if(local_buffer[offset + 1])
            n++;
        if(local_buffer[offset + blockwidth])
            n++;
        if(local_buffer[offset + blockwidth + 1])
            n++;
        if(leftCol[i - 1])
            n++;
        if(leftCol[i])
            n++;
        if(leftCol[i + 1])
            n++;

        local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));
    }

    //for right-column cells (no corners)
    for(int i = 1; i < blockheight - 1; i++){
        int offset = (i * blockwidth) + blockwidth - 1;
        int n = 0;
        if(local_buffer[offset - blockwidth - 1])
            n++;
        if(local_buffer[offset - blockwidth])
            n++;
        if(local_buffer[offset - 1])
            n++;
        if(local_buffer[offset + blockwidth - 1])
            n++;
        if(local_buffer[offset + blockwidth])
            n++;
        if(rightCol[i - 1])
            n++;
        if(rightCol[i])
            n++;
        if(rightCol[i] + 1)
            n++;

        local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));

    }

    //for top-left corner
    int n = 0;
    if(upRow[0])
        n++;
    if(leftCol[0])
        n++;
    if(upRow[1])
        n++;
    if(leftCol[1])
        n++;
    if(urCorner)
        n++;
    if(local_buffer[1])
        n++;
    if(local_buffer[blockwidth])
        n++;
    if(local_buffer[blockwidth + 1])
        n++;

    local_bufferR[0] = (n == 3 || (n == 2 && local_buffer[0]));

    //for top-right corner
    n = 0;
    if(upRow[blockwidth - 2])
        n++;
    if(upRow[blockwidth - 1])
        n++;
    if(rightCol[0])
        n++;
    if(rightCol[1])
        n++;
    if(urCorner)
        n++;
    if(local_buffer[blockwidth - 2])
        n++;
    if(local_buffer[(blockwidth * 2) - 2])
        n++;
    if(local_buffer[(blockwidth * 2) - 1])
        n++;

    local_bufferR[blockwidth - 1] = (n == 3 || (n == 2 && local_buffer[blockwidth - 1]));

    //for left-bottom corner
    n = 0;
    int offset = blockwidth * (blockheight - 1);

    if(local_buffer[offset - blockwidth])
        n++;
    if(local_buffer[offset - blockwidth + 1])
        n++;
    if(local_buffer[offset + 1])
        n++;
    if(downRow[0])
        n++;
    if(downRow[1])
        n++;
    if(leftCol[blockwidth - 1])
        n++;
    if(leftCol[blockwidth - 2])
        n++;
    if(dlCorner)
        n++;

    local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));

    //for right-bottom corner
    n = 0;
    offset = (blockwidth * blockheight) - 1;

    if(local_buffer[offset - blockwidth])
        n++;
    if(local_buffer[offset - blockwidth - 1])
        n++;
    if(local_buffer[offset - 1])
        n++;
    if(rightCol[blockwidth - 1])
        n++;
    if(rightCol[blockwidth - 2])
        n++;
    if(drCorner)
        n++;
    local_bufferR[offset] = (n == 3 || (n == 2 && local_buffer[offset]));


    // wait for all sends to process
    MPI_Waitall(8, sendRequests, MPI_STATUSES_IGNORE);

    char *tmp = local_bufferR;
    local_bufferR = local_buffer;
    local_buffer = tmp;

    free(upRow);
    free(leftCol);
    free(rightCol);
    free(downRow);

    return local_buffer;
}

//void game(int w, int h, char* filename) {
//    int univ[h][w];
//    if(filename != NULL){
//        FILE *file;
//
//        file = fopen(filename, "r");
//        if (file) {
//            for(int x = 0; x < h; ++x ) {
//                for(int y = 0; y < w; ++y ) {
//                    fscanf( file, "%d", &univ[x][y] );
//                }
//            }
//            fclose(file);
//        }
//
//    }else {
//        for(int x = 0;  x < w; x++)
//            for(int y = 0; y < h; y++)
//                univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
//
//    }
//    int previous[h][w];
//    for(int y = 0; y < h; y++)
//        for(int x = 0; x < w; x++)
//            previous[y][x] = univ[y][x];
//    show(univ, w, h);
//    evolve(univ, w, h);
//    while (!equal(previous, univ, w, h)) {
//        for(int y = 0; y < h; y++)
//            for(int x = 0; x < w; x++)
//                previous[y][x] = univ[y][x];
//        printf("\n");
//        evolve(univ, w, h);
//        show(univ, w, h);
//        usleep(400000);
//
//    }
//}
