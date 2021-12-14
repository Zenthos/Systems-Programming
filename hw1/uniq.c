#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char **inputs = NULL;
    int maxStringSize = 256;

    /* Dynamically allocated and set array of user input strings */
    int arrayLength = 0;
    int i = 0;
    while (1) {
        char typedInput[maxStringSize];
        char *line = fgets(typedInput, maxStringSize, stdin);
        if (line == NULL) {
            printf("^D\n");
	    break;
        }

        inputs = realloc(inputs, (i + 1) * sizeof(char*));
        inputs[i] = malloc((strlen(typedInput) + 1) * sizeof(char));
        strcpy(inputs[i], typedInput);
        i++;
    }
    arrayLength = i;

    /* 1 Array for Unique Words, 1 Array for Unique Word Count */
    char **unique = NULL;
    int *count = NULL;
    int uniqueCount = 0, j = 0, exists = 0;

    for (i = 0; i < arrayLength; i++) {
	exists = 0;
	/* Determine if word already exists in unique array */
	if (i > 0 && strcmp(unique[uniqueCount - 1], inputs[i]) == 0) {
	    count[uniqueCount - 1] = count[uniqueCount - 1] + 1;
	    exists = 1;
	}

	/* If word does not exist in unique array, reallocate memory and add it */
	if (exists == 0) {
	    unique = realloc(unique, (uniqueCount + 1) * sizeof(char*));
	    count = realloc(count, (uniqueCount + 1) * sizeof(int));

	    unique[uniqueCount] = malloc((strlen(inputs[i]) + 1) * sizeof(char));
	    strcpy(unique[uniqueCount], inputs[i]);

	    count[uniqueCount] = 1;
	    uniqueCount++;
	}
    }

    for (i = 0; i < arrayLength; i++) {
	free(inputs[i]);
    }
    free(inputs);

    for (i = 0; i < uniqueCount; i++) {
	printf("%d %s", count[i], unique[i]);
	free(unique[i]);
    }

    free(count);
    free(unique);

    return 0;
}
