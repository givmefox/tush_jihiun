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

const char error_message[30] = "An error has occurred\n";

void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

char *read_command_line() { // command line 읽기
    char *line = NULL;      // 라인 저장할 char 포인터
    size_t len = 0;         // 라인 길이
    if (getline(&line, &len, stdin) == -1) { // getline을 stdin에서 읽기
        if (feof(stdin)) {      
            exit(0);  // EOF, exit shell
        } else {
            print_error(); // getline() 에러
            exit(1);
        }
    }
    return line; // line 리턴
}

char **parse_command_line(char *line) { // command line parsing, line을 입력 받는다.
    char **tokens = malloc(64 * sizeof(char*)); // 이중 포인터로 토큰 배열 생성
    char *token;
    int i = 0;

    while ((token = strsep(&line, " \t\r\n")) != NULL) { // strsep() 공백으로 토큰 구분
        if (strlen(token) > 0) { // 인자가 있으면
            tokens[i++] = token; // 토큰 배열에 추가
        }
    }
    tokens[i] = NULL; // 끝에 NULL 추가
    return tokens; // 토큰 배열 리턴
}

bool is_builtin_command(char *cmd) { // 내장 명령 확인 token 입력 받기
    const char *builtin_commands[] = {"cd", "exit", "path", "clear", "echo", "alias", "jobs"}; // 내장명령 배열
    for (int i = 0; i < sizeof(builtin_commands) / sizeof(char*); i++) { // 배열 인자수 만큼 반복
        if (strcmp(cmd, builtin_commands[i]) == 0) { // 내장명령 확인
            return true; // 내장명령이면 true 반환
        }
    }
    return false;       
}

void execute_builtin_command(char **args) { // 내장명령 실행
    if (strcmp(args[0], "cd") == 0) { // cd 실행
        changedir(args);
    } else if (strcmp(args[0], "exit") == 0) { // exit
        printf("GOOD BYE!\n");
        exit(0);
    } else if (strcmp(args[0], "path") == 0) { // path
        path(args);
    } else if (strcmp(args[0], "clear") == 0) { // clear
        clear_screen();
    } else if (strcmp(args[0], "echo") == 0) { // echo
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
    } else if (strcmp(args[0], "alias") == 0) { // alias
        print_error(); // alias 미구현
    } else if (strcmp(args[0], "jobs") == 0) { // jobs
        print_error(); // jobs 미구현
    }
}

void handle_redirection(char **args) { // 리다이렉션 함수, 인자 배열 입력
    for (int i = 0; args[i] != NULL; i++) { // 인자 수 만큼 반복
        if (strcmp(args[i], ">") == 0) { // 출력 리다이렉션이 있으면
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // 리다이렉션 다음에 있는 파일을 생성하고 연다.
            if (fd < 0) { // open 에러
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO); // 파일 출력 바꿔주기
            close(fd);
            args[i] = NULL; // 리다이렉션 부분 삭제
            break;
        } else if (strcmp(args[i], "2>") == 0) { // 표준 에러 리다이렉션
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDERR_FILENO); // 표준 에러 파일로 리다이렉션
            close(fd);
            args[i] = NULL; // 리다이렉션 부분 삭제
            break;
        }
    }
}

void execute_external_command(char **args) { // 외부 명령 실행
    pid_t pid = fork(); // fork()로 자식 프로세스 생성
    if (pid == 0) {
        handle_redirection(args); // 리다이렉션 검사
        execvp(args[0], args); // execvp()로 프로세스 변경
        print_error();
        exit(1);
    } else if (pid < 0) { // fork() 오류 처리문
        print_error();
        exit(1);
    } else { // pid>0이면 부모 프로세스이므로 기다린다.
        wait(NULL);
    }
}

void execute_parallel_commands(char **args) { // 병렬 실행
    int i = 0;
    while (args[i] != NULL) { // 토큰 배열 한바퀴 돌기
        if (strcmp(args[i], "&") == 0) { // '&' 찾기
            args[i] = NULL; // '&' 없애기        
            pid_t pid = fork(); // fork 자식 프로세스 생성
            if (pid == 0) { // 자식 프로세스이면
                execute_external_command(args); // 외부 명령어 실행
                exit(0);
            } else if (pid < 0) { // fork() 오류
                print_error();
                exit(1);
            }
            args = &args[i + 1]; // 다음 명령어 실행
            i = 0;
        } else {
            i++; // & 아니면 다음 연산자 실행
        }
    }
    execute_external_command(args);
}

