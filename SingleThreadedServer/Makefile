# Makefile for the server

CC = gcc
CCFLAGS = -Wall -Wextra -Wpedantic -Wshadow -O2 -g
COMPILECC = ${CC} ${CCFLAGS}
MODULES = server
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