ifeq (, $(shell clang -v 2>&1 | grep "TON Labs"))
 $(error "Wrong clang version in $(PATH)")
endif

RED="\033[31m"
CLEAR="\033[0m"
SHELL := /bin/bash
SUFFIXES += .tvc
SUFFIXES += .d
INCLUDES += -I ../tokens -I ../ -I ../immutable -I ../exchange $(file < ../tokens/FlexWallet.hashes)
DEFINES  += -DTIP3_ENABLE_EXTERNAL -DTIP3_ENABLE_LEND_OWNERSHIP -DTIP3_ENABLE_BURN -DTIP3_ONLY_FLEX_EXTERNAL
CXXFLAGS += -mtvm-refunc -Wall -Wextra -Wno-invalid-noreturn $(DEPFLAGS) $(INCLUDES) $(DEFINES)

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

%.tvc : %.cpp ../tokens/FlexWallet.hashes $(DEPDIR)/%.d | $(DEPDIR)
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

