ARCH := $(shell uname -p)
NPROC := $(shell nproc)

all:
	make $(ARCH)

x86_64:
	make -f Makefile.x86 -j $(NPROC)

x86:
	make -f Makefile.x86 -j $(NPROC)

sparc:
	make -f Makefile.sparc -j $(NPROC)

icc:
	make -f Makefile.icc -j $(NPROC)

clang:
	make -f Makefile.clang -j $(NPROC)

clean:
	make -f Makefile.x86 clean -j $(NPROC)
