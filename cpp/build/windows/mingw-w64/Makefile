#
# Makefile for OpenZWave: MinGW-W64 build
# Stefan Broekman

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean

VERSION_MAJ ?= 1
VERSION_MIN ?= 4
VERSION := $(VERSION_MAJ).$(VERSION_MIN)
GITVERSION	:= $(VERSION_MAJ).$(VERSION_MIN).-1
VERSION_REV	:= 0

CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib

DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-inline -Wno-format -Wno-attributes -Wno-error=sequence-point -Wno-sequence-point -ggdb -DDEBUG -fPIC
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -Wno-attributes -Wno-error=sequence-point -Wno-sequence-point -O3 -DNDEBUG -fPIC
DEBUG_LDFLAGS	:= -g

top_srcdir := ../../../..

STATIC_LIB_NAME=libopenzwave.a
SHARED_LIB_NAME=libopenzwave.dll

# Change for DEBUG or RELEASE
CFLAGS	:= -c $(RELEASE_CFLAGS)
LDFLAGS += -shared -Wl,-soname,$(SHARED_LIB_NAME) $(RELEASE_LDFLAGS)
LIBS 	+= -lsetupapi
LIBDIR	:= .
OBJDIR	:= .

INCLUDES := -I $(top_srcdir)/cpp/src -I $(top_srcdir)/cpp/tinyxml/ -I $(top_srcdir)/cpp/hidapi/hidapi/

SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/windows

SOURCES		:= $(top_srcdir)/cpp/src $(top_srcdir)/cpp/src/command_classes $(top_srcdir)/cpp/tinyxml \
	$(top_srcdir)/cpp/src/value_classes $(top_srcdir)/cpp/src/platform $(top_srcdir)/cpp/src/platform/windows $(SOURCES_HIDAPI) $(top_srcdir)/cpp/src/aes/

VPATH = $(top_srcdir)/cpp/src:$(top_srcdir)/cpp/src/command_classes:$(top_srcdir)/cpp/tinyxml:\
	$(top_srcdir)/cpp/src/value_classes:$(top_srcdir)/cpp/src/platform:$(top_srcdir)/cpp/src/platform/windows:$(SOURCES_HIDAPI):$(top_srcdir)/cpp/src/aes/

%.d : %.cpp
	$(CXX) -MM $(CFLAGS) $(INCLUDES) $< > $*.d
	
tinyxml := $(notdir $(wildcard $(top_srcdir)/cpp/tinyxml/*.cpp))
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/linux/*.c)) # we do not want the libusb version
cclasses := $(notdir $(wildcard $(top_srcdir)/cpp/src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard $(top_srcdir)/cpp/src/value_classes/*.cpp))
pform := $(notdir $(wildcard $(top_srcdir)/cpp/src/platform/*.cpp)) \
	$(notdir $(wildcard $(top_srcdir)/cpp/src/platform/unix/*.cpp))
indep := $(notdir $(filter-out $(top_srcdir)/cpp/src/vers.cpp, $(wildcard $(top_srcdir)/cpp/src/*.cpp)))
aes := $(notdir $(wildcard $(top_srcdir)/cpp/src/aes/*.c))

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

default:	$(LIBDIR)/$(STATIC_LIB_NAME)
#Shared lib compiles and links but not working in projects yet. Disabled for now.
#shared:	$(LIBDIR)/$(SHARED_LIB_NAME)
clean:		
	rm -rf *.d* *.o $(STATIC_LIB_NAME) $(SHARED_LIB_NAME) $(top_srcdir)/cpp/src/vers.cpp

-include $(patsubst %.cpp,%.d,$(tinyxml))
-include $(patsubst %.c,%.d,$(hidapi))
-include $(patsubst %.cpp,%.d,$(cclasses))
-include $(patsubst %.cpp,%.d,$(vclasses))
-include $(patsubst %.cpp,%.d,$(pform))
-include $(patsubst %.cpp,%.d,$(indep))
-include $(patsubst %.c,%.d,$(aes))

$(top_srcdir)/cpp/src/vers.cpp:
	@echo :: Creating vers.cpp
	@echo #include "Defs.h" > $(top_srcdir)/cpp/src/vers.cpp
	@echo uint16_t ozw_vers_major = $(VERSION_MAJ); >> $(top_srcdir)/cpp/src/vers.cpp
	@echo uint16_t ozw_vers_minor = $(VERSION_MIN); >> $(top_srcdir)/cpp/src/vers.cpp
	@echo uint16_t ozw_vers_revision = $(VERSION_REV); >> $(top_srcdir)/cpp/src/vers.cpp
	@echo char ozw_version_string[] = "$(GITVERSION)"; >> $(top_srcdir)/cpp/src/vers.cpp

$(LIBDIR)/$(STATIC_LIB_NAME): $(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.c,%.o,$(hidapi)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.c,%.o,$(aes)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	@echo :: Linking Static Library
	$(AR) $@ $+
	$(RANLIB) $@
	@echo :: Finished static library. Library written to: $(LIBDIR)/$(STATIC_LIB_NAME)

$(LIBDIR)/$(SHARED_LIB_NAME):	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(aes)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) \
			$(OBJDIR)/vers.o
	@echo :: Linking Shared Library
	$(LD) $(LDFLAGS) -o $@ $+ $(LIBS)
	@echo :: Finished shared library. Written to: $(LIBDIR)/$(SHARED_LIB_NAME)