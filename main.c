#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>

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
    MPI_Scatterv(buffer, counts, disps, blocktype, local_buffer, blockheight*blockwidth, MPI_CHAR, 0, MPI_COMM_WORLD);


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