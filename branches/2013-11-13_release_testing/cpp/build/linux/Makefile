#
# Makefile for OpenZWave Linux build
# Greg Satz

# GNU make only

# Requires libudev-dev

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean verscheck

#these variables can be modified by the user if needed

#The Major Version Number
VERSION_MAJ	:= 1
#The Minor Version Number
VERSION_MIN := 0
#The Location of the svnversion command for determining the repository version
SVNVERSION := $(shell which svnversion)
#the build type we are making (release or debug)
BUILD	?= release
#the prefix to install the library into
PREFIX	?= /usr/local
#the System we are building on
UNAME  := $(shell uname -s)
#the location of Doxygen to generate our api documentation
DOXYGEN := $(shell which doxygen)
#the machine type we are building on (i686 or x86_64)
MACHINE := $(shell uname -m)
#the location of xmllink for checking our config files
XMLLINT := $(shell which xmllint)
#temp directory to build our tarfile for make dist target
TMP     := /tmp
#svn binary for doing a make dist export
SVN		:= $(shell which svn)
# if svnversion is not installed, then set the revision to 0
ifeq ($(SVNVERSION),)
VERSION_REV := 0
else
VERSION_REV := $(shell $(SVNVERSION) ../..|awk -F'[^0-9]*' '$$0=$$1')
endif
# version number to use on the shared library
VERSION := $(VERSION_MAJ).$(VERSION_MIN)

# support Cross Compiling options
CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib

# what flags we will use for compiling in debug mode
DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-inline -Wno-format -Werror -g -DDEBUG -fPIC
# what flags we will use for compiling in release mode
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Werror -Wno-format -O3 -DNDEBUG -fPIC
#what flags we will use for linking in debug mode
DEBUG_LDFLAGS	:= -g

#determine if we are release or debug Build and set appropriate flags
ifeq ($(BUILD), release)
CFLAGS	:= -c $(RELEASE_CFLAGS)
LDFLAGS	:= $(RELEASE_LDFLAGS)
else
CFLAGS	:= -c $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)
endif

#if we are on a Mac, add these flags and libs to the compile and link phases 
ifeq ($(UNAME), Darwin)
CFLAGS	+= -c -DDARWIN -arch i386 -arch x86_64
LDFLAGS += -arch i386 -arch x86_64
LIBS	+= -framework IOKit -framework CoreFoundation -arch i386 -arch x86_64
endif

#where to put the temporary library
LIBDIR	:= ../../lib/linux
#if /lib64 exists, then setup x86_64 library path to lib64 (good indication if a linux has /lib and lib64). 
#Else, if it doesnt, then set as /lib. This is used in the make install target 
ifeq ($(wildcard /lib64),)
instlibdir.x86_64 = /lib
else
instlibdir.x86_64 = /lib64
endif
instlibdir.i686   = /lib
instlibdir.i586   = /lib
instlibdir.i386   = /lib
instlibdir.i486   = /lib

#our actual install location for the library
instlibdir = $(PREFIX)$(instlibdir.$(MACHINE))


INCLUDES	:= -I ../../src -I ../../src/command_classes/ -I ../../src/value_classes/ \
	-I ../../src/platform/ -I ../../src/platform/unix -I ../../tinyxml/ -I ../../hidapi/hidapi/
SOURCES		:= ../../src ../../src/command_classes ../../tinyxml ../../hidapi/linux \
	../../src/value_classes ../../src/platform ../../src/platform/unix

VPATH = ../../src:../../src/command_classes:../../tinyxml:../../hidapi/linux:\
	../../src/value_classes:../../src/platform:../../src/platform/unix

%.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


