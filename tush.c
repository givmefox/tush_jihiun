#include "tush.h"


// jobs관련 구조체
typedef struct {
    pid_t pid;        // 프로세스 ID
    char command[256];// 명령어
    int index;        // 작업 번호
} Job;


const char error_message[30];    //에러메세지

Job jobs[64];       //백그라운드 잡 넣을 배열
int job_count;      //백그라운드 추적 수
int job_index;      //백그라운드 작업 인덱스

// 오류 메시지를 출력하는 함수
void print_error() {
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// 명령어 입력을 읽어오는 함수
char *read_command_line() {
    char *line = NULL;      // 입력된 라인을 저장할 포인터
    size_t len = 0;         // 라인의 길이를 저장할 변수
    if (getline(&line, &len, stdin) == -1) { // 표준 입력으로부터 라인을 읽음
        if (feof(stdin)) { // EOF를 만나면
            exit(0);  // 쉘 종료
        } else {
            print_error(); // getline()에서 오류 발생 시
            exit(1); // 프로그램 종료
        }
    }
    return line; // 읽어온 라인을 반환
}

// 명령어 입력을 파싱하여 토큰 배열로 반환하는 함수
char **parse_command_line(char *line) {
    char **tokens = malloc(64 * sizeof(char*)); // 64개의 토큰을 저장할 메모리 할당
    char *token;
    int i = 0;

    while ((token = strsep(&line, " \t\r\n")) != NULL) { // 공백, 탭, 개행문자로 토큰을 분리
        if (strlen(token) > 0) { // 토큰이 비어있지 않으면
            tokens[i++] = token; // 토큰을 배열에 추가
        }
    }
    tokens[i] = NULL; // 배열의 마지막에 NULL 추가
    return tokens; // 토큰 배열 반환
}

// 내장 명령어인지 확인하는 함수
bool is_builtin_command(char *cmd) {
    const char *builtin_commands[] = {"cd", "exit", "path", "clear", "echo", "alias", "jobs"}; // 내장 명령어 목록
    for (int i = 0; i < sizeof(builtin_commands) / sizeof(char*); i++) { // 목록을 순회하면서
        if (strcmp(cmd, builtin_commands[i]) == 0) { // 명령어가 목록에 있으면
            return true; // true 반환
        }
    }
    return false; // 목록에 없으면 false 반환
}

// 내장 명령어를 실행하는 함수
void execute_builtin_command(char **args) {
    if (strcmp(args[0], "cd") == 0) { // 명령어가 cd이면
        changedir(args); // changedir 함수 호출
    } else if (strcmp(args[0], "exit") == 0) { // 명령어가 exit이면
        printf("GOOD BYE!\n");
        exit(0); // 프로그램 종료
    } else if (strcmp(args[0], "path") == 0) { // 명령어가 path이면
        path(args); // path 함수 호출
    } else if (strcmp(args[0], "clear") == 0) { // 명령어가 clear이면
        clear_screen(); // clear_screen 함수 호출
    } else if (strcmp(args[0], "echo") == 0) { // 명령어가 echo이면
        fun_echo(args); // fun_echo 함수 호출
    } else if (strcmp(args[0], "jobs") == 0) { // 명령어가 jobs이면
        show_jobs(); // show_jobs 함수 호출
    }
}

// 리다이렉션을 처리하는 함수
void handle_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) { // args 배열을 순회하면서
        if (strcmp(args[i], ">") == 0) { // 출력 리다이렉션을 찾으면
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // 파일 생성 및 열기
            if (fd < 0) { // 파일 열기에 실패하면
                print_error(); // 오류 메시지 출력
                exit(1); // 프로그램 종료
            }
            dup2(fd, STDOUT_FILENO); // 파일 디스크립터를 표준 출력으로 변경
            close(fd); // 파일 디스크립터 닫기
            args[i] = NULL; // 리다이렉션 연산자를 NULL로 대체
            break; // 루프 탈출
        } else if (strcmp(args[i], "2>") == 0) { // 표준 에러 리다이렉션을 찾으면
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // 파일 생성 및 열기
            if (fd < 0) { // 파일 열기에 실패하면
                print_error(); // 오류 메시지 출력
                exit(1); // 프로그램 종료
            }
            dup2(fd, STDERR_FILENO); // 파일 디스크립터를 표준 에러로 변경
            close(fd); // 파일 디스크립터 닫기
            args[i] = NULL; // 리다이렉션 연산자를 NULL로 대체
            break; // 루프 탈출
        } else if (strcmp(args[i], ">>") == 0) { // 출력 추가 리다이렉션을 찾으면
            int fd = open(args[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644); // 파일 생성 및 열기
            if (fd < 0) { // 파일 열기에 실패하면
                print_error(); // 오류 메시지 출력
                exit(1); // 프로그램 종료
            }
            dup2(fd, STDOUT_FILENO); // 파일 디스크립터를 표준 출력으로 변경
            close(fd); // 파일 디스크립터 닫기
            args[i] = NULL; // 리다이렉션 연산자를 NULL로 대체
            break; // 루프 탈출
        } else if (strcmp(args[i], "<") == 0) { // 입력 리다이렉션을 찾으면
            int fd = open(args[i + 1], O_RDONLY); // 파일 열기
            if (fd < 0) { // 파일 열기에 실패하면
                print_error(); // 오류 메시지 출력
                exit(1); // 프로그램 종료
            }
            dup2(fd, STDIN_FILENO); // 파일 디스크립터를 표준 입력으로 변경
            close(fd); // 파일 디스크립터 닫기
            args[i] = NULL; // 리다이렉션 연산자를 NULL로 대체
            break; // 루프 탈출
        }
    }
}

