#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

char *find_executable_path(const char *command, const char *path_env) {
    
    char *path_copy = strdup(path_env);

    if (path_copy == NULL) {
        return NULL;
    }

    char *directory = strtok(path_copy, ":");

    while (directory != NULL) {

        int length = strlen(directory) + strlen(command) + 2;

        char *candidate = malloc(length);

        if (candidate == NULL) {
            free(path_copy);
            return NULL;
        }

        snprintf(candidate, length, "%s/%s", directory, command);

        if (access(candidate, X_OK) == 0) {
            free(path_copy);
            return candidate;
        }

        free(candidate);

        directory = strtok(NULL, ":");
    }
 
    free(path_copy);
    return NULL;
}

int main (void) {

    
    char line[1024];

    while (1) {

        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {

        }

        printf("tush> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        CommandLine *cl = parse_command_line(line);

        if (cl == NULL) {
            continue;
        }

        if (strcmp(cl->left.argv[0], "exit") == 0) {
            free_command_line(cl);
            break;
        }

        if (strcmp(cl->left.argv[0], "cd") == 0) {

            if (cl->left.argc < 2) {
                fprintf(stderr, "cd: missing directory\n");
            }

            else if (chdir(cl->left.argv[1]) != 0) {
                perror("cd");
            }

            free_command_line(cl);
            continue;
        }

        char *path_env = getenv("PATH");

        int pid = fork(); 

        if (pid < 0) {
            perror("fork");
            free_command_line(cl);
            continue;
        }

        else if (pid == 0) {

            if (cl->left.input_file != NULL) {

                int fd = open(cl->left.input_file, O_RDONLY);

                if (fd < 0) {
                    perror("open");
                    exit(1);
                }

                if (dup2(fd,0) < 0) {
                    perror("dup2");
                    close(fd);
                    exit(1);
                }


                
            }

            if (cl->left.output_file != NULL) {

                int fd = open(cl->left.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);

                if (fd < 0) {
                    perror("open");
                    exit(1);
                }

                if (dup2(fd,1) < 0) {
                    perror("dup2");
                    close(fd);
                    exit(1);
                }

                close(fd);

            }

            char *executable_path;

            if (cl->left.argv[0][0] == '/') {
                executable_path = cl->left.argv[0];
            }

            else {
                executable_path = find_executable_path(cl->left.argv[0],path_env);
            }

            if (executable_path == NULL) {
                fprintf(stderr, "command not found\n");
                exit(1);
            }

            execv(executable_path, cl->left.argv);

            perror("execv");
            exit(1);
        }

         else {

            if (cl->background) {
                printf("[background pid %d]\n", pid);
            }

            else {

            int wstatus;

            waitpid(pid, &wstatus, 0);

            if (WIFEXITED(status)) {
                int code = WEXITSTATUS(wstatus);

                if (code != 0) {
                    fprintf(stderr, "tush:exited with status %d\n", code);
                }
            }

        }
        }

        free_command_line(cl);
    }

    return 0;

}