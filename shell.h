#ifndef SHELL_H
#define SHELL_H

#define BUFFER_SIZE 1024
#define TOKENS_COUNT 16

//Declarations
void print_intro();
void print_dir();
char* shell_line_read();
char** shell_line_split(char* line);
int shell_exec(char** args, char* prev_command, char* line);
int shell_launch(char** args);
int shell_launch_with_output_redirection(char** args, char* output_file);
int shell_launch_with_input_redirection(char** args, char* input_file);
int shell_launch_pipe(char** args1, char** args2);

int help_command(char** args, char* prev_command, char* line);
int cd_command(char** args, char* prev_command, char* line);
int mkdir_command(char** args, char* prev_command, char* line);
int exit_command(char** args, char* prev_command, char* line);
int recall_command(char** args, char* prev_command, char* line);
int clear_command(char** args, char* prev_command, char* line);
int echo_command(char* line);

char* last_token(char** args);

//Build in commands
static char *built_in_commands[] = {
  "help",
  "cd",
  "mkdir",
  "exit",
  "!!",
  "clear",
  ((void*)0) //NULL
};

int (*built_in_functions[]) (char **, char*, char*) = {
    &help_command,
    &cd_command,
    &mkdir_command,
    &exit_command,
    &recall_command,
    &clear_command
};

#endif