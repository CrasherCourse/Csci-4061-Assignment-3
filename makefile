# Makefile for piper

piper: piper.o
	gcc -o piper piper.o

piper.o: piper.c
	gcc -o piper.o -g -c  piper.c

clean:
	rm *.o
	rm piper
