CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
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

# ---------------- schedsim (current master layout) ----------------

schedsim: $(SCHEDSIM)

$(SCHEDSIM): schedsim/schedsim.c schedsim/schedsim.h | $(BUILD)
	$(CC) $(CFLAGS) schedsim/schedsim.c -o $(SCHEDSIM) $(LDFLAGS)

# ---------------- workload generator ----------------

workload_gen: $(WORKLOAD_GEN)

$(WORKLOAD_GEN): tools/workload_gen.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# ---------------- driver ----------------

driver: $(DRIVER)

# placeholder until driver exists
$(DRIVER): | $(BUILD)
	@echo "driver not implemented yet"

clean:
	rm -rf $(BUILD)

