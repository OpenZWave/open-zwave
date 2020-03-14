#
# Makefile for OpenzWave Mac OS X applications
# Greg Satz

# GNU make only

# requires libudev-dev

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean

# 2019-10 added this test because there are path issues, hings go wrong when you try to run
# "make" in the cpp/test subdirectory. One of the offending statements is "top_builddir ?= $(CURDIR)"
# Needs some work to get a proper fix.

ifeq ($(top_builddir),)
 $(error Variable top_builddir is undefined, please run "make" from root of OpenzWave repository only.)
endif

COMMON_FLAGS	:= -std=c++11 -Wall -Wno-unknown-pragmas -Wsign-compare
DEBUG_CFLAGS    := -ggdb -DDEBUG $(CPPFLAGS) $(COMMON_FLAGS)
RELEASE_CFLAGS  := -O3 $(CPPFLAGS) $(COMMON_FLAGS)

DEBUG_LDFLAGS	:= -g

top_srcdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../../)

#where is put the temporary library
LIBDIR  	?= $(top_builddir)

INCLUDES	:= -I $(top_srcdir)/cpp/test/ -I $(top_srcdir)/cpp/test/include/ -I $(top_srcdir)/cpp/src -I $(top_srcdir)/cpp/tinyxml/ -I $(top_srcdir)/cpp/hidapi/hidapi/
OZW_LIB = $(wildcard $(LIBDIR)/*.a )
LIBS = $(OZW_LIB)

ifneq ($(UNAME),FreeBSD)
LIBS += -lresolv
endif

#LIBSDIR = $(abspath $(dir $(firstword $(LIBS))))
SOURCES  := $(top_srcdir)/cpp/test/src/ $(top_srcdir)/cpp/test/
gtestsrc := $(notdir $(wildcard $(top_srcdir)/cpp/test/src/*.cc))
testsrc := $(notdir $(wildcard $(top_srcdir)/cpp/test/*.cpp))
VPATH := $(top_srcdir)/cpp/test/:$(top_srcdir)/cpp/test/src/

top_builddir ?= $(CURDIR)

default: $(top_builddir)/gtest-main

include $(top_srcdir)/cpp/build/support.mk

-include $(patsubst %.cc,$(DEPDIR)/%.d,$(gtestsrc))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(testsrc))

#if we are on a Mac, add these flags and libs to the compile and link phases 
ifeq ($(UNAME),Darwin)
CFLAGS += -DDARWIN
TARCH += -arch x86_64
endif

# Dup from main makefile, but that is not included when building here..
ifeq ($(UNAME),FreeBSD)
LDFLAGS+= -lusb

ifeq ($(shell test $$(uname -U) -ge 1002000; echo $$?),1)
ifeq (,$(wildcard /usr/local/include/iconv.h))
$(error FreeBSD pre 10.2: Please install libiconv from ports)
else
CFLAGS += -I/usr/local/include
LDFLAGS+= -L/usr/local/lib -liconv
endif
endif

endif

$(OBJDIR)/%.o : %.cc
	@echo "Building $(notdir $@)"
	@$(CXX) -MM $(CFLAGS) $(INCLUDES) $< > $(DEPDIR)/$*.d
	@mv -f $(DEPDIR)/$*.d $(DEPDIR)/$*.d.tmp
	@$(SED) -e 's|.*:|$(OBJDIR)/$*.o: $(DEPDIR)/$*.d|' < $(DEPDIR)/$*.d.tmp > $(DEPDIR)/$*.d;
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $(DEPDIR)/$*.d.tmp | fmt -1 | \
	  $(SED) -e 's/^ *//' -e 's/$$/:/' >> $(DEPDIR)/.$*.d;
	@rm -f $(DEPDIR)/$*.d.tmp
	@$(CXX) $(CFLAGS) $(TARCH) $(INCLUDES) -o $@ $<

$(top_builddir)/gtest-main:	$(patsubst %.cc,$(OBJDIR)/%.o,$(gtestsrc)) \
	$(patsubst %.cpp,$(OBJDIR)/%.o,$(testsrc)) $(OZW_LIB)
	@echo "Linking $@"
	@$(LD) $(LDFLAGS) $(TARCH) -o $@ $+ $(LIBS) -pthread

test:	$(top_builddir)/gtest-main
	$(top_builddir)/gtest-main

clean:
	@rm -rf $(DEPDIR) $(OBJDIR) $(top_builddir)/gtest-main

.SUFFIXES:	.d .cpp .cc .o .a
