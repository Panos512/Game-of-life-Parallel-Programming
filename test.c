#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
#define for_x for (int x = 0; x < w; x++)
#define for_y for (int y = 0; y < h; y++)
#define for_xy for_x for_y
void show(void *u, int w, int h)
{
	int (*univ)[w] = u;
//	printf("\033[H");
	for(int y = 0; y < 30; y++){
		for(int x = 0; x < 30; x++)
			 printf(univ[y][x] ? "1 " : "0 ");
		printf("\n");
	}
printf("\n");
//	fflush(stdout);
}
 
void evolve(void *u, int w, int h)
{
	unsigned (*univ)[w] = u;
	unsigned new[h][w];
 
	for_y for_x {
		int n = 0;
		for (int y1 = y - 1; y1 <= y + 1; y1++)
			for (int x1 = x - 1; x1 <= x + 1; x1++)
				if (univ[(y1 + h) % h][(x1 + w) % w])
					n++;
 
		if (univ[y][x]) n--;
		new[y][x] = (n == 3 || (n == 2 && univ[y][x]));
	}
	for_y for_x univ[y][x] = new[y][x];
}
 
void game(int w, int h)
{
	unsigned univ[h][w];
	unsigned univ2[h][w];
	int count = 0;
	for_xy univ[y][x] = rand() < RAND_MAX / 10 ? 1 : 0;
	for_xy univ2[x][y] = univ[x][y];
	//show(univ, w,h);
	while (1) {
		count++;
		printf("%d\n", count);
		show(univ, w, h);
		evolve(univ, w, h);
		int br = 0;
		for_xy if(univ2[x][y] != univ[x][y]){
				br = 1;
				break;
		}

		if(br == 0)
			break;
		for_xy univ2[x][y] = univ[x][y];
	//	usleep(200000);
	}
//	printf("iterations %d\n", count);
	show(univ, w, h);
}
 
int main(int c, char **v)
{
	int w = 0, h = 0;
	if (c > 1) w = atoi(v[1]);
	if (c > 2) h = atoi(v[2]);
	if (w <= 0) w = 10;
	if (h <= 0) h = 10;
	game(w, h);
}
