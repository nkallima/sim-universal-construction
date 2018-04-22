ARCH  = $(shell uname -p)
NPROC = $(shell nproc)

all:
	make $(ARCH)

x86_64:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) CFLAGS='-Wall -std=gnu89 -Wno-unused-but-set-variable -Ofast -fPIC -flto -msse3 -ftree-vectorize -finline-functions -march=native -mtune=native -D_GNU_SOURCE'

x86:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) CFLAGS='-Wall -std=gnu89 -Wno-unused-but-set-variable -Ofast -fPIC -flto -msse3 -ftree-vectorize -finline-functions -march=native -mtune=native -D_GNU_SOURCE'

sparc:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) CFLAGS='-pipe -O3 -mtune=niagara2 -m32 -lrt -finline-functions -D_GNU_SOURCE'

icc:
	make -f Makefile.generic -j $(NPROC) COMPILER=icc CFLAGS='-static -gcc -Ofast -pthread -D_GNU_SOURCE'

unknown:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) CFLAGS='-std=gnu89 -Ofast -fPIC -flto -D_GNU_SOURCE'

clean:
	make -f Makefile.generic clean -j $(NPROC)
