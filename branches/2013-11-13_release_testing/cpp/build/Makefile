#
# Makefile for OpenZWave Linux build
# Greg Satz

# GNU make only

# Linux requires libudev-dev
# FreeBSD requires port misc/e2fsprogs-libuuid

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
#pkg-config binary for package config files
PKGCONFIG := $(shell which pkg-config)
#svn binary for doing a make dist export
SVN		:= $(shell which svn)
# if svnversion is not installed, then set the revision to 0
ifeq ($(SVNVERSION),)
VERSION_REV := 0
else
VERSION_REV := $(shell $(SVNVERSION) ../..|awk -F'[^0-9]*' '$$0=$$1')
endif
ifeq ($(VERSION_REV),)
VERSION_REV := 0
endif
# version number to use on the shared library
VERSION := $(VERSION_MAJ).$(VERSION_MIN)

# support Cross Compiling options
CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib
SED    := sed

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
ifeq ($(UNAME),Darwin)
CFLAGS	+= -c -DDARWIN -arch i386 -arch x86_64
LDFLAGS += -arch i386 -arch x86_64
LIBS	+= -framework IOKit -framework CoreFoundation -arch i386 -arch x86_64
else ifeq ($(UNAME),FreeBSD)
CFLAGS += -I/usr/local/include
else
LDFLAGS += -Wl,-soname,libopenzwave.so.$(VERSION)
endif

#where to put the temporary library
LIBDIR	:= ../lib/
#if /lib64 exists, then setup x86_64 library path to lib64 (good indication if a linux has /lib and lib64). 
#Else, if it doesnt, then set as /lib. This is used in the make install target 
ifeq ($(wildcard /lib64),)
instlibdir.x86_64 = /lib/
else
instlibdir.x86_64 = /lib64/
endif
instlibdir.default   = /lib/

#our actual install location for the library
ifneq ($(instlibdir.$(MACHINE)),)
instlibdir = $(PREFIX)$(instlibdir.$(MACHINE))
else
instlibdir = $(PREFIX)$(instlibdir.default)
endif

INCLUDES	:= -I ../src -I ../src/command_classes/ -I ../src/value_classes/ \
	-I ../src/platform/ -I ../src/platform/unix -I ../tinyxml/ -I ../hidapi/hidapi/

ifeq ($(UNAME),Darwin)
SOURCES_HIDAPI =../hidapi/mac
else ifeq ($(UNAME),FreeBSD)
SOURCES_HIDAPI =../hidapi/libusb
else
SOURCES_HIDAPI =../hidapi/linux
endif


OBJDIR = .lib
DEPDIR = .dep
SOURCES		:= ../src ../src/command_classes ../tinyxml \
	../src/value_classes ../src/platform ../src/platform/unix $(SOURCES_HIDAPI)
VPATH = ../src:../src/command_classes:../tinyxml:\
	../src/value_classes:../src/platform:../src/platform/unix:$(SOURCES_HIDAPI)
	

tinyxml := $(notdir $(wildcard ../tinyxml/*.cpp))

ifeq ($(UNAME),Darwin)
hidapi := $(notdir $(wildcard ../hidapi/mac/*.c))
else ifeq ($(UNAME),FreeBSD)
hidapi := $(notdir $(wildcard ../hidapi/libusb/*.c))
else
hidapi := $(notdir $(wildcard ../hidapi/linux/*.c)) # we do not want the libusb version
endif

cclasses := $(notdir $(wildcard ../src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard ../src/value_classes/*.cpp))
pform := $(notdir $(wildcard ../src/platform/*.cpp)) \
	$(notdir $(wildcard ../src/platform/unix/*.cpp))
indep := $(notdir $(wildcard ../src/*.cpp))

$(OBJDIR)/%.o : %.cpp
	@echo "Building $(notdir $@)"
	@$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

$(OBJDIR)/%.o : %.c
	@echo "Building $(notdir $@)"	
	@$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

default:	vers.cpp $(LIBDIR)/libopenzwave.a $(LIBDIR)/libopenzwave.so.$(VERSION)

clean:
	rm -rf $(DEPDIR) $(OBJDIR) $(LIBDIR)/libopenzwave.* openzwave*.tar.gz *.pc


ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../config/device_classes.xsd ../../config/device_classes.xml
	@$(XMLLINT) --noout --schema ../../config/options.xsd ../../config/options.xml
	@$(XMLLINT) --noout --schema ../../config/manufacturer_specific.xsd ../../config/manufacturer_specific.xml
	@$(XMLLINT) --noout --schema ../../config/device_configuration.xsd ../../config/*/*.xml
endif

$(DEPDIR)/%.d : %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDES) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(DEPDIR)/%.d : %.c
	@set -e; rm -f $@; \
	$(CXX) -MM $(INCLUDES) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(tinyxml))
-include $(patsubst %.c,$(DEPDIR)/%.d,$(hidapi))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(cclasses))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(vclasses))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(pform))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(indep))

