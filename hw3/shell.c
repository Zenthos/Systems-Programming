#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

/* STRUC TYPE DEFINITIONS */

typedef struct job_struc {
	pid_t pid;
	pid_t pgid;
	int hidden;
	int job_id;
	int status;
	int argc;
	char **argv;
	char *command;
} Job;

/* GLOBAL VARIABLES */

int job_count = 0;
pid_t foreground = -1;
pid_t sigchld_pid = -2;
Job *jobs;

char *built_in_commands[] = {
	"bg",
	"fg",
	"cd",
	"jobs",
	"kill",
	"exit",
	NULL
};

/* FUNCTION DEFINITIONS */

char* read_input() {
	char *input;
	size_t bufsize = 32;

	input = malloc(bufsize * sizeof(char));

	if (input == NULL) {
		perror("Unable to allocate buffer");
		exit(1);
	}

	printf("> ");
	getline(&input, &bufsize, stdin);

	input = strtok(input, "\n");
	return input;
}

int command_type(char *command) {
	int i = 0;
	while(built_in_commands[i] != NULL) {
		if (strcmp(command, built_in_commands[i]) == 0) {
			return i;
		}

		i++;
	}

	struct dirent *directory;
   	DIR *usrDir = opendir("/usr/bin");
    	if (!usrDir) return -1;

    	while ((directory = readdir(usrDir)) != NULL) {
		char *dir_name = directory->d_name;

		if (strcmp(dir_name, command) == 0) {
			closedir(usrDir);
			return 6;
		}
    	}

	closedir(usrDir);
    	DIR *binDir = opendir("/bin");
    	if (!binDir) return -1;

    	while ((directory = readdir(binDir)) != NULL) {
		char *dir_name = directory->d_name;

		if (strcmp(dir_name, command) == 0) {
			closedir(binDir);
			return 7;
		}
    	}
	
	closedir(binDir);
	return -1;
}

Job *getJobById(int num) {
	for(int i = 0; i < job_count; i++) {
		if (jobs[i].job_id == num) return &jobs[i];
	}

	return NULL;
}

Job *getJobByPid(pid_t pid) {
	for(int i = 0; i < job_count; i++) {
		if(jobs[i].pid == pid) return &jobs[i];
	}

	return NULL;
}

void addJob(Job job) {
	int max_id = 0;
	for(int i = 0; i < job_count; i++) {
		if (jobs[i].job_id > max_id) {
			max_id = jobs[i].job_id;
		}
	}

	jobs = realloc(jobs, (job_count + 1) * sizeof(Job));
	jobs[job_count] = job;
	job_count++;
}

void deleteJob(pid_t pid) {
	int jobsrunning = 0;
	
	for (int i = 0; i < job_count; i++) {
		if (jobs[i].pid == pid) {
			jobs[i].hidden = 1;
		}

		if (jobs[i].status != 2) {
			jobsrunning++;
		}
	}


	if (jobsrunning == 0) {
		job_count = 0;
	}	
}

void sigterm_handler(int sig) {
	exit(15);
}

void sigint_handler(int sig) {
	if (foreground != -1) {
		sigset_t mask, prev;
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);

		sigprocmask(SIG_BLOCK, &mask, &prev);
		printf("Killing process [%d]\n", foreground);
		kill(foreground, SIGKILL);
		deleteJob(foreground);
		
		sigprocmask(SIG_SETMASK, &prev, NULL);
	}
}

void sigtstp_handler(int sig) {
	if (foreground != -1) {
		sigset_t mask, prev;
		sigemptyset(&mask);
		sigaddset(&mask, SIGCHLD);
		
		sigprocmask(SIG_BLOCK, &mask, &prev);
		kill(foreground, SIGSTOP);
		
		Job *j = getJobByPid(foreground);
		if (j != NULL) j->status = 1;
		sigprocmask(SIG_SETMASK, &prev, NULL);
		sigchld_pid = j->pid;
	}
}

void sigchld_handler(int sig) {
	pid_t pid;

	/* Reap Child */
	while ((pid = waitpid(-1, 0, WNOHANG)) > 0) { 
		Job *j = getJobByPid(pid);
		j->status = 2;
		sigchld_pid = j->pid;
	}
}

int exec_fg(char **args) {
	if (strstr(args[1], "%")) {
		sigset_t prev;
		Job *j = getJobById(args[1][1] - '0');
		kill(j->pid, SIGCONT);
		j->status = 0;
		foreground = j->pid;

		// Keep suspending if signal received does not match foreground process
		do {
			sigsuspend(&prev);
		} while(sigchld_pid != foreground);

		foreground = -1;
		sigchld_pid = -2;
	}
	
	return 0;
}

int exec_bg(char **args) {
	if (strstr(args[1], "%")) {
		Job *j = getJobById(args[1][1] - '0');
		kill(j->pid, SIGCONT);
		j->status = 0;
	}

	return 0;
}

// Change Directory
int exec_cd(char **args, int argc) {
	char cwd[PATH_MAX];

	if (argc == 1) {
		chdir(getenv("HOME"));	
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			setenv("PWD", cwd, 1);
		}
	} else {
		chdir(args[1]);
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			setenv("PWD", cwd, 1);
		}
	}

	return 0;
}

