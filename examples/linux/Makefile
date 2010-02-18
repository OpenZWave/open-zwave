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

INCLUDES	:= -I ../../h -I ../../h/command_classes/ -I ../../h/value_classes/ \
	-I ../../h/platform/ -I ../../h/platform/mac -I ../../tinyxml/
LIBS := $(wildcard ../../lib/linux/*.a)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

all: test test1

test:	Main.o $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $< $(LIBS) -pthread

test1:	Main1.o $(LIBS)
	$(LD) -o $@ $(LDFLAGS) $< $(LIBS) -pthread
