ifeq (, $(shell clang -v 2>&1 | grep "TON Labs"))
 $(error "Wrong clang version in $(PATH)")
endif

RED="\033[31m"
CLEAR="\033[0m"
SHELL := /bin/bash
SUFFIXES += .tvc
SUFFIXES += .d
DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d
INCLUDES += -I ./ -I ../ -I ../ui -I ../tokens -I ../exchange
CXXFLAGS=-Wall -Wextra -Wno-invalid-noreturn $(DEPFLAGS) $(INCLUDES)

SRCS:=$(shell find ./ -name "*.cpp")
TVCS := $(SRCS:%.cpp=%.tvc)

.PHONY:rebuild clean tvcs all check_size

all : | tvcs check_size
tvcs : $(TVCS)

rebuild:
	$(MAKE) clean
	$(MAKE) all

%.tvc : %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	clang $(CXXFLAGS) -o $@ $<

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

