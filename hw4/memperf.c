#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "mymalloc.h"

void random_action(char **pointers, int *pointers_length, int action) {
	switch(action) {
		// Allocate memory, and save pointer in array
		case 0:
			pointers[*pointers_length] = mymalloc(rand() % 257);
			(*pointers_length)++;
		// Free a random pointer from array
		case 1:
			if (*pointers_length > 0) {
				myfree(pointers[rand() % *pointers_length]);
			}
		// Reallocate a random pointer in array
		default:
			if (*pointers_length > 0) {
				myrealloc(pointers[rand() % *pointers_length], rand() % 257);
			}
	}
}

int main() {
	int pointers_length = 0, operation_count = 10000000;
	struct timeval start, end;
	double time_taken = 0;
	time_t seed;
	srand((unsigned) time(&seed));

	char *pointers[1000000];

	// First Fit 
	myinit(0);
	gettimeofday(&start, NULL);
	for (int i = 0; i < operation_count; i++) {
		random_action(pointers, &pointers_length, rand() % 3);
	}
	gettimeofday(&end, NULL);
	time_taken = end.tv_sec - start.tv_sec;

	printf("First fit throughput: %.2f ops/sec\n", operation_count/time_taken);
	printf("First fit utilization: %.2f\n", utilization());

	mycleanup();
	// Next Fit
	myinit(1);
	gettimeofday(&start, NULL);
	for (int i = 0; i < operation_count; i++) {
		random_action(pointers, &pointers_length, rand() % 3);
	}
	gettimeofday(&end, NULL);
	time_taken = end.tv_sec - start.tv_sec;
	
	printf("Next fit throughput: %.2f ops/sec\n", operation_count/time_taken);
	printf("Next fit utilization: %.2f\n", utilization());

	mycleanup();

	// Best Fit
	myinit(2);
	gettimeofday(&start, NULL);
	for (int i = 0; i < operation_count; i++) {
		random_action(pointers, &pointers_length, rand() % 3);
	}
	gettimeofday(&end, NULL);
	time_taken = end.tv_sec - start.tv_sec;
	
	printf("Best fit throughput: %.2f ops/sec\n", operation_count/time_taken);
	printf("Best fit utilization: %.2f\n", utilization());

	mycleanup();
	return 0;
}
