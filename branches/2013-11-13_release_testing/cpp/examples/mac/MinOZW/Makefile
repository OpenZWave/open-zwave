#
# Makefile for OpenzWave Mac OS X applications
# Greg Satz

# GNU make only

.SUFFIXES:	.cpp .o .a

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-inline -Werror -Wno-format -g -DDEBUG
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Werror -Wno-format -O3 -DNDEBUG

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
ARCH :=  -arch i386 -arch x86_64
CFLAGS	:= -c -DDARWIN $(DEBUG_CFLAGS) $(ARCH)
LDFLAGS	:= $(DEBUG_LDFLAGS)

INCLUDES	:= -I ../../../src -I ../../../src/command_classes/ -I ../../../src/value_classes/ \
	-I ../../../src/platform/ -I ../../../h/platform/unix -I ../../../tinyxml/ -I ../../../hidapi/hidapi/
LIBS = $(wildcard ../../../lib/mac/*.a)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

all: test

lib:
	$(MAKE) -C ../../../build/mac

test:	Main.o #lib
	$(LD) -o $@ $(LDFLAGS) $< $(LIBS) -pthread -framework IOKit -framework CoreFoundation $(ARCH)

clean:
	rm -f test Main.o

XMLLINT := $(shell whereis xmllint)

ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../../config/zwcfg.xsd zwcfg_*.xml
	@$(XMLLINT) --noout --schema ../../../../config/zwscene.xsd zwscene.xml
endif
