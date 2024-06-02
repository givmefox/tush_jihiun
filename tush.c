#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

void clear_screen();
void changedir(char** args);
void path(char** args);
void fun_echo(char** args);
void show_jobs();

const char error_message[30] = "An error has occurred\n";

// 작업을 추적하기 위한 구조체와 변수 선언
typedef struct {
    pid_t pid;
    char command[256];
    int index;
} Job;

Job jobs[64];
int job_count = 0;
int job_index = 1;

void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// 명령어 입력을 읽어오는 함수
char *read_command_line() {
    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, stdin) == -1) {
        if (feof(stdin)) {
            exit(0);
        } else {
            print_error();
            exit(1);
        }
    }
    return line;
}

// 명령어 입력을 파싱하는 함수
char **parse_command_line(char *line) {
    char **tokens = malloc(64 * sizeof(char*));
    char *token;
    int i = 0;

    while ((token = strsep(&line, " \t\r\n")) != NULL) {
        if (strlen(token) > 0) {
            tokens[i++] = token;
        }
    }
    tokens[i] = NULL;
    return tokens;
}

// 내장 명령어인지 확인하는 함수
bool is_builtin_command(char *cmd) {
    const char *builtin_commands[] = {"cd", "exit", "path", "clear", "echo", "alias", "jobs"};
    for (int i = 0; i < sizeof(builtin_commands) / sizeof(char*); i++) {
        if (strcmp(cmd, builtin_commands[i]) == 0) {
            return true;
        }
    }
    return false;
}

// 내장 명령어를 실행하는 함수
void execute_builtin_command(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        changedir(args);
    } else if (strcmp(args[0], "exit") == 0) {
        printf("GOOD BYE!\n");
        exit(0);
    } else if (strcmp(args[0], "path") == 0) {
        path(args);
    } else if (strcmp(args[0], "clear") == 0) {
        clear_screen();
    } else if (strcmp(args[0], "echo") == 0) {
        fun_echo(args);
    } else if (strcmp(args[0], "jobs") == 0) {
        show_jobs();
    }
}

// 리다이렉션을 처리하는 함수
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
            args[i] = NULL;
            break;
        } else if (strcmp(args[i], "2>") == 0) {
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
            args[i] = NULL;
            break;
        }
    }
}

// 외부 명령어를 실행하는 함수
void execute_external_command(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        handle_redirection(args);
        execvp(args[0], args);
        print_error();
        exit(1);
    } else if (pid < 0) {
        print_error();
        exit(1);
    } else {
        wait(NULL);
    }
}

// 병렬 명령어를 실행하는 함수
void execute_parallel_commands(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL;
            pid_t pid = fork();
            if (pid == 0) {
                execute_external_command(args);
                exit(0);
            } else if (pid < 0) {
                print_error();
                exit(1);
            } else {
                printf("[%d] %d\n", job_index, pid);
                jobs[job_count].pid = pid;
                strncpy(jobs[job_count].command, args[0], 255);
                jobs[job_count].index = job_index;
                job_count++;
                job_index++;
            }
            args = &args[i + 1];
            i = 0;
        } else {
            i++;
        }
    }
    if (args[0] != NULL) {
        execute_external_command(args);
    }
}

// 파이프라인 명령어를 실행하는 함수
void execute_pipeline(char **args) {
    int pipefd[2];
    pid_t pid1, pid2;
    int i = 0;

    while (args[i] != NULL && strcmp(args[i], "|") != 0) {
        i++;
    }

    if (args[i] == NULL) {
        execute_external_command(args);
        return;
    }

    args[i] = NULL;
    char **first_command = args;
    char **second_command = &args[i + 1];

    if (pipe(pipefd) == -1) {
        print_error();
        exit(1);
    }

    pid1 = fork();
    if (pid1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(first_command[0], first_command);
        print_error();
        exit(1);
    } else if (pid1 < 0) {
        print_error();
        exit(1);
    }

    pid2 = fork();
    if (pid2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(second_command[0], second_command);
        print_error();
        exit(1);
    } else if (pid2 < 0) {
        print_error();
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// SIGINT 신호를 처리하는 함수
void handle_sigint(int sig) {
    printf("\n종료하려면 exit 입력\n진행하려면 Enter키 입력\n");
    fflush(stdout);
}

// 디렉토리 변경 함수
void changedir(char** args) {
    const char* dir;

    if (args[1] == NULL) {
        dir = getenv("HOME");
        if (dir == NULL) {
            print_error();
            return;
        }
    } else if (args[2] != NULL) {
        print_error();
        return;
    } else {
        dir = args[1];
    }

    if (chdir(dir) != 0) {
        print_error();
    }
}

// PATH 설정 함수
void path(char** args) {
    const char* old_path = getenv("PATH");
    char new_path[4096] = "";
    if (old_path == NULL) {
        print_error();
        return;
    }

    if (args[1] == NULL) {
        printf("%s\n", old_path);
    } else {
        strncpy(new_path, old_path, sizeof(new_path) - 1);
        for (int i = 1; args[i] != NULL; i++) {
            if (strlen(new_path) + strlen(args[i]) + 1 < sizeof(new_path)) {
                if (strlen(new_path) > 0) {
                    strcat(new_path, ":");
                }
                strcat(new_path, args[i]);
            } else {
                print_error();
                return;
            }
        }
        setenv("PATH", new_path, 1);
    }
}

// 화면을 지우는 함수
void clear_screen() {
    printf("\033[H\033[J");
}

// echo 함수
void fun_echo(char** args) {
    int newline = 1;
    int start = 1;

    if (args[1] != NULL && strcmp(args[1], "-n") == 0) {
        newline = 0;
        start = 2;
    }

    for (int i = start; args[i]!=NULL; ++i) {
        if (i > start) {
            printf(" ");
        }
        printf("%s", args[i]);
    }

    if (newline) {
        printf("\n");
    }
    return;
}

// jobs 명령어를 처리하는 함수
void show_jobs() {
    printf("백그라운드 실행 목록:\n");
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s\n", jobs[i].index, jobs[i].pid, jobs[i].command);
    }
}

int main() {
    signal(SIGINT, handle_sigint);

    while (1) {
        printf("prompt> ");
        fflush(stdout);

        char *line = read_command_line(); // 커맨드 읽기
        char **args = parse_command_line(line); // 커맨드 파싱

        if (args[0] == NULL) { // 아무것도 입력 안되면 다시 입력받기
            free(line);
            free(args);
            continue;
        }

        if (is_builtin_command(args[0])) { // 내장명령 검사, 내장명령이면
            execute_builtin_command(args); // 내장명령 실행
        } else {
            bool parallel = false; // 병렬 명령어 플래그
            bool pipeline = false; // 파이프라인 명령어 플래그
            for (int i = 0; args[i] != NULL; i++) {
                if (strcmp(args[i], "&") == 0) {
                    parallel = true;
                } else if (strcmp(args[i], "|") == 0) {
                    pipeline = true;
                }
            }

            if (parallel) {
                execute_parallel_commands(args); // 병렬 명령어 실행
            } else if (pipeline) {
                execute_pipeline(args); // 파이프라인 명령어 실행
            } else {
                execute_external_command(args); // 외부 명령어 실행
            }
        }

        free(line);
        free(args);
    }

    return 0;
}
