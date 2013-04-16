BUILD=$(shell pwd)/build
BUILD_STAMP=$(BUILD)/.dirstamp

INSTALL_LOC=$(BUILD)/usr

LIB_UNWIND_VERSION=libunwind-0.99-beta
LIB_UNWIND=$(INSTALL_LOC)/lib/libunwind-x86_64.a

LIB_UNWIND_DOWNLOAD=$(LIB_UNWIND_VERSION).tar.gz
LIB_UNWIND_TAR=$(BUILD)/$(LIB_UNWIND_DOWNLOAD)
LIB_UNWIND_SRC=$(BUILD)/$(LIB_UNWIND_VERSION)
LIB_UNWIND_SRC_STAMP=$(LIB_UNWIND_SRC)/.dirstamp
LIB_UNWIND_MAKE=$(LIB_UNWIND_SRC)/Makefile

GPERF_VERSION=gperftools-2.0
GPERF=$(INSTALL_LOC)/lib/libtcmalloc_and_profiler.a

GPERF_DOWNLOAD=$(GPERF_VERSION).tar.gz
GPERF_TAR=$(BUILD)/$(GPERF_DOWNLOAD)
GPERF_SRC=$(BUILD)/$(GPERF_VERSION)
GPERF_SRC_STAMP=$(GPERF_SRC)/.dirstamp
GPERF_MAKE=$(GPERF_SRC)/Makefile

build_with_hprof: $(GPERF)
	cd ../ && ./configure tcmalloc_minimal=$(GPERF) LDFLAGS="-L$(INSTALL_LOC)/lib/ -lunwind"
	make -C ../ DEBUG=1

$(GPERF): $(LIB_UNWIND) $(GPERF_MAKE)
	make install -C $(GPERF_SRC) LDFLAGS="-L$(INSTALL_LOC)/lib -lunwind-x86_64"

$(GPERF_MAKE): $(GPERF_SRC_STAMP)
	cd $(GPERF_SRC) && ./configure \
        --prefix $(INSTALL_LOC)

$(GPERF_SRC_STAMP): $(GPERF_DOWNLOAD)
	tar -xf $(GPERF_TAR) --directory=$(BUILD) && touch $(GPERF_SRC_STAMP)

$(GPERF_DOWNLOAD):
	wget -O $(GPERF_TAR) http://gperftools.googlecode.com/files/$(GPERF_DOWNLOAD)


# libunwind targets

$(LIB_UNWIND): $(LIB_UNWIND_MAKE)
	make -C $(LIB_UNWIND_SRC) install

$(LIB_UNWIND_MAKE): $(LIB_UNWIND_SRC_STAMP)
	cd $(LIB_UNWIND_SRC) && ./configure CFLAGS=-U_FORTIFY_SOURCE --prefix $(INSTALL_LOC)

$(LIB_UNWIND_SRC_STAMP): $(LIB_UNWIND_DOWNLOAD)
	tar -xf $(LIB_UNWIND_TAR) --directory=$(BUILD) && touch $(LIB_UNWIND_SRC_STAMP)

$(LIB_UNWIND_DOWNLOAD): $(BUILD_STAMP)
	wget -O $(LIB_UNWIND_TAR) http://download.savannah.gnu.org/releases/libunwind/$(LIB_UNWIND_DOWNLOAD)

$(BUILD_STAMP):
	mkdir $(BUILD) && touch $(BUILD_STAMP)

clean:
	rm -rf $(BUILD)

.PHONY: clean build_with_hprof
