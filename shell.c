#include "shell.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>



//Shell Functions
void print_intro() {
    printf("Welcome to Chris_Shell\nType \"help\" for help\n");
}

void print_dir() {
    printf("\033[1m\033[34mChris_Shell\033[0m$ ");
}

char* shell_read_line() {
    uint32_t buffer_size = BUFFER_SIZE, i = 0;
    char* buffer = malloc(sizeof(char) * buffer_size);
    char c; 

    //Make sure the buffer actually has the memory space
    if(!buffer) {
        perror("Chris_Shell line buffer allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        c = getchar();

        //EOF is the user pressing ctrl-d 
        if(c == EOF || c == '\n'){
            buffer[i++] = '\0';
            return buffer;
        } else {
            buffer[i++] = c;
        }

        //This hopefully will not happen, but we can make the buffer bigger if needed
        if(i >= buffer_size) {
            buffer_size *= 2; //Double the buffer size
            buffer = realloc(buffer, buffer_size);
            if(!buffer) {
                perror("Chris_Shell line buffer reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char** shell_line_split(char* line) {
    uint32_t bufsize = TOKENS_COUNT, i = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if(!tokens) {
        perror("Chris_Shell token buffer allocation error\n");
        exit(EXIT_FAILURE);
    }

    //p2 itterates over the chars
    //p1 marks the beginning of a token
    int p2 = 0, p1 = 0;
    char c = line[p2];
    while(c != '\0') {
        switch(c) {
            case ' ':
            case '\t':
            case '\r':
            case '\n':
                //Add the token before the space/tab/return/newline if one exists
                if(p2 > p1) {
                    token = strndup(&line[p1], p2 - p1);
                    tokens[i++] = token;
                }
                p1 = p2 + 1;
                break;
            case '|':
            case '>':
            case '<':
                //Add the token before the pipe if one exists
                if(p2 > p1) {
                    token = strndup(&line[p1], p2 - p1);
                    tokens[i++] = token;
                }
                token = strndup(&line[p2], 1); // Grabs the special char
                tokens[i++] = token;
                p1 = p2 + 1;
                break;
            default:
                break;
        }
        c = line[++p2];

        if(i >= bufsize) {
            bufsize *= 2;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens) {
                perror("Chris_Shell token buffer reallocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
 
    //Add the last token if one exists
    if(p2 > p1) {
        token = strndup(&line[p1], p2 - p1);
        tokens[i++] = token;
    }

    tokens[i] = NULL;
    return tokens;
}

int shell_exec(char** args, char* prev_command, char* line) {

    if (args[0] == NULL) { return 1; } //No commands entered

    //Check for echo
    char* last = last_token(args);
    if(last != NULL && (strcmp(last, "echo") == 0 || (strcmp(last, "ECHO") == 0))) {
        return echo_command(line);
    }

    //Redirection and Piping
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "|") == 0) {
            args[i] = NULL;
            //printf("Launch Pipe \n");
            return shell_launch_pipe(args, &args[i + 1]);
        } else if (strcmp(args[i], ">") == 0) {
            args[i] = NULL;
            //printf("Launch Output Redirection \n");
            return shell_launch_with_output_redirection(args, args[i + 1]);
        } else if (strcmp(args[i], "<") == 0) {
            args[i] = NULL;
            //printf("Launch Input Redirection \n");
            return shell_launch_with_input_redirection(args, args[i + 1]);
        }
        i++;
    }
 
    //Check for built in commands
    for(int i = 0; built_in_commands[i] != NULL; i++) {
        if (strcmp(args[0], built_in_commands[i]) == 0) {
        return (*built_in_functions[i])(args, prev_command, line);
        }
    }

    return shell_launch(args);
}

int shell_launch(char** args) {
    pid_t pid;//, wpid;
    int status;

    pid = fork();
    if(pid == 0) {
        //Child Process
        if(execvp(args[0], args) == -1) {
            perror("Chris_Shell exec error");
            exit(EXIT_FAILURE);
        } 
    } else if(pid < 0) {
        //Fork Error
        perror("Chris_Shell fork error");
    } else {
        //Parent process
        do {
            //wpid = waitpid(pid, &status, WUNTRACED);
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    
    return 1;
}

int shell_launch_with_output_redirection(char** args, char* output_file) {
    pid_t pid;
    int status;
    int fd;

    pid = fork();
    if (pid == 0) {
        // Child process
        fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Chris_Shell open error");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);

        if (execvp(args[0], args) == -1) {
            perror("Chris_Shell exec error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Chris_Shell fork error");
    } else {
        waitpid(pid, &status, 0);
    }

    return 1;
}

int shell_launch_with_input_redirection(char** args, char* input_file) {
    pid_t pid;
    int status;
    int fd;

    pid = fork();
    if (pid == 0) {
        fd = open(input_file, O_RDONLY);
        if (fd < 0) {
            perror("Chris_Shell open error");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);

        if (execvp(args[0], args) == -1) {
            perror("Chris_Shell exec error");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Chris_Shell fork error");
    } else {
        waitpid(pid, &status, 0);
    }

    return 1;
}

int shell_launch_pipe(char** args1, char** args2) {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) == -1) {
        perror("Chris_Shell pipe error");
        return 1;
    }

    p1 = fork();
    if (p1 == 0) {
        // First child process
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        if (execvp(args1[0], args1) == -1) {
            perror("Chris_Shell exec1 error");
            exit(EXIT_FAILURE);
        }
    } else if (p1 < 0) {
        perror("Chris_Shell fork1 error");
        return 1;
    }

    p2 = fork();
    if (p2 == 0) {
        // Second child process
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);

        if (execvp(args2[0], args2) == -1) {
            perror("Chris_Shell exec2 error");
            exit(EXIT_FAILURE);
        }
    } else if (p2 < 0) {
        perror("Chris_Shell fork2 error");
        return 1;
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(p1, NULL, 0);
    waitpid(p2, NULL, 0);

    return 1;
}

//Helper Functions
char* last_token(char** args) {
    int i = 0;
    while(args[i] != NULL) { i++; }
    if(i > 0) {
        return args[i-1];
    } else {
        return NULL;
    }
}

char* trim(char* str)
{
  char *end;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

//Built-in Functions
int help_command(char** args, char* prev_command, char* line) {
    printf("Here are all the built-in commands:\n");
    printf("\033[1;32mhelp\033[0m   - displays this text\n");
    printf("\033[1;32mcd\033[0m     - changes the current working directory\n");
    printf("\033[1;32mmkdir\033[0m  - makes a new directory\n");
    printf("\033[1;32mexit\033[0m   - exits this shell\n");
    printf("\033[1;32m!!\033[0m     - repeats the last command\n");
    printf("\033[1;32mclear\033[0m  - clears the screen\n");
    return 1;
}

int cd_command(char** args, char* prev_command, char* line) {
    if (args[1] == NULL) {
        perror("Chris_Shell expected argument to \"cd\"");
        return 1;
    } else {
        if (chdir(args[1]) != 0) {
            perror("Chris_Shell cd error");
        }
    }
    return 1;
}

int mkdir_command(char** args, char* prev_command, char* line) {
    if (args[1] == NULL) {
        perror("Chris_Shell expected argument to \"mkdir\"");
        return 1;
    } else {
        // Attempt to create the directory with default permissions (0755)
        if (mkdir(args[1], 0755) != 0) {
            perror("Chris_Shell mkdir error");
        }
    }
    return 1;
}

int exit_command(char** args, char* prev_command, char* line) {
    return 0; //The status gets set to 0 then the shell exits after mem cleanup
}

int recall_command(char** args, char* prev_command, char* line) {
    // Check if a previous command exists
    if (prev_command == NULL || strlen(prev_command) == 0) {
        perror("Chris_Shell no previous command to execute");
        return 1;
    }

    // Prepare a buffer to hold the combined command
    size_t new_command_length = strlen(prev_command) + strlen(line) + 1;
    char *new_command = malloc(new_command_length);

    if (!new_command) {
        perror("Chris_Shell recall_command buffer allocation failed");
        return 1;
    }

    // Copy the previous command and concatenate the rest of the line
    strcpy(new_command, prev_command);
    strcat(new_command, line + 2); // Skip the "!!"

    printf("%s\n", new_command); // Print the previous command as feedback to the user

    char **new_args = shell_line_split(new_command);
    
    if (!new_args) {
        perror("Chris_Shell failed to tokenize previous command");
        return 1;
    }

    int status = shell_exec(new_args, prev_command, new_command);

    // Free allocated memory for the tokenized previous command
    for (int i = 0; new_args[i] != NULL; i++) {
        free(new_args[i]);
    }
    free(new_args);
    free(new_command);

    return status;
}

int clear_command(char** args, char* prev_command, char* line) {
    // Use ANSI escape codes to clear the screen
    printf("\033[H\033[J");

    return 1;
}

int echo_command(char* line) {
    //This function will only be called when echo is the last token

    line = trim(line); //Trim whitespace at end

    size_t len = strlen(line) - 4, i = 0;
    
    while (i < len) {
        if (line[i] == ' ') {
            printf("SPACE\n");
            i++;
        } else if (line[i] == '|') {
            printf("PIPE\n");
            i++;
        } else {
            while (i < len && line[i] != ' ' && line[i] != '|') {
                putchar(line[i]);
                i++;
            }
            printf("\n");
        }
    }
    return 1;
}

//Main
int main(int argc, char* argv[]) {
    char *line;
    char *prev_command = NULL;
    char **args;
    int status = 1;

    print_intro();

    //First load the line into a buffer
    //Second tokenize each word (split by a space or pipe)
    //Finally, execute any commands or programs
    do {
        print_dir();
        line = shell_read_line();
        args = shell_line_split(line);
        status = shell_exec(args, prev_command, line);

        // Allocate memory for prev_command before copying
        if (prev_command == NULL) {
            prev_command = malloc(strlen(line) + 1);
        } else {
            prev_command = realloc(prev_command, strlen(line) + 1);
        }

        if (prev_command == NULL) {
            perror("Chris_Shell prev_command allocation error");
            exit(EXIT_FAILURE);
        }

        if(args[0] != NULL && strcmp(args[0], "!!")) {
            strcpy(prev_command, line);
        } 

        for(int i = 0; args[i] != NULL; i++) {
            free(args[i]);
        }
        free(args);
        free(line);
        
    } while(status);

    free(prev_command);

    return 0;

}