// 외부 명령어를 실행하는 함수
void execute_external_command(char **args) {
    pid_t pid = fork(); // 자식 프로세스 생성
    if (pid == 0) { // 자식 프로세스 실행 코드
        execvp(args[0], args); // 명령어 실행
        print_error(); // execvp가 실패하면 오류 메시지 출력
        exit(1); // 자식 프로세스 종료
    } else if (pid < 0) { // fork 실패 시
        print_error(); // 오류 메시지 출력
        exit(1); // 프로그램 종료
    } else { // 부모 프로세스 실행 코드
        wait(NULL); // 자식 프로세스 종료 대기
    }
}

// 병렬 명령어를 실행하는 함수
void execute_parallel_commands(char **args) {
    int i = 0;
    while (args[i] != NULL) { // args 배열을 순회하면서
        if (strcmp(args[i], "&") == 0) { // 병렬 명령어 구분자를 찾으면
            args[i] = NULL; // 구분자를 NULL로 대체
            pid_t pid = fork(); // 자식 프로세스 생성
            if (pid == 0) { // 자식 프로세스 실행 코드
                execute_external_command(args); // 외부 명령어 실행
                exit(0); // 자식 프로세스 종료
            } else if (pid < 0) { // fork 실패 시
                print_error(); // 오류 메시지 출력
                exit(1); // 프로그램 종료
            } else { // 부모 프로세스 실행 코드
                printf("[%d] %d\n", job_index, pid); // 작업 번호와 PID 출력
                jobs[job_count].pid = pid; // 작업 정보 저장
                strncpy(jobs[job_count].command, args[0], 255); // 명령어 저장
                jobs[job_count].index = job_index; // 작업 번호 저장
                job_count++; // 작업 개수 증가
                job_index++; // 작업 번호 증가
            }
            args = &args[i + 1]; // 다음 명령어로 이동
            i = 0; // 인덱스 초기화
        } else {
            i++;
        }
    }
    if (args[0] != NULL) { // 남은 명령어가 있으면
        execute_external_command(args); // 외부 명령어 실행
    }
} 

// 파이프라인 명령어를 실행하는 함수
void execute_pipeline(char **args) {
    int pipefd[2]; // 파이프 파일 디스크립터를 저장할 배열
    pid_t pid1, pid2; // 두 개의 자식 프로세스 ID를 저장할 변수
    int i = 0;

    while (args[i] != NULL && strcmp(args[i], "|") != 0) { // 파이프라인 구분자 '|'를 찾기 위해 인덱스를 증가시킴
        i++;
    }

    if (args[i] == NULL) { // '|'가 없으면 단일 명령어로 간주하고 실행
        execute_external_command(args);
        return;
    }

    args[i] = NULL; // '|'를 NULL로 대체하여 명령어를 분리
    char **first_command = args; // 첫 번째 명령어
    char **second_command = &args[i + 1]; // 두 번째 명령어

    if (pipe(pipefd) == -1) { // 파이프 생성
        print_error(); // 파이프 생성 실패 시 오류 메시지 출력
        exit(1); // 프로그램 종료
    }

    pid1 = fork(); // 첫 번째 자식 프로세스 생성
    if (pid1 == 0) { // 첫 번째 자식 프로세스 실행 코드
        dup2(pipefd[1], STDOUT_FILENO); // 파이프의 쓰기 끝을 표준 출력으로 복제
        close(pipefd[0]); // 파이프의 읽기 끝 닫기
        close(pipefd[1]); // 파이프의 쓰기 끝 닫기
        execvp(first_command[0], first_command); // 첫 번째 명령어 실행
        print_error(); // execvp 실패 시 오류 메시지 출력
        exit(1); // 자식 프로세스 종료
    } else if (pid1 < 0) { // fork 실패 시
        print_error(); // 오류 메시지 출력
        exit(1); // 프로그램 종료
    }

    pid2 = fork(); // 두 번째 자식 프로세스 생성
    if (pid2 == 0) { // 두 번째 자식 프로세스 실행 코드
        dup2(pipefd[0], STDIN_FILENO); // 파이프의 읽기 끝을 표준 입력으로 복제
        close(pipefd[0]); // 파이프의 읽기 끝 닫기
        close(pipefd[1]); // 파이프의 쓰기 끝 닫기
        execvp(second_command[0], second_command); // 두 번째 명령어 실행
        print_error(); // execvp 실패 시 오류 메시지 출력
        exit(1); // 자식 프로세스 종료
    } else if (pid2 < 0) { // fork 실패 시
        print_error(); // 오류 메시지 출력
        exit(1); // 프로그램 종료
    }

    close(pipefd[0]); // 부모 프로세스에서 파이프의 읽기 끝 닫기
    close(pipefd[1]); // 부모 프로세스에서 파이프의 쓰기 끝 닫기

    waitpid(pid1, NULL, 0); // 첫 번째 자식 프로세스가 종료될 때까지 대기
    waitpid(pid2, NULL, 0); // 두 번째 자식 프로세스가 종료될 때까지 대기
}

