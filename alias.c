#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALIASES 100         //최대 alias 수

typedef struct {            //alias 구조체
    char alias[50];
    char command[200];
} Alias;

Alias alias_list[MAX_ALIASES];
int alias_count = 0;                //alias 개수 추적 변수

void add_alias(const char* alias, const char* command) {        //alias 추가
    if (alias_count < MAX_ALIASES) {                            //alias 자리가 남아있으면
        strcpy(alias_list[alias_count].alias, alias);           //alias를 복사
        strcpy(alias_list[alias_count].command, command);
        alias_count++;
    }  
    else {
        fprintf(stderr, "alias리스트 꽉 찼음.\n");
    }
}

void print_alias(const char* alias) {                   //인자가 1개면 alias 리스트 출력
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(alias_list[i].alias, alias) == 0) {      //문자열 비교
            printf("alias %s='%s'\n", alias_list[i].alias, alias_list[i].command);
            return;
        }
    }
    printf("alias: %s: not found\n", alias);            //없으면 없다고 출력
}

void print_all_aliases() {                              //모든 alias 리스트 출력
    for (int i = 0; i < alias_count; i++) {
        printf("alias %s='%s'\n", alias_list[i].alias, alias_list[i].command);
    }
}

void process_aliases(char** args) {                     //alias 설정
    
    int argc = 0;
    while (args[argc] != NULL) {
        argc++;
    }

    if (argc == 1) {                                    
        // 인자가 없으면 모든 alais 출력
        print_all_aliases();
    }
    else if (argc == 2) {
        // 인자가 1개면 모든 alias 출력
        print_alias(args[1]);
    }
    else if (argc >= 3) {
        //인자가 2개 이상이면
        char command[200] = "";
        for (int i = 2; args[i] != NULL; i++) {
            if (i > 2) {
                strcat(command, " ");
            }
            strcat(command, args[i]);
        }
        add_alias(args[1], command);
    }
    else {
        fprintf(stderr, "Usage: [alias] [command]\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {
    // Convert argv to args for the process_aliases function
    char** args = argv;
    process_aliases(args);
    return EXIT_SUCCESS;
}
