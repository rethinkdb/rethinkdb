BUILD=build
SRC=src
DRIVERS=drivers

YAML_FILES=$(shell find $(SRC) -name '*.yaml')
DRIVER_FILES=$(wildcard $(DRIVERS)/*)
TEST_FILES=$(patsubst $(DRIVERS)/driver.%,$(BUILD)/test.%,$(DRIVER_FILES))

run: $(TEST_FILES)
	./test-runner run

$(TEST_FILES): $(YAML_FILES) $(DRIVER_FILES) test-runner
	mkdir -p build
	./test-runner build_test_scripts

clean:
	rm -r build

.PHONY: run clean
