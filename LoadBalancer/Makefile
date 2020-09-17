# Makefile for the load balancer

CC = gcc
CCFLAGS = -Wall -Wextra -Wpedantic -Wshadow -O2 -g -pthread
COMPILECC = ${CC} ${CCFLAGS}
MODULES = loadbalancer queue
CHEADER = ${MODULES:=.h}
CSOURCE = ${MODULES:=.c}
EXECBIN = loadbalancer
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
