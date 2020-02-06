LD = g++
CC = g++

wards.o: *.cpp *.h
	$(CC) *.cpp -o wards.o -I/usr/local/include -L/usr/local/lib -lgsl -lm -O3

clean:  
	rm wards.o
