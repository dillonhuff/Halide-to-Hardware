# all directories (just text) excluding a particular list
ALL_APPS := $(subst /,, $(dir $(wildcard */.)))
ALL_APPS := $(filter-out hls_support tools, $(ALL_APPS))

all: testall

.PHONY: $(ALL_APPS)
$(ALL_APPS):
	@$(MAKE) -sC $@ test

testall test_all: runall
# @echo ""
# @for app in $(ALL_APPS); do \
#   if [ -f $$app"/design_prepass.json" ]; then \
#     printf "%-15s \033[0;32m%s\033[0m\n" $$app "PASSED"; \
#   else \
#     printf "%-15s \033[0;31m%s\033[0m\n" $$app "FAILED"; \
#   fi \
# done

testtravis test_travis: pointwise


runall run_all:
	@for app in $(ALL_APPS); do \
	  echo -n "$$app: "; \
	  $(MAKE) -sC $$app run; \
	done

evalall eval_all:
	@for app in $(ALL_APPS); do \
	  echo -n "$$app: "; \
	  $(MAKE) -sC $$app eval; \
	done

updategoldens update_goldens goldens:
	@echo ""
	@for app in $(ALL_APPS); do \
	  echo -n "$$app: "; \
	  $(MAKE) -sC $$app update_golden; \
	  echo "updated golden files for $$app"; \
	done

checkall check_all:
	@echo ""
	@for app in $(ALL_APPS); do \
	  $(MAKE) -sC $$app check; \
	done

clean_all cleanall:
	for app in $(ALL_APPS); do \
	  $(MAKE) -C $$app clean > /dev/null; \
	done

