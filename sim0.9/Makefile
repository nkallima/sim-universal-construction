CC = gcc
CFLAGSSPARC = -pipe -O3 -mtune=niagara2 -m32 -lrt -finline-functions -lpthread -DN_THREADS=128 -DUSE_CPUS=128 -D_GNU_SOURCE
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
# A Fecth&Multiply, a stack and a queue object implemented with Oyama, et al method
	$(CC) $(CFLAGSX86) oyamauobject.c -o OyamaUObject.run

SPARC:
# A Fecth&Multiply, a stack and a queue object based on P-Sim universal construction
	$(CC) $(CFLAGSSPARC) sim.c -o Sim.run
	$(CC) $(CFLAGSSPARC) simstack.c -o SimStack.run
	$(CC) $(CFLAGSSPARC) simqueue.c -o SimQueue.run
# A lock-free Fecth&Multiply, a lock-free stack and a lock-free queue object
	$(CC) $(CFLAGSSPARC) lfobject.c -o LFobject.run
	$(CC) $(CFLAGSSPARC) lfstack.c -o LFStack.run
	$(CC) $(CFLAGSSPARC) msqueue.c -o MSQueue.run
# A Fecth&Multiply, a stack and a queue object implemented with high performance spinlocks
	$(CC) $(CFLAGSSPARC) luobject.c -o LUobject.run
	$(CC) $(CFLAGSSPARC) locksstack.c -o LocksStack.run
	$(CC) $(CFLAGSSPARC) locksqueue.c -o LocksQueue.run
# A Fecth&Multiply, a stack and a queue object implemented with Oyama, et al method
	$(CC) $(CFLAGSSPARC) oyamauobject.c -o OyamaUObject.run

clean:
	rm -f *.o *.run a.out res.txt

