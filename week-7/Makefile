
FLAGS = -Wall -pthread

all: t0 t1 t0_cv

clean:
	rm -f t0 t1 t0_cv

t0: t0.c
	gcc -o t0 t0.c $(FLAGS)

t1: t1.c
	gcc -o t1 t1.c $(FLAGS)

t0_cv: t0_cv.c
	gcc -o t0_cv t0_cv.c $(FLAGS)
