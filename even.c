#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void handle_sighup(int signum) {
    printf("Ouch!\n");
}

void handle_sigint(int signum) {
    printf("Yeah!\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_even_numbers>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "The number of even numbers should be a positive integer.\n");
        return 1;
    }
    
    signal(SIGHUP, handle_sighup);
    signal(SIGINT, handle_sigint);

    for (int i = 0; i < n; ++i) {
        printf("%d\n", i * 2);
        sleep(5);
    }

    return 0;
}
