CC = gcc
FLAGS = -Wall -Wextra -Wpedantic -std=c11 -D_GNU_SOURCE -g

ex12: main.o
	$(CC) $^ -o $@

main.o: main.c
	$(CC) $(FLAGS) -c $^ -o $@

clean:
	rm -f ex12 *.o 
