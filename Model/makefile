LD = gcc
CC = gcc

MetaWards.o: *.c *.h
	$(CC) *.c -o MetaWards.o $(shell gsl-config --cflags) $(shell gsl-config --libs) -lgsl -lm -O3

clean:  
	rm MetaWards.o