tinyxml := $(notdir $(wildcard ../../tinyxml/*.cpp))
ifeq ($(UNAME), Darwin)
hidapi := $(notdir $(wildcard ../../hidapi/mac/*.c))
else
hidapi := $(notdir $(wildcard ../../hidapi/linux/hid.c)) # we do not want the libusb version
endif
cclasses := $(notdir $(wildcard ../../src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard ../../src/value_classes/*.cpp))
pform := $(notdir $(wildcard ../../src/platform/*.cpp)) \
	$(notdir $(wildcard ../../src/platform/unix/*.cpp))
indep := $(notdir $(wildcard ../../src/*.cpp))

%.o : %.cpp
	@echo "Building $@"
	@$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

%.o : %.c
	@echo "Building $@"	
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

default:	vers.cpp $(LIBDIR)/libopenzwave.a $(LIBDIR)/libopenzwave.so.$(VERSION)

clean:
	rm -f *.d *.d.* *.o $(LIBDIR)/libopenzwave.* openzwave*.tar.gz


ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../config/device_classes.xsd ../../../config/device_classes.xml
	@$(XMLLINT) --noout --schema ../../../config/options.xsd ../../../config/options.xml
	@$(XMLLINT) --noout --schema ../../../config/manufacturer_specific.xsd ../../../config/manufacturer_specific.xml
	@$(XMLLINT) --noout --schema ../../../config/device_configuration.xsd ../../../config/*/*.xml
endif

-include $(tinyxml:.cpp=.d)
-include $(hidapi:.c=.d)
-include $(cclasses:.cpp=.d)
-include $(vclasses:.cpp=.d)
-include $(pform:.cpp=.d)
-include $(indep:.cpp=.d)

#create a vers.cpp file that contains our version and subversion revisions
vers.cpp:
	@echo "Creating Vers.cpp file"
	@echo '#include "Defs.h"' > vers.cpp
	@echo 'uint16_t ozw_vers_major = $(VERSION_MAJ);' >> vers.cpp
	@echo 'uint16_t ozw_vers_minor = $(VERSION_MIN);' >> vers.cpp
	@echo 'uint16_t ozw_vers_revision = $(VERSION_REV);' >> vers.cpp

vers.o:	vers.cpp

$(LIBDIR)/libopenzwave.a:	$(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.c,%.o,$(hidapi)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	@echo "Linking Static Library"
	@$(AR) $@ $?

$(LIBDIR)/libopenzwave.so.$(VERSION):	$(patsubst %.cpp,%.o,$(tinyxml)) \
			$(patsubst %.c,%.o,$(hidapi)) \
			$(patsubst %.cpp,%.o,$(cclasses)) \
			$(patsubst %.cpp,%.o,$(vclasses)) \
			$(patsubst %.cpp,%.o,$(pform)) \
			$(patsubst %.cpp,%.o,$(indep)) vers.o
	@echo "Linking Shared Library"
	@$(LD) $(LDFLAGS) $(LIBS) -shared -Wl,-soname,libopenzwave.so.$(VERSION) -o $@ $+
	@ln -sf libopenzwave.so.$(VERSION) $(LIBDIR)/libopenzwave.so


ifeq ($(DOXYGEN),)
doc:
		$(warning Documentation not being built)
else
doc:
		@cd ../../../docs/; PROJECT_NUMBER=$(VERSION).$(VERSION_REV) $(DOXYGEN)
endif
	
install: $(LIBDIR)/libopenzwave.so.$(VERSION) doc
	@install $(LIBDIR)/libopenzwave.so.$(VERSION) $(instlibdir)
	@ln -sf libopenzwave.so.$(VERSION) $(instlibdir)/libopenzwave.so
	@install -d $(PREFIX)/include/openzwave/
	@install ../../src/*.h $(PREFIX)/include/openzwave/
	@install -d $(PREFIX)/include/openzwave/command_classes/
	@install ../../src/command_classes/*.h $(PREFIX)/include/openzwave/command_classes/
	@install -d $(PREFIX)/include/openzwave/value_classes/
	@install ../../src/value_classes/*.h $(PREFIX)/include/openzwave/value_classes/
	@install -d $(PREFIX)/include/openzwave/platform/
	@install ../../src/platform/*.h $(PREFIX)/include/openzwave/platform/	
	@install -d $(PREFIX)/include/openzwave/platform/unix/
	@install ../../src/platform/unix/*.h $(PREFIX)/include/openzwave/platform/unix/
	@install -d $(PREFIX)/etc/openzwave/
	@cp -r ../../../config/* $(PREFIX)/etc/openzwave/
	@install -d $(PREFIX)/share/doc/openzwave-$(VERSION).$(VERSION_REV)
	@cp -r ../../../docs/* $(PREFIX)/share/doc/openzwave-$(VERSION).$(VERSION_REV)
	
SVNPATH  = $(shell $(SVN) info ../../..|awk '/^Path:/ {print $$NF}')
dist: vers.cpp doc
	@rm -rf $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/
	@echo "Exporting Current OZW Repository at $(SVNPATH)"
	@$(SVN) export -q --force $(SVNPATH) $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/
	@cp vers.cpp $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/cpp/build/linux/
	@cp -r ../../../docs/api-doc/ $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/docs/
	@echo "Creating and Compressing openzwave-$(VERSION).$(VERSION_REV).tar.gz"
	@cd $(TMP); tar -czf openzwave-$(VERSION).$(VERSION_REV).tar.gz openzwave-$(VERSION).$(VERSION_REV)/*
	rm -rf $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/
	@cp $(TMP)/openzwave-$(VERSION).$(VERSION_REV).tar.gz .
	@echo "openzwave-$(VERSION).$(VERSION_REV).tar.gz is ready"
	