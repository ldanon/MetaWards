LD = g++
CC = g++

wards.o: *.cpp *.h
	$(CC) *.cpp -o wards.o $(shell gsl-config --cflags) $(shell gsl-config --libs) -lgsl -lm -O3

clean:  
	rm wards.o