LATTE_C_WORKSPACE?=$(CURDIR)/../../deps/latte_c
LATTE_DISKDB_WORKSPACE?=$(CURDIR)/../..
include $(LATTE_C_WORKSPACE)/mks/latte.mk
FINAL_CC_LIBS=-I$(LATTE_C_WORKSPACE)/src/ -I$(LATTE_DISKDB_WORKSPACE)/src/

logReader_test: logReader_test.o 
	$(LATTE_CC) $(DEBUG) -o logReader_test logReader_test.o     $(LATTE_DISKDB_WORKSPACE)/out/lib/liblatte_diskdb.a $(LATTE_DISKDB_WORKSPACE)/out/lib/liblatte.a $(LATTE_DISKDB_WORKSPACE)/out/lib/libjemalloc.a -lm  -ldl -lz -lstdc++ -lpthread
	
test: logReader_test
	./logReader_test