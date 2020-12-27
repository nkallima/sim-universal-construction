ARCH  = $(shell uname -p)
NPROC = $(shell nproc)
LIBS  = $(shell ./libconcurrent/libdeps.sh)

all:
	make $(ARCH)

x86_64:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) LDLIBS='$(LIBS)' CFLAGS='-Wall -std=gnu89 -Ofast -fPIC -flto -ftree-vectorize -finline-functions -march=native -mtune=native -D_GNU_SOURCE'

icc:
	make -f Makefile.generic -j $(NPROC) COMPILER=icc LDLIBS='$(LIBS)' CFLAGS='-static -gcc -Ofast -pthread -D_GNU_SOURCE'

unknown:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) LDLIBS='$(LIBS)'  CFLAGS='-std=gnu89 -Ofast -fPIC -flto -D_GNU_SOURCE'

clean:
	make -f Makefile.generic clean -j $(NPROC)
