CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Ischedsim/include
LDFLAGS :=

BUILD := build
MEMSIM := $(BUILD)/memsim
SCHEDSIM := $(BUILD)/schedsim
WORKLOAD_GEN := $(BUILD)/workload_gen
DRIVER := $(BUILD)/driver

.PHONY: all clean memsim schedsim workload_gen driver

all: memsim schedsim workload_gen driver

$(BUILD):
	mkdir -p $(BUILD)

# ---------------- memsim ----------------

memsim: $(MEMSIM)

$(MEMSIM): memsim/memsim.c memsim/memsim.h | $(BUILD)
	$(CC) $(CFLAGS) memsim/memsim.c -o $(MEMSIM) $(LDFLAGS)

# ---------------- schedsim (refactored layout) ----------------

schedsim: $(SCHEDSIM)

$(SCHEDSIM): \
	schedsim/src/schedsim.c \
	schedsim/src/policies.c \
	schedsim/src/queue.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $(SCHEDSIM) $(LDFLAGS)

# ---------------- workload generator ----------------

workload_gen: $(WORKLOAD_GEN)

$(WORKLOAD_GEN): tools/workload_gen.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ---------------- driver ----------------

driver: $(DRIVER)

# placeholder until driver exists
$(DRIVER): | $(BUILD)
	@echo "Unified memory and scheduling simulator not implemented yet"

clean:
	rm -rf $(BUILD)

