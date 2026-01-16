CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
LDFLAGS :=

BUILD := build
MEMSIM := $(BUILD)/memsim
SCHEDSIM := $(BUILD)/schedsim
DRIVER := $(BUILD)/driver

.PHONY: all clean memsim schedsim driver

all: memsim schedsim driver

$(BUILD):
	mkdir -p $(BUILD)

memsim: $(MEMSIM)

$(MEMSIM): memsim/memsim.c memsim/memsim.h | $(BUILD)
	$(CC) $(CFLAGS) memsim/memsim.c -o $(MEMSIM) $(LDFLAGS)

schedsim: $(SCHEDSIM)

# placeholder until schedsim exists
$(SCHEDSIM): | $(BUILD)
	@echo "schedsim not implemented yet"

driver: $(DRIVER)

# placeholder until driver exists
$(DRIVER): | $(BUILD)
	@echo "driver not implemented yet"

clean:
	rm -rf $(BUILD)
