#
# Makefile for OpenZWave Mac OS X build
# Greg Satz

# GNU make only

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CFLAGS    := -Wall -Wno-format -g -DDEBUG -DLOG_STDERR
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c -DDARWIN $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)

LIBDIR	:= ../../lib/mac

INCLUDES	:= -I ../../h -I ../../h/command_classes/ -I ../../h/value_classes/ \
	-I ../../h/platform/ -I ../../h/platform/unix -I ../../tinyxml/
SOURCES		:= ../../src ../../src/command_classes ../../tinyxml \
	../../src/value_classes ../../src/platform ../../src/platform/unix

VPATH = ../../src:../../src/command_classes:../../tinyxml:\
	../../src/value_classes:../../src/platform:../../src/platform/unix

%.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

tinyxml := $(notdir $(wildcard ../../tinyxml/*.cpp))
cclasses := $(notdir $(wildcard ../../src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard ../../src/value_classes/*.cpp))
pform := $(notdir $(wildcard ../../src/platform/*.cpp)) \
	$(notdir $(wildcard ../../src/platform/unix/*.cpp))
indep := $(notdir $(wildcard ../../src/*.cpp))

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

default:	$(LIBDIR)/openzwave.a

clean:
	rm -f *.d *.o

-include $(tinyxml:.cpp=.d)
-include $(cclasses:.cpp=.d)
-include $(vclasses:.cpp=.d)
-include $(pform:.cpp=.d)
-include $(indep:.cpp=.d)

vers.c:
	echo 'char ozw_vers[] = "OpenZWave version 1.0.'`svnversion ../..`'";' > vers.c

vers.o:	vers.c

$(LIBDIR)/openzwave.a:	$(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	$(AR) $@ $?
