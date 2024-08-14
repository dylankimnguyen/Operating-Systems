#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define NV 20          /* max number of command tokens */
#define NL 100         /* input buffer size */
#define MAX_BG_JOBS 100  /* maximum number of background jobs */
char line[NL];         /* command input buffer */

typedef struct { //struct 
    pid_t pid;
    int job_number;
    char command[NL];
} BackgroundJob;

BackgroundJob background_jobs[MAX_BG_JOBS];
int job_count = 0; // track background jobs

/* Function to print shell prompt */
void prompt(void) {
    fflush(stdout);
}

// Function to handle completed background processes
void check_background_processes() {
    int status;
    pid_t pid;

    for (int i = 0; i < job_count; ) {
        pid = waitpid(background_jobs[i].pid, &status, WNOHANG);
        if (pid == -1) {
            perror("waitpid failed");
            break;
        }
        if (pid > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                printf("[%d]+ Done %s\n", background_jobs[i].job_number, background_jobs[i].command);

                for (int j = i; j < job_count - 1; j++) {
                    background_jobs[j] = background_jobs[j + 1];
                }
                job_count--; //remove background job
                // Do not increment i, as we are shifting jobs down and need to check the new job at position i
            } else {
                i++; // can move to next job if current hasn't exited
            }
        } else {
            // if wpid = 0 job is still running
            i++;
        }
    }
}

void sigchld_handler(int signum) {
    (void)signum; // Avoid unused parameter warning
    check_background_processes();
}

int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal;     /* value returned by fork sys call */
    char *v[NV];       /* array of pointers to command line tokens */
    char *sep = " \t\n";/* command line token separators */
    int i;             /* parse index */
    int background;    /* flag for background processes */

    // signal handle for sigchld
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    /* Prompt for and process one command line at a time */
    while (1) {
        // Check for background process completion before prompting for new input 
        check_background_processes();
        
        prompt();
        if (fgets(line, NL, stdin) == NULL) {
            if (feof(stdin)) { /* Exit on EOF */
                //fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
                exit(0);
            }
            perror("msh: fgets failed");
            continue;
        }

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue; /* Ignore comments and empty lines */

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }

        /* Check for background process */
        if (i > 1 && strcmp(v[i-1], "&") == 0) {
            background = 1;
            v[i-1] = NULL; /* Remove '&' from command arguments */
        } else {
            background = 0;
        }

        // cd
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "msh: expected argument to \"cd\"\n");
            } else {
                if (chdir(v[1]) != 0) {
                    perror("msh: chdir failed");
                }
            }
            continue; /* Return to prompt after handling cd */
        }

        /* Fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
        case -1: /* Fork returns error to parent process */
            perror("msh: fork failed");
            break;
        case 0: /* Code executed only by child process */
            if (execvp(v[0], v) == -1) {
                perror("msh: execvp failed");
                exit(EXIT_FAILURE); /* Ensure child process exits on failure */
            }
            break;
        default: /* Code executed only by parent process */
            if (!background) {
                waitpid(frkRtnVal, NULL, 0); /* Wait for child if not background */
            } else {
                /* Store background job details */
                background_jobs[job_count].pid = frkRtnVal;
                background_jobs[job_count].job_number = job_count + 1;

                // Store the entire command with arguments
                snprintf(background_jobs[job_count].command, NL, "%s", v[0]);
                for (int j = 1; v[j] != NULL; j++) {
                    strncat(background_jobs[job_count].command, " ", NL - strlen(background_jobs[job_count].command) - 1);
                    strncat(background_jobs[job_count].command, v[j], NL - strlen(background_jobs[job_count].command) - 1);
                }

                background_jobs[job_count].command[NL - 1] = '\0'; // Ensure null termination
                job_count++;
                printf("[%d] %d\n", background_jobs[job_count - 1].job_number, frkRtnVal);
            }
            break;
        } /* switch */
    } /* while */
    return 0;
} /* main */
