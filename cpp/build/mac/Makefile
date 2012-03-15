#
# Makefile for OpenZWave Mac OS X build
# Greg Satz

# GNU make only

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean verscheck

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CFLAGS    := -Wall -Wno-format -g -DDEBUG -DLOG_STDERR -fPIC
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3 -DNDEBUG -fPIC

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c -DDARWIN $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)

LIBDIR	:= ../../lib/mac

INCLUDES	:= -I ../../src -I ../../src/command_classes/ -I ../../src/value_classes/ \
	-I ../../src/platform/ -I ../../src/platform/unix -I ../../tinyxml/ -I ../../hidapi/hidapi/
SOURCES		:= ../../src ../../src/command_classes ../../tinyxml ../../hidapi/mac\
	../../src/value_classes ../../src/platform ../../src/platform/unix

VPATH = ../../src:../../src/command_classes:../../tinyxml:../../hidapi/mac:\
	../../src/value_classes:../../src/platform:../../src/platform/unix

%.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

tinyxml := $(notdir $(wildcard ../../tinyxml/*.cpp))
hidapi := $(notdir $(wildcard ../../hidapi/mac/*.c))
cclasses := $(notdir $(wildcard ../../src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard ../../src/value_classes/*.cpp))
pform := $(notdir $(wildcard ../../src/platform/*.cpp)) \
	$(notdir $(wildcard ../../src/platform/unix/*.cpp))
indep := $(notdir $(wildcard ../../src/*.cpp))

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

default:	$(LIBDIR)/libopenzwave.a $(LIBDIR)/libopenzwave.so

clean:
	rm -f *.d *.d.* *.o vers.c

-include $(tinyxml:.cpp=.d)
-include $(hidapi:.c=.d)
-include $(cclasses:.cpp=.d)
-include $(vclasses:.cpp=.d)
-include $(pform:.cpp=.d)
-include $(indep:.cpp=.d)

vers.c:	verscheck
	echo 'char ozw_vers[] = "OpenZWave version 1.0.'`svnversion ../..`'";' > vers.c

vers.o:	vers.c

$(LIBDIR)/libopenzwave.a:	$(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.c,%.o,$(hidapi)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	$(AR) $@ $?

$(LIBDIR)/libopenzwave.so:	$(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.c,%.o,$(hidapi)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	$(LD) -shared -o $@ $+ -framework IOKit -framework CoreFoundation
