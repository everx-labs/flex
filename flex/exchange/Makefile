ifeq (, $(shell clang -v 2>&1 | grep "TON Labs"))
 $(error "Wrong clang version in $(PATH)")
endif

RED="\033[31m"
CLEAR="\033[0m"
SHELL := /bin/bash
SUFFIXES += .tvc
SUFFIXES += .d
INCLUDES += -I ./ -I ../immutable -I ../ui -I ../tokens $(file < ../tokens/FlexWallet.hashes)
CXXFLAGS += -Wall -Wextra -Wno-invalid-noreturn $(DEPFLAGS) $(INCLUDES)

SRCS:=$(shell find ./ -name "*.cpp")
TVCS := $(SRCS:%.cpp=%.tvc)
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

.PHONY:rebuild clean tvcs all check_size

all : | tvcs check_size
tvcs : $(TVCS)

rebuild:
	$(MAKE) clean
	$(MAKE) all

Flex.tvc : Flex.cpp ../tokens/FlexWallet.hashes $(DEPDIR)/Flex.d | $(DEPDIR)
	clang $(CXXFLAGS) -o $@ $<

XchgPair.tvc : XchgPair.cpp ../tokens/FlexWallet.hashes $(DEPDIR)/Flex.d | $(DEPDIR)
	clang $(CXXFLAGS) -o $@ $<

PriceXchg.tvc : PriceXchg.cpp ../tokens/FlexWallet.hashes $(DEPDIR)/Flex.d | $(DEPDIR)
	clang $(CXXFLAGS) -mtvm-refunc -o $@ $<

$(DEPDIR): ; @mkdir -p $@
DEPFILES := $(SRCS:%.cpp=$(DEPDIR)/%.d)

.PRECIOUS: $(DEPDIR)/%.d
$(DEPDIR)/%.d: ;

check_size: tvcs
	@ls -l *.tvc | awk '{ if ($$5 > 16000) { print ${RED} "CONTRACT BIG SIZE" ${CLEAR} " | " $$5 " | " $$9; err = 1; } } END {exit err}'

-include $(DEPFILES)

clean:
	rm -f *.abi
	rm -f *.tvc
	rm -f *.hashes

