default: all

.DEFAULT:
	cd src && $(MAKE) $@

install:
	cd src && $(MAKE) $@

lib_test:
	cd tests && $(MAKE) all

.PHONY: install