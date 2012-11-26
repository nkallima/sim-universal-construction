CC = gcc
CFLAGSSPARC = -pipe -O3 -mtune=niagara2 -m32 -lrt -finline-functions -lpthread -DN_THREADS=128 -DUSE_CPUS=128 -D_GNU_SOURCE
CFLAGSX86 = -pipe -O3 -ftree-vectorize -ftree-vectorizer-verbose=0 -msse3 -march=native -mtune=native -finline-functions -lpthread -DN_THREADS=16 -DUSE_CPUS=16 -D_GNU_SOURCE

X86:
# A Fecth&Multiply, a stack and a queue object implemented with Oyama, et al method
	$(CC) $(CFLAGSX86) oyamauobject.c -o OyamaUObject.run
# A Fecth&Multiply, a stack and a queue object based on P-Sim universal construction
	$(CC) $(CFLAGSX86) sim.c -o Sim.run
	$(CC) $(CFLAGSX86) simstack.c -o SimStack.run
	$(CC) $(CFLAGSX86) simqueue.c -o SimQueue.run
# A Fecth&Multiply, a stack and a queue object based on cc-BSim universal construction
	$(CC) $(CFLAGSX86) ccuobject.c -o ccUObject.run
	$(CC) $(CFLAGSX86) ccstack.c -o ccStack.run
	$(CC) $(CFLAGSX86) ccqueue.c -o ccQueue.run
# A Fecth&Multiply, a stack and a queue object based on dsm-BSim universal construction
	$(CC) $(CFLAGSX86) dsmuobject.c -o dsmUObject.run
	$(CC) $(CFLAGSX86) dsmstack.c -o dsmStack.run
	$(CC) $(CFLAGSX86) dsmqueue.c -o dsmQueue.run
# A lock-free Fecth&Multiply, a lock-free stack and a lock-free queue object 
	$(CC) $(CFLAGSX86) lfobject.c -o LFobject.run
	$(CC) $(CFLAGSX86) lfstack.c -o LFStack.run
	$(CC) $(CFLAGSX86) msqueue.c -o MSQueue.run
# A Fecth&Multiply, a stack and a queue object implemented with high performance spinlocks
	$(CC) $(CFLAGSX86) locksuobject.c -o LocksUobject.run
	$(CC) $(CFLAGSX86) locksstack.c -o LocksStack.run
	$(CC) $(CFLAGSX86) locksqueue.c -o LocksQueue.run
# Hierarchical universal constructions
	$(CC) $(CFLAGSX86) huobject.c -o hUObject.run
	$(CC) $(CFLAGSX86) hstack.c -o hStack.run
	$(CC) $(CFLAGSX86) hqueue.c -o hQueue.run

SPARC:
# A Fecth&Multiply, a stack and a queue object implemented with Oyama, et al method
	$(CC) $(CFLAGSSPARC) oyamauobject.c -o OyamaUObject.run
# A Fecth&Multiply, a stack and a queue object based on P-Sim universal construction
	$(CC) $(CFLAGSSPARC) sim.c -o Sim.run
	$(CC) $(CFLAGSSPARC) simstack.c -o SimStack.run
	$(CC) $(CFLAGSSPARC) simqueue.c -o SimQueue.run
# A Fecth&Multiply, a stack and a queue object based on cc-BSim universal construction
	$(CC) $(CFLAGSSPARC) ccuobject.c -o ccUObject.run
	$(CC) $(CFLAGSSPARC) ccstack.c -o ccStack.run
	$(CC) $(CFLAGSSPARC) ccqueue.c -o ccQueue.run
# A Fecth&Multiply, a stack and a queue object based on dsm-BSim universal construction
	$(CC) $(CFLAGSSPARC) dsmuobject.c -o dsmUObject.run
	$(CC) $(CFLAGSSPARC) dsmstack.c -o dsmStack.run
	$(CC) $(CFLAGSSPARC) dsmqueue.c -o dsmQueue.run
# A lock-free Fecth&Multiply, a lock-free stack and a lock-free queue object
	$(CC) $(CFLAGSSPARC) lfobject.c -o LFobject.run
	$(CC) $(CFLAGSSPARC) lfstack.c -o LFStack.run
	$(CC) $(CFLAGSSPARC) msqueue.c -o MSQueue.run
# A Fecth&Multiply, a stack and a queue object implemented with high performance spinlocks
	$(CC) $(CFLAGSSPARC) locksuobject.c -o LocksUobject.run
	$(CC) $(CFLAGSSPARC) locksstack.c -o LocksStack.run
	$(CC) $(CFLAGSSPARC) locksqueue.c -o LocksQueue.run
# Hierarchical universal constructions
	$(CC) $(CFLAGSSPARC) huobject.c -o hUObject.run
	$(CC) $(CFLAGSSPARC) hstack.c -o hStack.run
	$(CC) $(CFLAGSSPARC) hqueue.c -o hQueue.run

	
clean:
	rm -f *.o *.run *~ a.out res.txt