void execute_pipeline(char **args) { // 파이프라인 수행 함수
    int pipefd[2]; // 파이프 파일 디스크립터를 저장할 배열
    pid_t pid1, pid2; // 두 개의 프로세스를 저장할 변수
    int i = 0;

    // 파이프라인 구분자인 '|'를 찾기 위해 인덱스를 증가시킴
    while (args[i] != NULL && strcmp(args[i], "|") != 0) {
        i++;
    }

    // '|'가 없으면 단일 명령어로 간주하고 실행
    if (args[i] == NULL) {
        execute_external_command(args);
        return;
    }

    // '|'를 NULL로 대체하여 명령어를 분리
    args[i] = NULL;
    char **first_command = args; // 첫 번째 명령어
    char **second_command = &args[i + 1]; // 두 번째 명령어

    // 파이프 생성
    if (pipe(pipefd) == -1) {
        print_error();
        exit(1);
    }

    // 첫 번째 자식 프로세스 생성
    pid1 = fork();
    if (pid1 == 0) { // 자식 프로세스
        // 파이프의 쓰기 끝을 표준 출력으로 복제
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); // 읽기 끝 닫기
        close(pipefd[1]); // 쓰기 끝 닫기
        execvp(first_command[0], first_command); // 첫 번째 명령어 실행
        print_error();
        exit(1);
    } else if (pid1 < 0) { // fork 실패 시 오류 처리
        print_error();
        exit(1);
    }

    // 두 번째 자식 프로세스 생성
    pid2 = fork();
    if (pid2 == 0) { // 자식 프로세스
        // 파이프의 읽기 끝을 표준 입력으로 복제
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]); // 읽기 끝 닫기
        close(pipefd[1]); // 쓰기 끝 닫기
        execvp(second_command[0], second_command); // 두 번째 명령어 실행
        print_error();
        exit(1);
    } else if (pid2 < 0) {
        print_error();
        exit(1);
    }

    // 부모 프로세스에서 파이프 닫기
    close(pipefd[0]);
    close(pipefd[1]);

    // 자식 프로세스가 종료될 때까지 대기
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void handle_sigint(int sig) {
    printf("\n종료하려면 exit 입력\n진행하려면 Enter키 입력\n");
    fflush(stdout);
}

void changedir(char** args) {
    const char* dir;

    if (args[1] == NULL) {
        // 인자가 없을 경우 홈 디렉토리로 이동
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

    // 디렉토리 변경 시도
    if (chdir(dir) != 0) {
        print_error();
    }
}

void path(char** args) {
    const char* old_path = getenv("PATH"); // 기존 PATH 값 가져오기
    char new_path[4096] = ""; // 새로운 PATH 값을 저장할 배열
    if (old_path == NULL) {
        print_error();
        return;
    }

    if (args[1] == NULL) { // 인자가 없으면 단순 PATH 변수 출력
        printf("%s\n", old_path);     
    } else {
        strncpy(new_path, old_path, sizeof(new_path) - 1); // 기존 PATH 값을 복사
        // 새 PATH 문자열 생성
        for (int i = 1; args[i] != NULL; i++) {
            if (strlen(new_path) + strlen(args[i]) + 1 < sizeof(new_path)) { // 공간이 충분한지 확인
                if (strlen(new_path) > 0) {
                    strcat(new_path, ":"); // 기존 PATH가 있으면 구분자 ':' 추가
                }
                strcat(new_path, args[i]); // 새로운 경로 추가
            } else {
                print_error();
                return;
            }
        }
        setenv("PATH", new_path, 1); // 새로운 PATH 설정
    }
}

void clear_screen() {
    printf("\033[H\033[J"); // clear
}

int main() {
    signal(SIGINT, handle_sigint);

    while (1) {
        printf("prompt> ");
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
            int has_pipeline = 0; // 파이프라인 검사
            for (int i = 0; args[i] != NULL; i++) {
                if (strcmp(args[i], "|") == 0) { // 있으면 플래그 바꿔주기
                    has_pipeline = 1;
                    break;
                }
            }

            if (has_pipeline) {
                execute_pipeline(args); // 파이프라인 실행
            } else {
                int has_parallel = 0; // 병령명령 플래그
                for (int i = 0; args[i] != NULL; i++) {
                    if (strcmp(args[i], "&") == 0) { // 병렬 명령 커맨드인지 검사
                        has_parallel = 1;
                        break;
                    }
                }

                if (has_parallel) { // 병렬 명령 실행
                    execute_parallel_commands(args);
                } else {
                    execute_external_command(args);
                }
            }
        }

        free(line);
        free(args);
    }
    return 0;
}
