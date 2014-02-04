#
# Makefile for OpenzWave Mac OS X applications
# Greg Satz

# GNU make only

# requires libudev-dev

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean install


top_srcdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
top_builddir ?= $(CURDIR)
export top_builddir
PREFIX ?= /usr/local
export PREFIX

all: 
	$(MAKE) -C $(top_srcdir)/cpp/build/ -$(MAKEFLAGS) 
	$(MAKE) -C $(top_srcdir)/cpp/examples/MinOZW/ -$(MAKEFLAGS) 

install:
	$(MAKE) -C $(top_srcdir)/cpp/build/ -$(MAKEFLAGS) $(MAKECMDGOALS)
	$(MAKE) -C $(top_srcdir)/cpp/examples/MinOZW/ -$(MAKEFLAGS) $(MAKECMDGOALS)

clean:
	$(MAKE) -C $(top_srcdir)/cpp/build/ -$(MAKEFLAGS) $(MAKECMDGOALS)
	$(MAKE) -C $(top_srcdir)/cpp/examples/MinOZW/ -$(MAKEFLAGS) $(MAKECMDGOALS)

cpp/src/vers.cpp:
	$(MAKE) -C $(top_srcdir)/cpp/build/ -$(MAKEFLAGS) cpp/src/vers.cpp

check:

include $(top_srcdir)/cpp/build/support.mk

dist-update:
	@echo "Updating List of Distribition Files"
	@$(SVN) --xml -v st > .distfiles
	@$(top_srcdir)/makedist

DIST_FORMATS ?= gzip

include $(top_srcdir)/distfiles.mk
include $(top_srcdir)/dist.mk