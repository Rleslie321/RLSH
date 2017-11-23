#   $Id: Makefile,v 1.3 2017/04/23 22:49:02 leslier4 Exp $

#Robert Leslie
#4/22/2017
#CSCI 352
#Assignment 3

#This is a Makefile for msh

CC = gcc
CFLAGS = -g -Wall
DEPS = proto.h
OBJS = msh.o arg_parse.o builtin.o expand.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

msh: $(OBJS)
	gcc -o $@ $^ $(CFLAGS)

clean:
	-rm -f *.o *~ msh
