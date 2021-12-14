#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>


int cstring_cmp(const void *a, const void *b) {
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcasecmp(*ia, *ib);
}


int main(int argc, char *argv[]) {
    char **files = NULL;
    int showMeta = 0;

    if (argc > 1 && strcmp(argv[1], "-l") == 0)
	showMeta = 1;

    struct dirent *directory;
    DIR *dir = opendir(".");

    if (!dir) return 1;

    int i = 0;
    while ((directory = readdir(dir)) != NULL) {
	char *dir_name = directory->d_name;

	if (strcmp(dir_name, ".") != 0 && strcmp(dir_name, "..") != 0) {
            files = realloc(files, (i + 1) * sizeof(char*));
	    files[i] = malloc((strlen(dir_name) + 1) * sizeof(char));
	    strcpy(files[i], dir_name);
	    i++;
	}
    }

    qsort(files, i, sizeof(char *), cstring_cmp);

    for (int k = 0; k < i; k++) {

	if (showMeta == 1) {
	    struct stat buf;
	    stat(files[k], &buf);

	    printf( (S_ISDIR(buf.st_mode)) ? "d" : "-");
   	    printf( (buf.st_mode & S_IRUSR) ? "r" : "-");
  	    printf( (buf.st_mode & S_IWUSR) ? "w" : "-");
   	    printf( (buf.st_mode & S_IXUSR) ? "x" : "-");
   	    printf( (buf.st_mode & S_IRGRP) ? "r" : "-");
    	    printf( (buf.st_mode & S_IWGRP) ? "w" : "-");
   	    printf( (buf.st_mode & S_IXGRP) ? "x" : "-");
    	    printf( (buf.st_mode & S_IROTH) ? "r" : "-");
    	    printf( (buf.st_mode & S_IWOTH) ? "w" : "-");
    	    printf( (buf.st_mode & S_IXOTH) ? "x" : "-");

	    struct passwd *pw = getpwuid(buf.st_uid);
	    struct group  *gr = getgrgid(buf.st_gid);

	    if (pw != NULL)
	         printf(" %s", pw->pw_name);
	    else
	         printf(" %d", buf.st_uid);

	    if (gr != NULL)
	         printf(" %s", gr->gr_name);
	    else
	         printf(" %d", buf.st_gid);

	    printf(" %ld", buf.st_size);

	    char time[80];
	    strftime(time, 80, "%b %d %H:%M", localtime(&buf.st_mtime));

	    printf(" %s", time);
	    printf(" %s\n", files[k]);
	} else {
      	    printf("%s\n", files[k]);
	}

	free(files[k]);
    }

    free(files);
    closedir(dir);
    return 0;
}

