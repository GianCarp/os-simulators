CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 
LDFLAGS :=

BUILD := build
TEST_BUILD := $(BUILD)/tests/memsim

MEMSIM := $(BUILD)/memsim
SCHEDSIM := $(BUILD)/schedsim
WORKLOAD_GEN := $(BUILD)/workload_gen
DRIVER := $(BUILD)/driver

UNIT_TESTS := \
	$(TEST_BUILD)/test_lru_dll \
	$(TEST_BUILD)/test_mmu_lifecycle \
	$(TEST_BUILD)/test_page_operations \
	$(TEST_BUILD)/test_replace_page

.PHONY: all clean memsim schedsim workload_gen driver test test-unit test-integration

all: memsim schedsim workload_gen driver

$(BUILD):
	mkdir -p $(BUILD)

$(TEST_BUILD):
	mkdir -p $(TEST_BUILD)

memsim: $(MEMSIM)
$(MEMSIM): memsim/main.c memsim/memsim.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $(MEMSIM) $(LDFLAGS)

schedsim: $(SCHEDSIM)
$(SCHEDSIM): \
	schedsim/src/main.c \
	schedsim/src/schedsim.c \
	schedsim/src/policies.c \
	schedsim/src/queue.c | $(BUILD)
	$(CC) $(CFLAGS) -Ischedsim/include $^ -o $(SCHEDSIM) $(LDFLAGS)

workload_gen: $(WORKLOAD_GEN)
$(WORKLOAD_GEN): tools/workload_gen.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

driver: $(DRIVER)
$(DRIVER): | $(BUILD)
	@echo "Unified memory and scheduling simulator not implemented yet"

# tests

$(TEST_BUILD)/test_%: tests/memsim/unit/test_%.c memsim/memsim.c memsim/memsim.h | $(TEST_BUILD)
	$(CC) $(CFLAGS) -Imemsim $< -o $@ -lcmocka

test: memsim $(UNIT_TESTS)
	@tests/run_tests.sh

test-unit: $(UNIT_TESTS)
	@for t in $(UNIT_TESTS); do $$t; done

test-integration: memsim
	@tests/memsim/integration/test_*.sh

clean:
	rm -rf $(BUILD)
