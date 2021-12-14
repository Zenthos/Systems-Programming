#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int cstring_cmp(const void *a, const void *b) {
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcasecmp(*ia, *ib);
}

void listFiles(char *rootPath, int depth) {
    char path[1000];
    char **files = NULL;

    struct dirent *dp;
    DIR *dir = opendir(rootPath);

    if (!dir) return;

    int i = 0;
    while ((dp = readdir(dir)) != NULL) {
        char *dir_name = dp->d_name;

        if (strcmp(dir_name, ".") != 0 && strcmp(dir_name, "..") != 0) {
	    files = realloc(files, (i + 1) * sizeof(char*));
	    files[i] = malloc((strlen(dir_name) + 1) * sizeof(char));
	    strcpy(files[i], dir_name);
	    i++;
	}
    }

    qsort(files, i, sizeof(char*), cstring_cmp);

    for (int k = 0; k < i; k++) {
	for (int m = 0; m < depth; m++)
	    printf("  ");

	char *dir_name = files[k];

        strcpy(path, rootPath);
        strcat(path, "/");
        strcat(path, dir_name);

        struct stat buf;
	stat(path, &buf);

	printf(" - %s\n", dir_name);

	if (S_ISDIR(buf.st_mode))
	    listFiles(path, depth + 1);

        free(files[k]);
    }

    free(files);
    closedir(dir);
}

int main(int argc, char *argv[]) {
    printf(".\n");
    listFiles(".", 0);

    return 0;
}
