#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <cuda.h>
#include <climits>

void printBoard(unsigned char* buffer, int width, int height)
{
	printf("----------------\n");
	for (int i = 0; i < height; i++){
		for (int j = 0; j < width; j++){
			printf("%c ", buffer[i * height + j]? 'o' : ' ');
		}
		printf("\n");
	}
	printf("----------------\n");


}

bool equal(unsigned char *array1, unsigned char *array2, int width, int height){
		for (int i = 0; i < height; i++){
			for (int j = 0; j < width; j++){
				if(array1[i * height + j] != array2[i * height + j]){
					return false;
				}
			}
		}
		printf("Evolution stoped!\n");
		return true;
}

bool empty(unsigned char *array, int width, int height){
	for (int i = 0; i < height; i++){
		for (int j = 0; j < width; j++){
			if(array[i * height + j] == 0x1){
				return false;
			}
		}
	}
	printf("Everybody died!\n");
	return true;
}

// bool equal(unsigned char *array1, unsigned char *array2, int w, int h){
// 	printf("Equal started!\n");
//     for(int i = 0; i < w * h; i++)
//         if(array1[i] != array2[i])
//             return false;
//
// 		printf("Evolution stoped!\n");
//     return true;
// }
//
// bool empty(unsigned char *array, int w, int h){
// 	printf("empty started!\n");
//     for(int i = 0 ; i < w * h; i++)
//         if(array[i] == 0x1)
//             return false;
// 		printf("Everybody died!\n");
//     return true;
// }



__global__ void golGpu(int height, int width, unsigned char* pBuffer1, unsigned char* pBuffer2){
		int x = blockIdx.x * 2 + threadIdx.x;
		int y = blockIdx.y * 2 + threadIdx.y;

		int indx = x * height + y;

		pBuffer2[indx] = pBuffer1[indx];

		int num = 0;

		if (x-1 >= 0 && x-1 < height && y >= 0 && y < width)
			num += pBuffer1[(x-1) * height + y];

		if (x+1 >= 0 && x+1 < height && y >= 0 && y < width)
			num += pBuffer1[(x+1) * height + y];

		if (x >= 0 && x < height && y-1 >= 0 && y-1 < width)
			num += pBuffer1[x * height + (y-1)];

		if (x >= 0 && x < height && y+1 >= 0 && y+1 < width)
			num += pBuffer1[x * height + (y+1)];

		if (x-1 >= 0 && x-1 < height && y-1 >= 0 && y-1 < width)
			num += pBuffer1[(x-1) * height + (y-1)];

		if (x-1 >= 0 && x-1 < height && y+1 >= 0 && y+1 < width)
			num += pBuffer1[(x-1) * height + (y+1)];

		if (x+1 >= 0 && x+1 < height && y-1 >= 0 && y-1 < width)
			num += pBuffer1[(x+1) * height + (y-1)];

		if (x+1 >= 0 && x+1 < height && y+1 >= 0 && y+1 < width)
			num += pBuffer1[(x+1) * height + (y+1)];

		if(num < 2)
			pBuffer2[indx] = 0x0;

		if(num > 3)
			pBuffer2[indx] = 0x0;

		if(num == 3 && !pBuffer1[indx])
			pBuffer2[indx] = 0x1;
		//return num;

}

void create_buffer(unsigned char* buffer, int width, int height, char** argv, int argc) {
    if (argc == 2) {
        char *filename = argv[1];
        FILE *file;

        file = fopen(filename, "r");
        if (file) {
            fscanf(file, "%*d");
            fscanf(file, "%*d");
            int arraySize = width * height;

            for (int i = 0; i < arraySize; i++) {
                int num;
                fscanf(file, "%d", &num);
                buffer[i] = num;
            }
            fclose(file);

            return;
        }

        return;


    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            float rnd = rand() / (float) RAND_MAX;
            buffer[i * height + j] = (rnd >= 0.7f) ? 0x1 : 0x0;
        }
    }
    return;

}


int main(int argc, char **argv){

	int width, height;
	int iterations = INT_MAX;
	// Random seed
	time_t t;
	srand((unsigned) time(&t));

	// Read file of dimensions from user (if none 12x12 is the default)
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
	}else if(argc == 4){
			width = atoi(argv[1]);
			height = atoi(argv[2]);
			iterations = atoi(argv[3]);
	}
	else{
		width = 12;
		height = 12;
	}

	//Initialise Buffer
	unsigned char* buffer;
	buffer = (unsigned char *) malloc(sizeof(unsigned char) * width * height);
  create_buffer(buffer, width, height, argv, argc);


	// printf("Starting board!\n");
	// printBoard(buffer,width,height);

	// Allocate GPU boards
	unsigned char* pBuffer1;
	cudaMalloc((void **)&pBuffer1, width * height * sizeof(unsigned char));
	cudaMemcpy(pBuffer1, buffer, width * height * sizeof(unsigned char), cudaMemcpyHostToDevice);

	unsigned char* pBuffer2;
	cudaMalloc((void **)&pBuffer2, width * height * sizeof(unsigned char));
	cudaMemcpy(pBuffer2, 0x0, width * height * sizeof(unsigned char), cudaMemcpyHostToDevice);

	dim3 blocksize(2, 2);
	dim3 gridsize((width + blocksize.x - 1)/blocksize.x, (height + blocksize.y - 1)/blocksize.y , 1);

	unsigned char* current;
	unsigned char* next;

	unsigned char* previeousResult;
	previeousResult = (unsigned char *)malloc(width * height * sizeof(unsigned char*));

	int gen = 0;

  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  cudaEventRecord(start);

	do{
		if(gen == iterations) break;
		//printf("Gen: %d\n\n\n", gen);
		memcpy(previeousResult, buffer,width * height * sizeof(unsigned char*));

		// Switching buffers to save previeous state.
		if ((gen % 2) == 0)
		{
			current = pBuffer1;
			next = pBuffer2;
		}
		else
		{
			current = pBuffer2;
			next = pBuffer1;
		}
		golGpu<<<gridsize, blocksize>>>(height, width, current, next);
		gen++;

		cudaMemcpy(buffer, next, width * height * sizeof(unsigned char), cudaMemcpyDeviceToHost);
		cudaMemcpy(previeousResult, current, width * height * sizeof(unsigned char), cudaMemcpyDeviceToHost);

		//printf("Evolved\n");
		//printBoard(buffer, width, height);

		// printf("\n\nPrevieous\n\n\n");
		// printBoard(previeousResult, width, height);

	}while(!empty(buffer,width,height) && !equal(buffer, previeousResult, width, height));

	printf("Generations: %d\n", gen);

	cudaEventRecord(stop);
	cudaEventSynchronize(stop);
	float milliseconds = 0;
	cudaEventElapsedTime(&milliseconds, start, stop);

	printf("Time elapsed: %f ms\n",milliseconds);

	cudaFree(pBuffer1);
	cudaFree(pBuffer2);
	free(buffer);
	free(previeousResult);
	return 0;
}
