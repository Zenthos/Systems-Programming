#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
	printf("You need to input an argument\n");
        return 1;
    }

    if (argc > 2) {
	printf("You have too many arguments\n");
	return 1;
    }

    char *arg = argv[1];
    int num = atoi(arg);

    if (num == 0 || num < 0) {
	printf("Please enter a valid positive integer\n");
        return 1;
    }

    int count;
    for(count = 2; num > 1; count++) {
        while(num % count == 0) {
            printf("%d ", count);
            num = num / count;
        }
    }

    printf("\n");
    return 0;
}
