# Makefile for tush shell

# 컴파일러와 컴파일러 플래그
CC = gcc
CFLAGS = -Wall -g

# 바이너리 파일 이름
TARGET = tush

# 소스 파일
SRCS = tush.c

# 오브젝트 파일
OBJS = $(SRCS:.c=.o)

# 기본 타겟
all: $(TARGET)

# 타겟을 링크하여 실행 파일 생성
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# 소스 파일을 컴파일하여 오브젝트 파일 생성
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# clean 타겟: 생성된 파일들을 삭제
clean:
	rm -f $(TARGET) $(OBJS)

# .PHONY를 사용하여 가상의 타겟임을 명시
.PHONY: all clean

