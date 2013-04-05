
TEST_FILTER := default
RUN_TEST_ARGS := -d /dev/shm

$/bench/stress-client/stress:
	$P MAKE -C $/bench/stress-client
	$(EXTERN_MAKE) -C $/bench/stress-client

.PHONY: test
test: $/build/debug/rethinkdb $/build/debug/rethinkdb-unittest $/bench/stress-client/stress
	$P RUN-TESTS $(TEST_FILTER)
	$/scripts/run-tests.sh -f $(TEST_FILTER) $(RUN_TEST_ARGS)
