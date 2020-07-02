CC=gcc
ODIR=build
IDIR=src
CFLAGS=-shared -fPIC -lpthread -fno-omit-frame-pointer -g
LDFLAGS=-shared
_DEPS=chunk.h arena.h cpu_monitor.h ptlbmalloc2.h types.h global.h
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))
_OBJ=chunk.o arena.o cpu_monitor.o ptlbmalloc2.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o:  $(IDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)


bin/libmem.a: $(OBJ)
	ar -rc $@ $^ 
