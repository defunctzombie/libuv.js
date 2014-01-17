-include config.mk

BUILDTYPE ?= Release
PYTHON ?= python
DESTDIR ?=
SIGN ?=
PREFIX ?= /usr/local

# Default to verbose builds.
# To do quiet/pretty builds, run `make V=` to set V to an empty string,
# or set the V environment variable to an empty string.
V ?= 1

all: out/Makefile
	$(MAKE) -C out BUILDTYPE=$(BUILDTYPE) V=$(V)

test: all
	./out/$(BUILDTYPE)/uvjs --bootstrap test/support/bootstrap.js --expose-gc test/index.js

out/Makefile: common.gypi vendor/uv/uv.gyp vendor/v8/build/toolchain.gypi vendor/v8/build/features.gypi vendor/v8/tools/gyp/v8.gyp config.gypi uvjs.gyp
	$(PYTHON) tools/run_gyp.py -f make

config.gypi: configure
	if [ -f $@ ]; then
		$(error Stale $@, please re-run ./configure)
	else
		$(error No $@, please run ./configure first)
	fi

clean:
	-rm -rf out/Makefile out/$(BUILDTYPE)/uvjs out/$(BUILDTYPE)/libuvjs
	-find out/ -name '*.o' -o -name '*.a' | xargs rm -rf

.PHONY: clean test
