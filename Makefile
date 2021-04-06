ARCH     = $(shell uname -p)
NPROC    = $(shell nproc)
LIBS     = $(shell ./libconcurrent/libdeps.sh) -latomic
CLANG_AR = ar

all:
	make $(ARCH)

debug:
	make $(ARCH) D_ARGS="-DDEBUG"

x86_64:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) LDLIBS='$(LIBS)' CFLAGS='-Wall -std=gnu89 -Ofast -fPIC -flto -ftree-vectorize -finline-functions -march=native -mtune=native'

icx:
	make -f Makefile.generic -j $(NPROC) COMPILER=icx LDLIBS='$(LIBS)' CFLAGS='-Wall -std=gnu89 -O3 -fPIC -ftree-vectorize -finline-functions -march=native -mtune=native'

clang:
	make -f Makefile.generic -j $(NPROC) COMPILER=clang AR=$(CLANG_AR) LDLIBS='$(LIBS)' CFLAGS='-Wall -std=gnu89 -Ofast -fPIC -ftree-vectorize -finline-functions -march=native -mtune=native'

unknown:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) LDLIBS='$(LIBS)'  CFLAGS='-std=gnu89 -Ofast -fPIC -flto'

clean:
	make -f Makefile.generic clean -j $(NPROC)
