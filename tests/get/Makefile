LATTE_C_WORKSPACE?=$(CURDIR)/../../deps/latte_c
include $(LATTE_C_WORKSPACE)/mks/latte.mk
FINAL_CC_LIBS=-I$(LATTE_C_WORKSPACE)/src/
test: get_test.o 
	$(LATTE_CC) $(DEBUG) -o get_test get_test.o 
