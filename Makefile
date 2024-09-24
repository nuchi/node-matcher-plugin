BUILD_TYPE ?= Release
BUILD_DIR := build

NPROC ?= 3

.PHONY: all clean rebuild

all: $(BUILD_DIR)
	@echo "Building..."
	@cd $(BUILD_DIR) && cmake --build . --config $(BUILD_TYPE) -j $(NPROC)

$(BUILD_DIR):
	@echo "Configuring..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)

rebuild: clean all
