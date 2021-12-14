#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void findFile(char*rootPath, char *substring) {
    char path[50];

    struct dirent *directory;
    DIR *dir = opendir(rootPath);

    if (!dir) return;

    while ((directory = readdir(dir)) != NULL) {
	char *dir_name = directory->d_name;

	if (strcmp(dir_name, ".") != 0 && strcmp(dir_name, "..") != 0) {
	    strcpy(path, rootPath);
	    strcat(path, "/");
	    strcat(path, dir_name);

	    if (strstr(dir_name, substring))
	        printf("%s\n", path);

	    findFile(path, substring);
	}
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
	printf("No arguments were given\n");
	return 1;
    }

    if (argc > 2) {
	printf("Too many arguments given\n");
	return 1;
    }

    findFile(".", argv[1]);
    return 0;
}
