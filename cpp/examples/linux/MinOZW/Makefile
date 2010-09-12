#
# Makefile for OpenzWave Mac OS X applications
# Greg Satz

# GNU make only

.SUFFIXES:	.cpp .o .a .s

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CFLAGS    := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)

INCLUDES	:= -I ../../../src -I ../../../src/command_classes/ -I ../../../src/value_classes/ \
	-I ../../../src/platform/ -I ../../../h/platform/unix -I ../../../tinyxml/
LIBS := $(wildcard ../../../lib/linux/*.a)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

all: test

test:	Main.o $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $< $(LIBS) -pthread

clean:
	rm -f test Main.o
