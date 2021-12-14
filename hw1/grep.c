#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char *toLower(char *str)
{
    char *str_l = calloc(strlen(str)+1, sizeof(char));

    for (int i = 0; i < strlen(str); ++i) {
        str_l[i] = tolower((unsigned char)str[i]);
    }
    return str_l;
}

int main(int argc, char *argv[]) {
    int insensitive = 0;

    if (argc < 2) {
	printf("An argument needs to be given\n");
	return 1;
    }

    char *firstArg = argv[1];
    char insensitiveArg[] = "-i";

    if (strcmp(firstArg, insensitiveArg) == 0) {
	insensitive = 1;
	firstArg = argv[2];
    }

    int maxStringLen = 256;
    char str1[maxStringLen];

    while (1) {
	char *line = fgets(str1, maxStringLen, stdin);

	if (line == NULL) {
	    printf("^D\n");
	    break;
	}

	if (insensitive == 1 && strstr(toLower(str1), toLower(firstArg))) {
	    printf("%s", str1);
	}

	if (insensitive == 0 && strstr(str1, firstArg)) {
	    printf("%s", str1);
	}
    }
}