// Prints the jobs
int print_jobs() {
	for (int i = 0; i < job_count; i++) {
		char *status;
		switch(jobs[i].status) {
			case 0:
				status = "Running";
				break;
			case 1:
				status = "Stopped";
				break;
			case 2:
				status = "Finished";
				break;
			default:
				status = "Terminated";
		}

		if (jobs[i].hidden == 0) {
			printf("[%d] %d %s %s\n", jobs[i].job_id, jobs[i].pid, status, jobs[i].command);
		}
	}

	for (int j = 0; j < job_count; j++) {
		if (jobs[j].status == 2 && jobs[j].hidden == 0) {
			deleteJob(jobs[j].pid);
		}
	}

	return 0;
}

// Kills a terminal instance
int exec_kill(char **args, Job job) {
	if (strstr(args[1], "%")) {
		Job *j = getJobById(args[1][1] - '0');
		if (j) {
			kill(j->pid, SIGTERM);
			j->status = 3;
			printf("[%d] %d Terminated on signal 15\n", j->job_id, j->pid);
			deleteJob(j->pid);
		} else {
			printf("%s: no such job\n", job.command);
		}
	}

	return 0;
}

// Send signals to non terminated processes on exit 
int exec_exit(char **args) {
	for (int i = 0; i < job_count - 1; i++) {
		kill(jobs[i].pid, SIGHUP);
	}

	for (int i = 0; i < job_count - 1; i++) {
		if (jobs[i].status == 1) {
			kill(jobs[i].pid, SIGCONT);	
		}
	}

	return 1;
}

void free_jobs() {
	for (int j = 0; j < job_count; j++) {
		free(jobs[j].command);
	}

	free(jobs);
}

int exec_command(Job *job) {
	pid_t pid;
	int fg_flag = 0;

	signal(SIGCHLD, sigchld_handler);
	sigset_t prev;

	// Remove the & if its the last argument
	if (job->argc > 1 && strcmp(job->argv[job->argc - 1], "&") == 0) {
		fg_flag = 1;
		job->argv[job->argc - 1] = NULL;
		free(job->argv[job->argc]);
		free(job->argv[job->argc - 1]);
	}

	pid = fork();
	// Child process
	if (pid == 0)
       	{
		// Add SIGTERM handler to child process
		signal(SIGTERM, sigterm_handler);
		setpgid(0, 0);
		execv(job->argv[0], job->argv);	
		return 1;
	} 
	// Parent process
	else if (pid > 0) 
	{	
		// Add process to job array
		job->pid = pid;
		addJob(*job);

		// Wait for child to finish if in the foreground
		if (fg_flag == 0) {
			foreground = pid;
			
			// Keep suspending if signal received does not match foreground process
			do {
				sigsuspend(&prev);
			} while(sigchld_pid != foreground);

			foreground = -1;
			sigchld_pid = -2;
		}

		return 0;
	}
	// On Fork Failure
       	else
       	{
		printf("Fork failed\n");
		return 1;
	}
}

int main() {
	int sentinel = 0;
	
	job_count = 0;
	jobs = malloc(1 * sizeof(Job));
	signal(SIGINT, sigint_handler);
	signal(SIGTSTP, sigtstp_handler);
	
	while(sentinel == 0) {
		char **args = NULL;
		int arg_c = 0;
		int i = 0;
		
		Job job;
		char *str = read_input();
		char *split;
		
		job.command = malloc((strlen(str) + 1) * sizeof(char));
		strcpy(job.command, str);
		split = strtok(str, " \t");

		while (split != NULL) {
			args = realloc(args, (i + 1) * sizeof(char*));
			args[i] = malloc((strlen(split) + 1) * sizeof(char));
			strcpy(args[i], split);
			i++;

			split = strtok(NULL, " \t");
		}
		
		// Add NULL to end of the array, for execv
		args = realloc(args, (i + 1) * sizeof(char*));
		args[i] = NULL;
		arg_c = i;
		i++;

		job.argv = args;
		job.argc = arg_c;
		job.pgid = getpid();
		job.job_id = job_count;
		job.status = 0;
		job.hidden = 0;

		char firstChar = args[0][0];

		// Arg attempting to execute file
		// Else attempting to execute command
		if (firstChar == '.' || firstChar == '/') {
			if(access(args[0], F_OK) == 0) {
				sentinel = exec_command(&job);
			} else {
				free(job.command);
				printf("%s: No such file or directory\n", args[0]);
			}
		} else {
			int command_index = command_type(args[0]);	
			char usrPath[] = "/usr/bin/";
			char binPath[] = "/bin/";
			char path[256];

			switch(command_index) {
				case 0:
					sentinel = exec_bg(args);
					free(job.command);
					break;
				case 1:
					sentinel = exec_fg(args);
					free(job.command);
					break;
				case 2:
					sentinel = exec_cd(args, arg_c);
					free(job.command);
					break;
				case 3:
					sentinel = print_jobs();
					free(job.command);
					break;
				case 4:
					sentinel = exec_kill(args, job);
					free(job.command);
					break;
				case 5:
					sentinel = exec_exit(args);
					free(job.command);
					break;
				case 6:
					strcat(path, usrPath);
					strcat(path, args[0]);
					args[0] = realloc(args[0], (strlen(path) + 1) * sizeof(char));
					strcpy(args[0], path);
					sentinel = exec_command(&job);
					break;
				case 7:
					strcat(path, binPath);
					strcat(path, args[0]);		
					args[0] = realloc(args[0], (strlen(path) + 1) * sizeof(char));
					strcpy(args[0], path);
					sentinel = exec_command(&job);
					break;
				default:
					free(job.command);
					printf("%s: command not found\n", args[0]);
			}

			path[0] = '\0';
		}
		
		for (i = 0; i < arg_c; i++)
			free(args[i]);

		free(args);
		free(split);
		free(str);
	}
		
	free_jobs();
	return 0;
}
