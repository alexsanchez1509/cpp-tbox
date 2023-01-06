include build_env.mk

.PHONY: all modules examples test clean distclean print

CCFLAGS := -Wall

RELEASE ?= 0
ENABLE_ASAN ?= 1
ENABLE_GPROF ?= 0

ifeq ($(RELEASE), 1)
CCFLAGS += -O2 -Os
else
CCFLAGS += -DDEBUG=1 -O0 -ggdb
ifeq ($(ENABLE_ASAN), 1)
CCFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS += -fsanitize=address -static-libasan
endif
ifeq ($(ENABLE_GPROF), 1)
CCFLAGS += -pg
LDFLAGS += -pg
endif
endif

CXXFLAGS := $(CCFLAGS) $(CXXFLAGS)
CFLAGS := $(CCFLAGS) $(CFLAGS)
APPS_DIR := $(PWD)

export CC CXX CFLAGS CXXFLAGS LDFLAGS APPS_DIR
export MODULES

include config.mk

all: modules test examples

print:
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"

modules:
	$(MAKE) -C $@

test: modules
	$(MAKE) -C modules test

examples: modules
	$(MAKE) -C $@

clean:
	-rm -rf $(BUILD_DIR)

distclean: clean
	-rm -rf $(STAGING_DIR) $(INSTALL_DIR)
