#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "bitmap.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>


#define ERROR_MESSAGE "Warning: one or more filter had an error, so the output image may not be correct.\n"
#define SUCCESS_MESSAGE "Image transformed successfully!\n"


/*
 * Check whether the given command is a valid image filter, and if so,
 * run the process.
 *
 * We've given you this function to illustrate the expected command-line
 * arguments for image_filter. No further error-checking is required for
 * the child processes.
 */
void run_command(const char *cmd) {
    if (strcmp(cmd, "copy") == 0 || strcmp(cmd, "./copy") == 0 ||
        strcmp(cmd, "greyscale") == 0 || strcmp(cmd, "./greyscale") == 0 ||
        strcmp(cmd, "gaussian_blur") == 0 || strcmp(cmd, "./gaussian_blur") == 0 ||
        strcmp(cmd, "edge_detection") == 0 || strcmp(cmd, "./edge_detection") == 0) {
        execl(cmd, cmd, NULL);
    } else if (strncmp(cmd, "scale", 5) == 0) {
        // Note: the numeric argument starts at cmd[6]
        execl("scale", "scale", cmd + 6, NULL);
    } else if (strncmp(cmd, "./scale", 7) == 0) {
        // Note: the numeric argument starts at cmd[8]
        execl("./scale", "./scale", cmd + 8, NULL);
    } else {
        fprintf(stderr, "Invalid command '%s'\n", cmd);
        exit(1);
    }
}


// TODO: Complete this function.
int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: image_filter input output [filter ...]\n");
        exit(1);
    }

    // special case handled separately
    else if (argc == 3) {
        int in_des = open(argv[1], O_RDONLY);
        if (dup2(in_des, fileno(stdin)) == -1) {
            perror("dup2");
            exit(1);
        }
        if (close(in_des) == -1) {
            perror("closed");
            exit(1);
        }

        int out_des = open(argv[2], O_WRONLY);
        if ((dup2(out_des, fileno(stdout))) == -1) {
            perror("dup2");
            exit(1);
        }
        if (close(out_des) == -1) {
            perror("close");
            exit(1);
        }
        
        execl("copy", "copy", NULL);
    }

    // general case
    else {
        int pipefds[argc - 3][2], r;
        for (int i = 0; i < argc - 3; i++) {

            // create pipe for the i-th child process
            if (pipe(pipefds[i]) == -1) {
                perror("pipe");
                exit(1);
            }
            
            // create the i-th child process
            r = fork();

            if (r == -1) {
                perror("fork");
                exit(1);
            }

            // child process
            else if (r == 0) {
                
                // close both the write and read ends of pipes that are two indices less than i
                for (int j = i - 2; j >= 0; j--) {
                    if (close(pipefds[j][0]) == -1) {
                        perror("close");
                        exit(1);
                    }
                    if (close(pipefds[j][1]) == -1) {
                        perror("close");
                        exit(1);
                    }
                }
                
                // resetting stdin as the read end of pipe in previous process
                if (i > 0) {
                    if (dup2(pipefds[i-1][0], fileno(stdin)) == -1) {
                        perror("dup2");
                        exit(1);
                    }
                    if (close(pipefds[i-1][0]) == -1) {
                        perror("close");
                        exit(1);
                    }
                    if (close(pipefds[i-1][1]) == -1) {
                        perror("close");
                        exit(1);
                    }
                // or the file specified as argv[1]
                } else {
                    int in_des = open(argv[1], O_RDONLY);
                    if (in_des == -1) {
                        perror("open");
                        exit(1);
                    }
                    if (dup2(in_des, fileno(stdin)) == -1) {
                        perror("dup2");
                        exit(1);
                    }
                    if (close(in_des) == -1) {
                        perror("close");
                        exit(1);
                    }
                }
                
                // resetting stdout to as the write end of current pipe
                if (i < argc - 4) {
                    if (dup2(pipefds[i][1], fileno(stdout)) == -1) {
                        perror("dup2");
                        exit(1);
                    }
                // or the file specified by argv[3]
                } else {
                    int out_des = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (out_des == -1) {
                        perror("open");
                        exit(1);
                    }
                    if (dup2(out_des, fileno(stdout)) == -1) {
                        perror("dup2");
                        exit(1);
                    }
                    if (close(out_des) == -1) {
                        perror("close");
                        exit(1);
                    }
                }
                
                // close both write and read ends of current pipe
                if (close(pipefds[i][0]) == -1) {
                    perror("close");
                    exit(1);
                }
                if (close(pipefds[i][1]) == -1) {
                    perror("close");
                    exit(1);
                }

                // run filter
                run_command(argv[i+3]);

                // exit(0);
            } 

            else if (r > 0) {
            }
        }

        // close all parents pipes now
        for (int j = argc - 4; j >= 0; j--) {
            if (close(pipefds[j][0]) == -1) {
                perror("close");
                exit(1);
            }
            if (close(pipefds[j][1]) == -1) {
                perror("close");
                exit(1);
            }
        }

        // wait for processes to finish and print the error if exit status is 1
        int stat;
        for (int i = 0; i < argc - 3; i++) {
            if (wait(&stat) == -1) {
                perror("wait");
                exit(1);
            }
            if (WIFEXITED(stat)) {
                if (WEXITSTATUS(stat) == 1) {
                    printf(ERROR_MESSAGE);
                    exit(1);
                }
            }
        }
    }

    // runs only if all processes exits with 0
    printf(SUCCESS_MESSAGE);
    return 0;
}
