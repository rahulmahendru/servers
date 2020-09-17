# Makefile for Multithreaded server
CC = gcc
CCFLAGS = -Wall -Wextra -Wpedantic -Wshadow -O2 -g -pthread
COMPILECC = ${CC} ${CCFLAGS}
MODULES = server queue
MAIN = main
CHEADER = ${MODULES:=.h}
CSOURCE = ${MODULES:=.c} main.c
EXECBIN = httpserver
OBJECTS = ${CSOURCE:.c=.o}

all: ${EXECBIN}

${EXECBIN}: ${OBJECTS}
	${COMPILECC} -o $@ $^

%.o : %.c
	${COMPILECC} -c $^

clean:
	rm -f ${OBJECTS}

spotless:
	rm ${EXECBIN}
