
all: wave

wave: wave.h wave.c project4.c
	gcc -std=c99 wave.c project4.c -o wave

clean:
	rm -f *.o wave

