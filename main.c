#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
typedef enum {DIR_ULEFT = 0, DIR_UP, DIR_URIGHT, DIR_LEFT, DIR_CENTER,
    DIR_RIGHT, DIR_DLEFT, DIR_DOWN, DIR_DRIGHT} dir;
// store the ranks of neighbouring processes in ranks[]
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


char* create_buffer(char* buffer, int width, int height, char** argv, int argc){
    if(argc == 2){
        char* filename = argv[1];
        FILE *file;

        file = fopen(filename, "r");
        if (file) {
            fscanf(file, "%*d");
            fscanf(file, "%*d");
            int arraySize = width * height;

            buffer =(char*) malloc(sizeof (char) * arraySize);
            for(int i = 0; i < arraySize; i++){
                int num;
                fscanf( file, "%d", &num);
                buffer[i]= num;
            }
            fclose(file);
        }
    }else if(argc == 3){

        int arraySize = width * height;

        buffer = (char*) malloc(sizeof(char) * arraySize);
        for(int i = 0; i < arraySize; i++)
            buffer[i] = rand() < RAND_MAX / 10 ? 1 : 0;
    }

    return buffer;

}
int main(int argc, char **argv) {
    MPI_Comm cartesian;

    int size, rank;
    int width, height;

    if(argc == 2){
        char* filename = argv[1];
        FILE *file;

        file = fopen(filename, "r");
        if (file) {
            fscanf(file, "%d", &width);
            fscanf(file, "%d", &height);
        }
        fclose(file);
    }else if(argc == 3){
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }

    //MPI Initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    //Matrix Creation

    char* buffer;

    if(rank == 0){
        buffer = create_buffer(buffer, width, height, argv, argc);
    }

    //Calculating width & height of each block

    int blockside = (int)sqrt(size);
    int blockwidth = width / blockside;
    int blockheight = height / blockside;

    int sides[2] = {blockside,blockside};
    int periodic[2] = {1, 1};

    MPI_Cart_create(MPI_COMM_WORLD, 2, sides, periodic, 1, &cartesian);
    if (cartesian == MPI_COMM_NULL) {
        printf("Could not create cartesian communicator\n");
        MPI_Finalize();
        return 1;
    }


    //Allocate local buffer of each process
    char local_buffer[blockheight*blockwidth];
    for (int i = 0; i <  blockheight*blockwidth; i ++)
        local_buffer[i] = 0;

    //Allocate local return buffer of each process
    char local_bufferR[blockheight*blockwidth];
    for (int i = 0; i <  blockheight*blockwidth; i ++)
        local_bufferR[i] = 0;

    //Create datatype for sending blocks
    MPI_Datatype blocktype;
    MPI_Datatype blocktype2;

    MPI_Type_vector(blockheight, blockwidth, width, MPI_CHAR, &blocktype2);
    MPI_Type_create_resized( blocktype2, 0, sizeof(char), &blocktype);
    MPI_Type_commit(&blocktype);



    //Calculate displacements and counts for MPI_Scatterv
    int disps[size];
    int counts[size];
    for (int i = 0; i < blockside; i++) {
        for (int j = 0; j < blockside; j++) {
            disps[i * blockside + j] = i * width * blockheight + j * blockwidth;
            counts [i * blockside + j] = 1;
        }
    }

    //Scatter blocks to processes
    MPI_Scatterv(buffer, counts, disps, blocktype, local_buffer, blockheight*blockwidth, MPI_CHAR, 0, cartesian);
    int neighbour_ranks[9];
    get_ranks(rank,  cartesian, neighbour_ranks);

    for (int proc=0; proc<size; proc++) {
        if (proc == rank) {
            printf("Rank = %d\n", rank);
            if (rank == 0) {
                printf("Global matrix: \n");
                for (int ii=0; ii<height; ii++) {
                    for (int jj=0; jj<width; jj++) {
                        printf("%d ",(int)buffer[ii*width+jj]);
                    }
                    printf("\n");
                }
            }
            printf("My Neighbors: \n");
            for(int i = 0; i < 9; i++)
                printf("%d ", neighbour_ranks[i]);
            printf("\n");
            printf("Local Matrix: \n");
            for (int ii=0; ii<blockheight; ii++) {
                for (int jj=0; jj<blockwidth; jj++) {
                    printf("%d ",(int)local_buffer[ii*blockwidth+jj]);
                }
                printf("\n");
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }


    MPI_Finalize();
    return 0;
}