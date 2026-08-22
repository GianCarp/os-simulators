CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 
LDFLAGS :=

BUILD := build
MEMSIM_TEST_BUILD := $(BUILD)/tests/memsim
SCHED_TEST_BUILD := $(BUILD)/tests/schedsim

MEMSIM := $(BUILD)/memsim
SCHEDSIM := $(BUILD)/schedsim
WORKLOAD_GEN := $(BUILD)/workload_gen
DRIVER := $(BUILD)/driver

MEMSIM_UNIT_TESTS := \
	$(MEMSIM_TEST_BUILD)/test_lru_dll \
	$(MEMSIM_TEST_BUILD)/test_mmu_lifecycle \
	$(MEMSIM_TEST_BUILD)/test_page_operations \
	$(MEMSIM_TEST_BUILD)/test_replace_page

SCHED_UNIT_TESTS :=

.PHONY: all clean memsim schedsim workload_gen driver test test-unit test-integration

all: memsim schedsim workload_gen driver

$(BUILD):
	mkdir -p $(BUILD)

$(MEMSIM_TEST_BUILD):
	mkdir -p $(MEMSIM_TEST_BUILD)

$(SCHED_TEST_BUILD):
	mkdir -p $(SCHED_TEST_BUILD)

memsim: $(MEMSIM)
$(MEMSIM): memsim/main.c memsim/memsim.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $(MEMSIM) $(LDFLAGS)

schedsim: $(SCHEDSIM)
$(SCHEDSIM): \
	schedsim/src/main.c \
	schedsim/src/schedsim.c \
	schedsim/src/policies.c \
	schedsim/src/queue.c \
	schedsim/src/args.c | $(BUILD)
	$(CC) $(CFLAGS) -Ischedsim/include $^ -o $(SCHEDSIM) $(LDFLAGS)

workload_gen: $(WORKLOAD_GEN)
$(WORKLOAD_GEN): tools/workload_gen.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

driver: $(DRIVER)
$(DRIVER): | $(BUILD)
	@echo "Unified memory and scheduling simulator not implemented yet"

# tests

$(MEMSIM_TEST_BUILD)/test_%: tests/memsim/unit/test_%.c memsim/memsim.c memsim/memsim.h | $(MEMSIM_TEST_BUILD)
	$(CC) $(CFLAGS) -Imemsim $< -o $@ -lcmocka

# schedsim is split across several translation units, so unlike the memsim rule
# the sources have to be named rather than relying on $<. schedsim.c is a
# prerequisite but is deliberately kept off the command line: a test that needs
# its static functions includes it textually, which would clash with a linked
# copy.
$(SCHED_TEST_BUILD)/test_%: tests/schedsim/unit/test_%.c \
	schedsim/src/policies.c \
	schedsim/src/queue.c \
	schedsim/src/args.c \
	schedsim/src/schedsim.c | $(SCHED_TEST_BUILD)
	$(CC) $(CFLAGS) -Ischedsim/include -Ischedsim/src \
	  $< schedsim/src/policies.c schedsim/src/queue.c schedsim/src/args.c \
	  -o $@ -lcmocka

test: memsim schedsim $(MEMSIM_UNIT_TESTS) $(SCHED_UNIT_TESTS)
	@tests/run_tests.sh

test-unit: $(MEMSIM_UNIT_TESTS) $(SCHED_UNIT_TESTS)
	@for t in $(MEMSIM_UNIT_TESTS) $(SCHED_UNIT_TESTS); do $$t; done

test-integration: memsim schedsim
	@tests/memsim/integration/test_*.sh
	@tests/schedsim/integration/test_*.sh

clean:
	rm -rf $(BUILD)
