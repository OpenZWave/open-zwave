#The Major Version Number
VERSION_MAJ	?= 1
#The Minor Version Number
VERSION_MIN ?= 4

#the build type we are making (release or debug)
BUILD	?= release
#the prefix to install the library into
PREFIX	?= /usr/local


#the System we are building on
UNAME  := $(shell uname -s)
#the location of Doxygen to generate our api documentation
DOXYGEN := $(shell which doxygen)
#dot is required for doxygen (part of Graphviz)
DOT := $(shell which dot)
#the machine type we are building on (i686 or x86_64)
MACHINE := $(shell uname -m)
#the location of xmllink for checking our config files
XMLLINT := $(shell which xmllint)
#temp directory to build our tarfile for make dist target
TMP     := /tmp
#pkg-config binary for package config files
PKGCONFIG := $(shell which pkg-config)
#svn binary for doing a make dist export
GIT		:= $(shell which git)
# if svnversion is not installed, then set the revision to 0
ifeq ($(GIT),)
VERSION_REV ?= 0
else
GITVERSION	:= $(shell $(GIT) --git-dir $(top_srcdir)/.git describe --long --tags --dirty 2>/dev/null | sed s/^v//)
ifeq ($(GITVERSION),)
GITVERSION	:= $(VERSION_MAJ).$(VERSION_MIN).-1
VERSION_REV	:= 0
else
VERSION_REV 	?= $(shell echo $(GITVERSION) | awk '{split($$0,a,"-"); print a[2]}')
endif
endif
ifeq ($(VERSION_REV),)
VERSION_REV ?= 0
endif
# version number to use on the shared library
VERSION := $(VERSION_MAJ).$(VERSION_MIN)

# using seting from bitbake
ifeq ($(BITBAKE_ENV),1)
CC     := $(CC)
CXX    := $(CXX)
LD     := $(CXX)
AR     := $(AR)
RANLIB := $(RANLIB)
else

# support Cross Compiling options
ifeq ($(UNAME),FreeBSD)
# Actually hide behind c++ which works for both clang based 10.0 and earlier(?)
CC     := $(CROSS_COMPILE)cc
CXX    := $(CROSS_COMPILE)c++
LD     := $(CROSS_COMPILE)c++
else
CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
endif
ifeq ($(UNAME),Darwin)
AR     := libtool -static -o 
RANLIB := ranlib
else
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib
endif

endif
SED    := sed


#determine if we are release or debug Build and set appropriate flags
ifeq ($(BUILD), release)
CFLAGS	+= -c $(RELEASE_CFLAGS)
LDFLAGS	+= $(RELEASE_LDFLAGS)
else
CFLAGS	+= -c $(DEBUG_CFLAGS)
LDFLAGS	+= $(DEBUG_LDFLAGS)
endif

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
instlibdir ?= $(PREFIX)$(instlibdir.$(MACHINE))
else
instlibdir ?= $(PREFIX)$(instlibdir.default)
endif

#pkg-config doesn't exist, lets try to guess best place to put the pc file
ifeq ($(PKGCONFIG),)
pkgconfigdir ?= $(shell if [ -d "/usr/lib64/pkgconfig" ]; then echo "/usr/lib64/pkgconfig"; else echo "/usr/lib/pkgconfig"; fi)
else
pkgconfigdir ?= $(shell test -d "$(instlibdir)/pkgconfig" && echo "$(instlibdir)/pkgconfig" || pkg-config --variable pc_path pkg-config | awk -F: '{ print $$1 }')
endif

ifeq ($(BITBAKE_ENV),1)
sysconfdir := $(PREFIX)/etc/openzwave/
includedir := $(PREFIX)/include/openzwave/
docdir := $(PREFIX)/share/doc/openzwave-$(VERSION).$(VERSION_REV)
else
sysconfdir ?= $(PREFIX)/etc/openzwave/
includedir ?= $(PREFIX)/include/openzwave/
docdir ?= $(PREFIX)/share/doc/openzwave-$(VERSION).$(VERSION_REV)
endif

top_builddir ?= $(CURDIR)
export top_builddir

OBJDIR = $(top_builddir)/.lib
DEPDIR = $(top_builddir)/.dep

ifeq ($(UNAME),NetBSD)
FMTCMD = fmt -g 1
else
FMTCMD = fmt -1
endif

$(OBJDIR)/%.o : %.cpp
	@echo "Building $(notdir $@)"
	@$(CXX) -MM $(CFLAGS) $(INCLUDES) $< > $(DEPDIR)/$*.d
	@mv -f $(DEPDIR)/$*.d $(DEPDIR)/$*.d.tmp
	@$(SED) -e 's|.*:|$(OBJDIR)/$*.o: $(DEPDIR)/$*.d|' < $(DEPDIR)/$*.d.tmp > $(DEPDIR)/$*.d;
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $(DEPDIR)/$*.d.tmp | $(FMTCMD) | \
	  $(SED) -e 's/^ *//' -e 's/$$/:/' >> $(DEPDIR)/.$*.d;
	@rm -f $(DEPDIR)/$*.d.tmp
	@$(CXX) $(CFLAGS) $(TARCH) $(INCLUDES) -o $@ $<


$(OBJDIR)/%.o : %.c
	@echo "Building $(notdir $@)"	
	@$(CC) -MM $(CFLAGS) $(INCLUDES) $< > $(DEPDIR)/$*.d
	@mv -f $(DEPDIR)/$*.d $(DEPDIR)/$*.d.tmp
	@$(SED) -e 's|.*:|$(OBJDIR)/$*.o: $(DEPDIR)/$*.d|' < $(DEPDIR)/$*.d.tmp > $(DEPDIR)/$*.d;
	@$(SED) -e 's/.*://' -e 's/\\$$//' < $(DEPDIR)/$*.d.tmp | $(FMTCMD) | \
	  $(SED) -e 's/^ *//' -e 's/$$/:/' >> $(DEPDIR)/.$*.d;
	@rm -f $(DEPDIR)/$*.d.tmp
	@$(CC) $(CFLAGS) $(TARCH) $(INCLUDES) -o $@ $<


dummy := $(shell test -d $(OBJDIR) || mkdir -p $(OBJDIR))
dummy := $(shell test -d $(DEPDIR) || mkdir -p $(DEPDIR))
