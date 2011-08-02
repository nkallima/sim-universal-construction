CC = gcc
CFLAGSX86 = -pipe -O3 -ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions -lpthread -DN_THREADS=32 -DUSE_CPUS=32 -D_GNU_SOURCE

X86:
# A Fecth&Multiply, a stack and a queue object based on P-Sim universal construction
	$(CC) $(CFLAGSX86) sim.c -o Sim.run
	$(CC) $(CFLAGSX86) simstack.c -o SimStack.run
	$(CC) $(CFLAGSX86) simqueue.c -o SimQueue.run
# A lock-free Fecth&Multiply, a lock-free stack and a lock-free queue object 
	$(CC) $(CFLAGSX86) lfobject.c -o LFobject.run
	$(CC) $(CFLAGSX86) lfstack.c -o LFStack.run
	$(CC) $(CFLAGSX86) msqueue.c -o MSQueue.run
# A Fecth&Multiply, a stack and a queue object implemented with high performance spinlocks
	$(CC) $(CFLAGSX86) luobject.c -o LUobject.run
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run

clean:
	rm -f *.o *.run a.out res.txt

