ifeq (, $(shell clang -v 2>&1 | grep "TON Labs"))
 $(error "Wrong clang version in $(PATH)")
endif

all: SuperRootOwner.tvc make-immutable make-tokens make-exchange make-ui

make-immutable:
	$(MAKE) -C immutable all
make-tokens:
	$(MAKE) -C tokens all
make-exchange:
	$(MAKE) -C exchange all
make-ui:
	$(MAKE) -C ui all

rebuild-immutable:
	$(MAKE) -C immutable rebuild
rebuild-tokens:
	$(MAKE) -C tokens rebuild
rebuild-exchange:
	$(MAKE) -C exchange rebuild
rebuild-ui:
	$(MAKE) -C ui rebuild

rebuild:
	$(MAKE) rebuild-immutable
	$(MAKE) rebuild-tokens
	$(MAKE) rebuild-exchange
	$(MAKE) rebuild-ui
	$(MAKE) clean
	$(MAKE) all

SuperRootOwner.tvc: SuperRootOwner.cpp SuperRootOwner.hpp immutable/SuperRoot.hpp
	clang -o SuperRootOwner.tvc SuperRootOwner.cpp -I ./immutable -I ./ -I ./ui -Wall -I ./tokens -I ./exchange

docs: Flex-doc

clean-docs:
	rm -rf Flex.doc

Flex-doc:
	doxygen Flex.doxycfg

clean:
	rm -f *.abi
	rm -f *.tvc
	rm -f *.hashes

