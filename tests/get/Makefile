LATTE_C_WORKSPACE?=$(CURDIR)/../../deps/latte_c
LATTE_DISKDB_WORKSPACE?=$(CURDIR)/../..
include $(LATTE_C_WORKSPACE)/mks/latte.mk
FINAL_CC_LIBS=-I$(LATTE_C_WORKSPACE)/src/ -I$(LATTE_DISKDB_WORKSPACE)/src/

get_test: get_test.o 
	$(LATTE_CC) $(DEBUG) -o get_test get_test.o     $(LATTE_DISKDB_WORKSPACE)/out/lib/liblatte_diskdb.a $(LATTE_DISKDB_WORKSPACE)/out/lib/liblatte.a $(LATTE_DISKDB_WORKSPACE)/out/lib/libjemalloc.a -lm  -ldl -lz -lstdc++ -lpthread
	
test: get_test
	./get_test