// SIGINT 신호를 처리하는 함수
void handle_sigint(int sig) {
    printf("\n종료하려면 exit 입력\n진행하려면 Enter키 입력\n"); // 종료 안내 메시지 출력
    fflush(stdout); // 출력 버퍼 비우기
}

// 디렉토리 변경 함수
void changedir(char** args) {
    const char* dir;

    if (args[1] == NULL) { // 인자가 없으면 홈 디렉토리로 변경
        dir = getenv("HOME");
        if (dir == NULL) {
            print_error(); // 홈 디렉토리 가져오기 실패 시 오류 메시지 출력
            return;
        }
    } else if (args[2] != NULL) { // 인자가 너무 많으면 오류
        print_error();
        return;
    } else {
        dir = args[1]; // 지정된 디렉토리로 변경
    }

    if (chdir(dir) != 0) { // 디렉토리 변경
        print_error(); // 디렉토리 변경 실패 시 오류 메시지 출력
    }
}

// PATH 설정 함수
void path(char** args) {
    const char* old_path = getenv("PATH"); // 현재 PATH 가져오기
    char new_path[4096] = ""; // 새로운 PATH 저장할 변수
    if (old_path == NULL) {
        print_error(); // 현재 PATH 가져오기 실패 시 오류 메시지 출력
        return;
    }

    if (args[1] == NULL) { // 인자가 없으면 현재 PATH 출력
        printf("%s\n", old_path);
    } else {
        strncpy(new_path, old_path, sizeof(new_path) - 1); // 현재 PATH 복사
        for (int i = 1; args[i] != NULL; i++) { // 추가할 경로들을 순회하면서
            if (strlen(new_path) + strlen(args[i]) + 1 < sizeof(new_path)) { // 새로운 PATH가 버퍼 크기를 초과하지 않으면
                if (strlen(new_path) > 0) {
                    strcat(new_path, ":"); // 콜론으로 구분하여 추가
                }
                strcat(new_path, args[i]);
            } else {
                print_error(); // 버퍼 크기를 초과하면 오류 메시지 출력
                return;
            }
        }
        setenv("PATH", new_path, 1); // 새로운 PATH 설정
    }
}

// 화면을 지우는 함수
void clear_screen() {
    printf("\033[H\033[J"); // ANSI escape code를 사용하여 화면을 지움
}

// echo 함수
void fun_echo(char** args) {
    int newline = 1; // 개행 여부를 저장할 변수
    int start = 1; // 인자를 읽기 시작할 인덱스

    if (args[1] != NULL && strcmp(args[1], "-n") == 0) { // -n 옵션 확인
        newline = 0; // 개행하지 않음
        start = 2; // 인자를 2번째부터 읽음
    }

    for (int i = start; args[i]!=NULL; ++i) { // 인자를 순회하면서
        if (i > start) {
            printf(" "); // 인자 사이에 공백 추가
        }
        printf("%s", args[i]); // 인자 출력
    }

    if (newline) { // 개행이 필요하면
        printf("\n");
    }
    return;
}

// jobs 명령어를 처리하는 함수
void show_jobs() {
    printf("백그라운드 실행 목록:\n");
    for (int i = 0; i < job_count; i++) { // 작업 목록을 순회하면서
        printf("[%d] %d %s\n", jobs[i].index, jobs[i].pid, jobs[i].command); // 작업 정보 출력
    }
}

int main() {
    signal(SIGINT, handle_sigint); // SIGINT 신호 핸들러 설정

    while (1) { // 무한 루프
        printf("prompt> "); // 프롬프트 출력
        fflush(stdout); // 출력 버퍼 비우기

        char *line = read_command_line(); // 커맨드 읽기
        char **args = parse_command_line(line); // 커맨드 파싱

        if (args[0] == NULL) { // 아무것도 입력 안되면 다시 입력받기
            free(line);
            free(args);
            continue;
        }

        handle_redirection(args);   //리다이렉션 실행

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

        free(line); // 동적 할당된 메모리 해제
        free(args); // 동적 할당된 메모리 해제
    }

    return 0;
}
