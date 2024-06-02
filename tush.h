// tush.h

#ifndef TUSH_H
#define TUSH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

// 함수 선언
void clear_screen();
void changedir(char** args);
void path(char** args);
void fun_echo(char** args);
void show_jobs();
void handle_sigint(int sig);
char *read_command_line();
char **parse_command_line(char *line);
bool is_builtin_command(char *cmd);
void execute_builtin_command(char **args);
void handle_redirection(char **args);
void execute_external_command(char **args);
void execute_parallel_commands(char **args);
void execute_pipeline(char **args);
void print_error();



#endif