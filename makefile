FLAGS = -DDEBUG
LIBS = -lm

NVCC = nvcc
CC = gcc

ALWAYS_REBUILD = makefile

nbody: nbody.o compute.o
	$(NVCC) $(FLAGS) nbody.o compute.o -o nbody $(LIBS)

nbody.o: nbody.c planets.h config.h vector.h $(ALWAYS_REBUILD)
	$(CC) $(FLAGS) -c nbody.c -o nbody.o

compute.o: compute.c config.h vector.h $(ALWAYS_REBUILD)
	$(NVCC) $(FLAGS) -x cu -c compute.c -o compute.o

clean:
	rm -f *.o nbody
