LD = g++
CC = g++

wards.o: *.cpp *.h
	$(CC) *.cpp -o wards.o -I/opt/local/include -L/opt/local/lib -lgsl -lm -O5

clean:  
	rm wards.o
