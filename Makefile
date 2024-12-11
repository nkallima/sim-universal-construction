ARCH     = $(shell uname -p)
NPROC    = $(shell nproc)
LIBS     = $(shell ./libconcurrent/libdeps.sh)
CLANG_AR = ar
DIR      = /opt

all:
	make $(ARCH)

debug:
	make $(ARCH) D_ARGS="-DDEBUG"

codecov:
	make $(ARCH) D_ARGS="-DDEBUG -ftest-coverage -fprofile-arcs --coverage -O0"

x86_64:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) LDLIBS='$(LIBS)' CFLAGS='-Wall -std=c11 -Ofast -fPIC -flto=auto -ftree-vectorize -finline-functions -march=native -mtune=native -D_XOPEN_SOURCE=700'

icx:
	make -f Makefile.generic -j $(NPROC) COMPILER=icx LDLIBS='$(LIBS)' CFLAGS='-Wall -std=c11 -O3 -fPIC -ftree-vectorize -finline-functions -march=native -mtune=native -D_XOPEN_SOURCE=700'

clang:
	make -f Makefile.generic -j $(NPROC) COMPILER=clang AR=$(CLANG_AR) LDLIBS='$(LIBS)' CFLAGS='-Wall -std=c11 -Ofast -fPIC -ftree-vectorize -finline-functions -march=native -mtune=native -D_XOPEN_SOURCE=700'

unknown:
	make -f Makefile.generic -j $(NPROC) COMPILER=$(CC) LDLIBS='$(LIBS)'  CFLAGS='-std=c11 -Ofast -fPIC -flto=auto -D_XOPEN_SOURCE=700'

clean:
	make -f Makefile.generic clean

docs:
	make -f Makefile.generic docs

install:
	make -f Makefile.generic install INSTALL_DIR=$(DIR)

uninstall:
	make -f Makefile.generic uninstall
