# Copyright 2014 RethinkDB, all rights reserved.

PROTOBUF_TEST_SRC_DIR = $(TOP)/test/protobuf_test
PROTOBUF_TEST_INC_FILES = $(wildcard $(PROTOBUF_TEST_SRC_DIR)/*.hpp)
PROTOBUF_TEST_BUILD_DIR = $(BUILD_DIR)/tests/protobuf_test
PROTOBUF_TEST = $(PROTOBUF_TEST_BUILD_DIR)/protobuf_test

PROTOBUF_TEST_OBJ_FILES := $(PROTOBUF_TEST_BUILD_DIR)/rethink-db-cpp-driver/rql.o $(PROTOBUF_TEST_BUILD_DIR)/rethink-db-cpp-driver/connection.o $(PROTOBUF_TEST_BUILD_DIR)/main.o $(BUILD_DIR)/obj/rdb_protocol/ql2.pb.o

$(PROTOBUF_TEST_BUILD_DIR)/%.o: $(PROTOBUF_TEST_SRC_DIR)/%.cpp $(PROTOBUF_INCLUDE_DEP) $(BOOST_INCLUDE_DEP) $(BUILD_DIR)/proto/rdb_protocol/ql2.pb.h $(PROTOBUF_TEST_INC_FILES)
	$P CC
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -std=gnu++0x -c $< -I$(PROTOBUF_TEST_SRC_DIR)/rethink-db-cpp-driver $(PROTOBUF_INCLUDE) $(BOOST_INCLUDE) -I$(BUILD_DIR) -o $@

$(PROTOBUF_TEST): $(PROTOBUF_TEST_OBJ_FILES) $(PROTOBUF_LIBS_DEP) $(BOOST_SYSTEM_LIBS_DEP)
	$P LD
	$(CXX) $(LDFLAGS) -std=gnu++0x -o $(PROTOBUF_TEST) $(PROTOBUF_TEST_OBJ_FILES) $(PROTOBUF_LIBS) $(PTHREAD_LIBS) $(BOOST_SYSTEM_LIBS)

.PHONY: protobuf-test-make
protobuf-test-make: $(PROTOBUF_TEST)

.PHONY: protobuf-test-clean
protobuf-test-clean:
	$P RM $(PROTOBUF_TEST_BUILD_DIR)
	rm -rf $(PROTOBUF_TEST_BUILD_DIR)

.PHONY: protobuf-test-output-location
protobuf-test-output-location: $(PROTOBUF_TEST)
	 $(info protobuff-test executable location: $(realpath $(PROTOBUF_TEST)))
