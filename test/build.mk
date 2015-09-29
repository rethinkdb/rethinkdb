
.PHONY: test-deps
test-deps: $(BUILD_DIR)/rethinkdb $(BUILD_DIR)/rethinkdb-unittest web-assets rb-driver py-driver

.PHONY: test
test: test-deps
	$P RUN-TESTS $(TEST)
	MAKEFLAGS= $/test/run $(RUN_TEST_ARGS) -b $(BUILD_DIR) $(TEST)

.PHONY: full-test
full-test: TEST = all
full-test: test
