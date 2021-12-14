#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cstring_cmp(const void *a, const void *b) {
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp(*ia, *ib);
}

int main(int argc, char *argv[]) {
    char **words = NULL;
    int maxStringSize = 256;
    int arrayLength = 0;

    /* Dynamically allocated and set array of user input strings */
    int i = 0;
    while (1) {
	char typedInput[maxStringSize];
	char *line = fgets(typedInput, maxStringSize, stdin);

	if (line == NULL) {
	    printf("^D\n");
	    break;
	}

	words = realloc(words, (i + 1) * sizeof(char*));
	words[i] = malloc((strlen(typedInput) + 1) * sizeof(char));
	strcpy(words[i], typedInput);
	++i;
    }
    arrayLength = i;

    /* Sort Array Here */
    qsort(words, arrayLength, sizeof(char *), cstring_cmp);

    /* Print Array Here */
    for (i = 0; i < arrayLength; i++) {
	printf("%s", words[i]);
	free(words[i]);
    }
    free(words);

    return 0;
}
