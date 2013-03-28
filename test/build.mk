
TEST_FILTER := '*'

.PHONY: test
test: $/build/debug/rethinkdb $/build/debug/rethnkdb-unittest $/bench/stress-client/stress
	$/scripts/run-tests.sh -f $(TEST_FILTER)
