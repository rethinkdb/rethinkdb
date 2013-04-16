
$/bench/stress-client/stress:
	$P MAKE -C $/bench/stress-client
	$(EXTERN_MAKE) -C $/bench/stress-client

.PHONY: test-deps
test-deps: $(BUILD_DIR)/rethinkdb $(BUILD_DIR)/rethinkdb-unittest $/bench/stress-client/stress web-assets
#	$(EXTERN_MAKE) -C $/drivers/ruby -s
	$(EXTERN_MAKE) -C $/drivers/python -s

.PHONY: test
test: test-deps
	$P RUN-TESTS $(TEST)
	$/scripts/run-tests.sh $(RUN_TEST_ARGS) -b $(BUILD_DIR) $(TEST)

.PHONY: full-test
full-test: TEST = all
full-test: test