#create a vers.cpp file that contains our version and subversion revisions
vers.cpp:
	@echo "Creating Vers.cpp file"
	@echo '#include "Defs.h"' > vers.cpp
	@echo 'uint16_t ozw_vers_major = $(VERSION_MAJ);' >> vers.cpp
	@echo 'uint16_t ozw_vers_minor = $(VERSION_MIN);' >> vers.cpp
	@echo 'uint16_t ozw_vers_revision = $(VERSION_REV);' >> vers.cpp

$(OBJDIR)/vers.o:	vers.cpp

$(LIBDIR)/libopenzwave.a:	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) $(OBJDIR)/vers.o
	@echo "Linking Static Library"
	@$(AR) $@ $?
	@$(RANLIB) $@

$(LIBDIR)/libopenzwave.so.$(VERSION):	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) $(OBJDIR)/vers.o
	@echo "Linking Shared Library"
	@$(LD) $(LDFLAGS) $(LIBS) -shared  -o $@ $+
	@ln -sf libopenzwave.so.$(VERSION) $(LIBDIR)/libopenzwave.so

ifeq ($(PKGCONFIG),)
libopenzwave.pc: libopenzwave.pc.in
	$(warning Not Installing Package Config File)
else
libopenzwave.pc: libopenzwave.pc.in $(PKGCONFIG)
	@echo "Making libopenzwave pkg-config file"
	@$(SED) \
		-e 's|[@]prefix@|$(PREFIX)|g' \
		-e 's|[@]exec_prefix@|$(PREFIX)/bin|g' \
		-e 's|[@]libdir@|$(instlibdir)|g' \
		-e 's|[@]includedir@|$(PREFIX)/include/openzwave/|g' \
		-e 's|[@]VERSION@|$(VERSION)|g' \
		< "$<" > "$@"
	@install -D libopenzwave.pc $(instlibdir)/pkgconfig/libopenzwave.pc
endif

ifeq ($(DOXYGEN),)
doc:
		$(warning Documentation not being built)
else
doc:
		@cd ../../docs/; PROJECT_NUMBER=$(VERSION).$(VERSION_REV) $(DOXYGEN)
endif
	
install: $(LIBDIR)/libopenzwave.so.$(VERSION) doc libopenzwave.pc
	@install -D $(LIBDIR)/libopenzwave.so.$(VERSION) $(instlibdir)/libopenzwave.so.$(VERSION)
	@ln -sf libopenzwave.so.$(VERSION) $(instlibdir)/libopenzwave.so
	@install -d $(PREFIX)/include/openzwave/
	@install ../src/*.h $(PREFIX)/include/openzwave/
	@install -d $(PREFIX)/include/openzwave/command_classes/
	@install ../src/command_classes/*.h $(PREFIX)/include/openzwave/command_classes/
	@install -d $(PREFIX)/include/openzwave/value_classes/
	@install ../src/value_classes/*.h $(PREFIX)/include/openzwave/value_classes/
	@install -d $(PREFIX)/include/openzwave/platform/
	@install ../src/platform/*.h $(PREFIX)/include/openzwave/platform/	
	@install -d $(PREFIX)/include/openzwave/platform/unix/
	@install ../src/platform/unix/*.h $(PREFIX)/include/openzwave/platform/unix/
	@install -d $(PREFIX)/etc/openzwave/
	@cp -r ../../config/* $(PREFIX)/etc/openzwave/
	@install -d $(PREFIX)/share/doc/openzwave-$(VERSION).$(VERSION_REV)
	@cp -r ../../docs/* $(PREFIX)/share/doc/openzwave-$(VERSION).$(VERSION_REV)
	@if [ -a "$(PKGCONFIG)" ]; then install -D libopenzwave.pc $(instlibdir)/pkgconfig/libopenzwave.pc; fi

	
SVNPATH  = $(shell $(SVN) info ../..|awk '/^Path:/ {print $$NF}')
dist: vers.cpp doc
	@rm -rf $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/
	@echo "Exporting Current OZW Repository at $(SVNPATH)"
	@$(SVN) export -q --force $(SVNPATH) $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/
	@cp vers.cpp $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/cpp/build/linux/
	@cp -r ../../docs/api-doc/ $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/docs/
	@echo "Creating and Compressing openzwave-$(VERSION).$(VERSION_REV).tar.gz"
	@cd $(TMP); tar -czf openzwave-$(VERSION).$(VERSION_REV).tar.gz openzwave-$(VERSION).$(VERSION_REV)/*
	rm -rf $(TMP)/openzwave-$(VERSION).$(VERSION_REV)/
	@cp $(TMP)/openzwave-$(VERSION).$(VERSION_REV).tar.gz .
	@echo "openzwave-$(VERSION).$(VERSION_REV).tar.gz is ready"

dummy := $(shell test -d $(OBJDIR) || mkdir -p $(OBJDIR))
dummy := $(shell test -d $(DEPDIR) || mkdir -p $(DEPDIR))