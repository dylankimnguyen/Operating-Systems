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
char line[NL];         /* command input buffer */

void prompt(void) {
    // fprintf(stdout, "\nmsh> ");
    fflush(stdout);
}

int main(int argk, char *argv[], char *envp[]) {
    int frkRtnVal;     /* value returned by fork sys call */
    int wpid;          /* value returned by wait */
    char *v[NV];       /* array of pointers to command line tokens */
    char *sep = " \t\n";/* command line token separators */
    int i;             /* parse index */
    int background;    /* flag for background processes */
    
    /* signal handler to reap zombie processes */
    signal(SIGCHLD, SIG_IGN);

    /* prompt for and process one command line at a time */
    while (1) {        /* do Forever */
        prompt();
        fgets(line, NL, stdin);
        fflush(stdin);

        if (feof(stdin)) {      /* non-zero on EOF */
            // fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(), feof(stdin), ferror(stdin));
            exit(0);
        }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
            continue;           /* to prompt */

        v[0] = strtok(line, sep);
        for (i = 1; i < NV; i++) {
            v[i] = strtok(NULL, sep);
            if (v[i] == NULL)
                break;
        }
        /* assert i is number of tokens + 1 */
        
        /* Check for background process */
        if (i > 1 && strcmp(v[i-1], "&") == 0) {
            background = 1;
            v[i-1] = NULL; /* Remove '&' from command arguments */
        } else {
            background = 0;
        }

        /* handle 'cd' command */
        if (strcmp(v[0], "cd") == 0) {
            if (v[1] == NULL) {
                fprintf(stderr, "msh: expected argument to \"cd\"\n");
            } else {
                if (chdir(v[1]) != 0) {
                    perror("msh: chdir failed");
                }
            }
            continue; /* Return to prompt */
        }

        /* fork a child process to exec the command in v[0] */
        switch (frkRtnVal = fork()) {
        case -1:           /* fork returns error to parent process */
            perror("msh: fork failed");
            break;
        case 0:            /* code executed only by child process */
            if (execvp(v[0], v) == -1) {
                perror("msh: execvp failed");
                exit(EXIT_FAILURE); /* Ensure child process exits on failure */
            }
            break;
        default:           /* code executed only by parent process */
            if (!background) {
                wpid = waitpid(frkRtnVal, NULL, 0); /* Wait for child if not background */
                if (wpid == -1) {
                    perror("msh: waitpid failed");
                } else {
                    // printf("%s done\n", v[0]);
                }
            } else {
                printf("Process %d running in background\n", frkRtnVal);
            }
            break;
        } /* switch */
    } /* while */
    return 0;
} /* main */
