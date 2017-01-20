#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

int* create_buffer(int* buffer, int width, int height, char** argv, int argc){
    if(argc == 2){
        char* filename = argv[1];
        FILE *file;

        file = fopen(filename, "r");
        if (file) {
            fscanf(file, "%*d");
            fscanf(file, "%*d");
            int arraySize = width * height;

            buffer =(int*) malloc(sizeof (int) * arraySize);
            for(int i = 0; i < arraySize; i++)
                fscanf( file, "%d", &buffer[i]);

            fclose(file);
        }
    }else if(argc == 3){

        int arraySize = width * height;

        buffer = (int*) malloc(sizeof(int) * arraySize);
        for(int i = 0; i < arraySize; i++)
            buffer[i] = i;
    }

    return buffer;

}
int main(int argc, char **argv) {
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

    MPI_Comm cartesian;
    //MPI Initialization
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    //Matrix Creation
    int* buffer = NULL;
    if(rank == 0){
        buffer = create_buffer(buffer, width, height, argv, argc);
        printf("Rank: %d\n", rank);
        for(int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++)
                printf("%d ", buffer[i * width + j]);
            printf("\n");
        }
        printf("\n");
    }

    //Calculating width & height of each block
    int blockside = (int)sqrt(size);
    int blockwidth = width / blockside;
    int blockheight = height / blockside;

    int sides[2] = {blockside, blockside};
    int periodic[2] = {1, 1};

    MPI_Cart_create(MPI_COMM_WORLD, 2, sides, periodic, 1, &cartesian);
    if (cartesian == MPI_COMM_NULL) {
        printf("Could not create cartesian communicator\n");
        MPI_Finalize();
        return 1;
    }

    //Calculate displacements and counts for MPI_Scatterv
    int *disps = malloc(size*sizeof(int));
    int *counts = malloc(size*sizeof(int));
    for (int i = 0; i < blockside; i++) {
        for (int j = 0; j < blockside; j++) {
            disps[i * blockside + j] = i * width * blockheight + j * blockwidth;
            counts [i * blockside + j] = 1;
        }
    }

    //Create datatype for sending blocks
    MPI_Datatype mpi_type;
    MPI_Datatype mpi_type2;

    MPI_Type_vector(blockheight, blockwidth, width, MPI_CHAR, &mpi_type2);
    MPI_Type_create_resized(mpi_type2, 0, sizeof(char), &mpi_type);
    MPI_Type_commit(&mpi_type);

    //Allocate local buffer of each process
    int *local_buffer = (int*) malloc(sizeof(int)*width*height/size);
    for (int i = 0; i <  width*height/size; i ++)
        local_buffer[i] = 0;

    //Allocate local return buffer of each process
    int *local_bufferR = (int*) malloc(sizeof(int)*width*height/size);
    for (int i = 0; i <  width*height/size; i ++)
        local_bufferR[i] = 0;
    //Scatter blocks to processes
    MPI_Scatterv(buffer, counts, disps, mpi_type, local_buffer, width*height/size, MPI_CHAR, 0, cartesian);


    for (int proc=0; proc<size; proc++) {
        if (proc == rank) {
            printf("Rank = %d\n", rank);
            if (rank == 0) {
                printf("Global matrix: \n");
                for (int ii=0; ii<height; ii++) {
                    for (int jj=0; jj<width; jj++) {
                        printf("%3d ",(int)buffer[ii*width+jj]);
                    }
                    printf("\n");
                }
            }
            printf("Local Matrix: \n");
            for (int ii=0; ii<blockheight; ii++) {
                for (int jj=0; jj<blockwidth; jj++) {
                    printf("%3d ",(int)local_buffer[ii*blockwidth+jj]);
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