README for tush 쉘 프로그램
개요
tush (Tiny UNIX Shell)은 유닉스 셸과 유사한 명령어를 실행하도록 설계된 커스텀 셸 프로그램입니다. 내장 명령어와 외부 명령어를 모두 지원하며, 리다이렉션, 파이프라인, 백그라운드 작업 관리 등의 추가 기능을 제공합니다.

기능
내장 명령어: cd, exit, path, clear, echo, jobs 등의 명령어 포함.
외부 명령어: 시스템 PATH에 있는 외부 바이너리 실행.
리다이렉션: 입력 및 출력 리다이렉션 지원 (>, 2>, >>, <).
파이프라인: | 연산자를 사용하여 명령어 체이닝 가능.
백그라운드 작업: & 연산자를 사용하여 명령어를 백그라운드에서 실행하고 jobs 명령어로 관리 가능.
신호 처리: SIGINT 신호를 사용자 친화적인 메시지로 처리.
컴파일
tush 프로그램을 컴파일하려면 다음 명령어를 사용하세요:

sh
코드 복사
gcc -o tush tush.c
사용법
컴파일된 바이너리를 실행하세요:

sh
코드 복사
./tush
프롬프트 (prompt>)가 나타나면 명령어를 입력할 수 있습니다.

내장 명령어
cd
현재 디렉토리를 변경합니다.

sh
코드 복사
cd <directory>
디렉토리를 지정하지 않으면 홈 디렉토리로 이동합니다.

exit
셸을 종료합니다.

sh
코드 복사
exit
path
PATH 변수를 표시하거나 설정합니다.

sh
코드 복사
path
현재 PATH를 표시합니다.

sh
코드 복사
path <path1> <path2> ...
지정된 디렉토리로 PATH를 설정합니다.

clear
터미널 화면을 지웁니다.

sh
코드 복사
clear
echo
제공된 인수를 터미널에 출력합니다.

sh
코드 복사
echo <message>
메시지 뒤에 개행을 하지 않으려면 -n 옵션을 사용합니다:

sh
코드 복사
echo -n <message>
jobs
모든 백그라운드 작업을 나열합니다.

sh
코드 복사
jobs
명령어 실행
외부 명령어
내장 명령어로 인식되지 않는 모든 명령어는 외부 명령어로 처리됩니다.

sh
코드 복사
ls -l
리다이렉션
명령어의 출력을 파일로 리다이렉션합니다.

sh
코드 복사
command > output.txt
명령어의 출력을 파일에 추가합니다.

sh
코드 복사
command >> output.txt
명령어의 에러 출력을 파일로 리다이렉션합니다.

sh
코드 복사
command 2> error.txt
파일에서 입력을 리다이렉션합니다.

sh
코드 복사
command < input.txt
파이프라인
파이프라인 연산자(|)를 사용하여 명령어를 체이닝합니다.

sh
코드 복사
command1 | command2
백그라운드 작업
& 연산자를 사용하여 명령어를 백그라운드에서 실행합니다.

sh
코드 복사
command &
백그라운드 작업을 나열하려면 jobs 명령어를 사용합니다.

신호 처리
Ctrl+C를 누르면 셸은 다음 메시지를 표시합니다:

bash
코드 복사
종료하려면 exit 입력
진행하려면 Enter키 입력
이 메시지는 우발적인 셸 종료를 방지하는 데 도움을 줍니다.

예제 사용법
sh
코드 복사
prompt> cd /usr
prompt> ls -l | grep bin
prompt> echo "Hello, World!" > hello.txt
prompt> cat < hello.txt
prompt> sleep 60 &
prompt> jobs
prompt> exit
오류 처리
오류가 발생하면 표준 오류 메시지를 출력합니다:

go
코드 복사
An error has occurred
정리
프로그램은 명령어 실행 후 자원을 적절히 해제하고 표준 파일 디스크립터를 복원합니다.

참고 사항
필수 시스템 호출(예: fork, execvp, pipe, dup2)이 환경에서 지원되는지 확인하세요.
이 셸은 최소한의 구현이며 bash나 zsh와 같은 보다 견고한 셸의 일부 기능이 부족할 수 있습니다.
라이선스
이 프로젝트는 MIT 라이선스에 따라 라이선스가 부여됩니다. 자세한 내용은 LICENSE 파일을 참조하세요.

감사의 말
기여자들과 오픈 소스 커뮤니티에 감사드립니다. 이들의 귀중한 지원과 자원이 큰 도움이 되었습니다.
