
LIB_MODUELS="sds dict ae config connection list endianconv rax server mutex fs utils dict_plugins set"

uname_S:= $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M:= $(shell sh -c 'uname -s 2>/dev/null || echo not')

OPTIMIZATION?=-O0
STD=-std=c99 -pedantic -DREDIS_STATIC=''
WARN=-Wall -W -Wno-missing-field-initializers
OPT=$(OPTIMIZATION)
DEBUG=-g -ggdb
FINAL_CFLAGS=$(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS) $(LATTE_REDIS_CFLAGS)
LATTE_DISKDB_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
ifeq ($(uname_S),Linux)
	FINAL_SHARD_LIB=-lrt 
endif

LATTE_C_WORKSPACE=$(CURDIR)/../deps/latte_c

$(LATTE_C_WORKSPACE)/out/lib/liblatte.a:
	cd ../deps/latte_c && make build MODULES=$(LIB_MODUELS) 

%.o: %.c 
	$(LATTE_DISKDB_CC) -o $@ -c $< -I../deps/latte_c/out/include 

MAC_M1_CFLAGS=-fprofile-arcs -ftest-coverage
OBJS= db.o read.o  write.o comparator.o
# -lstdc++
all: $(LATTE_C_WORKSPACE)/out/lib/liblatte.a $(OBJS) 
	mkdir -p ../out/lib &&\
	cp $(LATTE_C_WORKSPACE)/out/lib/liblatte.a ../out/lib/ &&\
	cp $(LATTE_C_WORKSPACE)/out/lib/libjemalloc.a ../out/lib/ &&\
	$(AR) $(ARFLAGS) ../out/lib/liblatte_diskdb.a  $(OBJS) 

clean:
	cd ../deps/latte_c && make clean_all 
	rm -rf *.o 
	rm -rf main

test: 
	@(cd ..; ./runtest )