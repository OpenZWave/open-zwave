#
# Makefile for OpenZWave Linux build
# Greg Satz

# GNU make only

# Linux requires libudev-dev
# FreeBSD requires port misc/e2fsprogs-libuuid



#these variables can be modified by the user if needed


# what flags we will use for compiling in debug mode
DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-inline -Werror -Wno-error=sequence-point -Wno-sequence-point -ggdb -DDEBUG -fPIC  -DSYSCONFDIR="\"$(PREFIX)/etc/openzwave/\"" -DOPENZWAVE_ENABLE_EXCEPTIONS=1
DEBUG_CPPFLAGS  := -std=c++11 
# what flags we will use for compiling in release mode
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Werror -Wno-error=sequence-point -Wno-sequence-point -O3 -DNDEBUG -fPIC  -DSYSCONFDIR="\"$(PREFIX)/etc/openzwave/\"" -DOPENZWAVE_ENABLE_EXCEPTIONS=1
RELEASE_CPPFLAGS := -std=c++11
#what flags we will use for linking in debug mode
DEBUG_LDFLAGS	:= -g

#default Libraries we should link with
ifneq ($(UNAME),FreeBSD)
LIBS += -lresolv
endif

top_srcdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../../)

include $(top_srcdir)/cpp/build/support.mk

#Mac prefers a dylib, not a so
ifeq ($(UNAME),Darwin)
SHARED_LIB_NAME=libopenzwave-$(VERSION).dylib
SHARED_LIB_UNVERSIONED=libopenzwave.dylib
else
SHARED_LIB_NAME=libopenzwave.so.$(VERSION)
SHARED_LIB_UNVERSIONED=libopenzwave.so
endif

#if we are on a Mac, add these flags and libs to the compile and link phases 
ifeq ($(UNAME),Darwin)
CFLAGS	+= -c -DDARWIN
ifeq ($(DARWIN_MOJAVE_UP),1)
# Newer macOS releases don't support i386 so only build 64-bit
DARWIN_BUILD_TARGET = -arch x86_64
else
DARWIN_BUILD_TARGET = -arch i386 -arch x86_64
endif
TARCH	+= $(DARWIN_BUILD_TARGET)
LDFLAGS += -dynamiclib -install_name "$(DESTDIR)/$(instlibdir)/$(SHARED_LIB_NAME)"
LIBS	+= -framework IOKit -framework CoreFoundation $(DARWIN_BUILD_TARGET)
else ifeq ($(UNAME),FreeBSD)
LDFLAGS+= -shared -lusb -Wl,-soname,$(SHARED_LIB_NAME)

# Pre FreeBSD 10.2 we have no native, or "old" native iconv.h (non-posix compliant
# const modifiers)
# For these, require libiconv pkg to be installed.
ifeq ($(shell test $$(uname -U) -ge 1002000; echo $$?),1)
# <10.2
ifeq (,$(wildcard /usr/local/include/iconv.h))
$(error FreeBSD pre 10.2: Please install libiconv from ports)
else
# Installed, use it
CFLAGS += -I/usr/local/include
# User application *must* use -liconv; we don't do any linking from this file since we only
# build a library.
endif
endif
# For 10.2 and later, use iconv from base, no extra include path required.

else ifeq ($(UNAME),NetBSD)
CFLAGS += $(shell pkgconf --cflags libusb-1.0)
LDFLAGS+= -shared $(shell pkgconf --libs libusb-1.0)
else ifeq ($(UNAME),SunOS)
CFLAGS+=  -I$(PREFIX)/include/libusb-1.0
LDFLAGS+= -shared -lusb-1.0 -liconv
else
LDFLAGS += -shared -Wl,-soname,$(SHARED_LIB_NAME)
endif
#CFLAGS  += $(CPPFLAGS)

#where to put the temporary library
LIBDIR	?= $(top_builddir)

INCLUDES	:= -I $(top_srcdir)/cpp/src


ifeq ($(USE_BI_TXML), 1)
INCLUDES += -I $(top_srcdir)/cpp/tinyxml/
SOURCES_TXML =$(top_srcdir)/cpp/tinyxml/
tinyxml := $(notdir $(wildcard $(top_srcdir)/cpp/tinyxml/*.cpp))
else
LIBS+= -ltinyxml
endif





ifeq ($(USE_HID),1)
INCLUDES += -I $(top_srcdir)/cpp/hidapi/hidapi/
CFLAGS += -DUSE_HID

ifeq ($(UNAME),Darwin)
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/mac
else ifeq ($(UNAME),FreeBSD)
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/libusb
else ifeq ($(UNAME),NetBSD)
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/libusb
else ifeq ($(UNAME),SunOS)
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/libusb
else
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/linux
LIBS += -ludev
endif
endif # USE_HID

SOURCES		:= $(top_srcdir)/cpp/src $(top_srcdir)/cpp/src/command_classes $(SOURCES_TXML) \
	$(top_srcdir)/cpp/src/value_classes $(top_srcdir)/cpp/src/platform $(top_srcdir)/cpp/src/platform/unix $(SOURCES_HIDAPI) $(top_srcdir)/cpp/src/aes/
VPATH = $(top_srcdir)/cpp/src:$(top_srcdir)/cpp/src/command_classes:$(SOURCES_TXML):\
	$(top_srcdir)/cpp/src/value_classes:$(top_srcdir)/cpp/src/platform:$(top_srcdir)/cpp/src/platform/unix:$(SOURCES_HIDAPI):$(top_srcdir)/cpp/src/aes/


ifeq ($(USE_HID),1)
ifeq ($(UNAME),Darwin)
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/mac/*.c))
else ifeq ($(UNAME),FreeBSD)
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/libusb/*.c))
else ifeq ($(UNAME),NetBSD)
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/libusb/*.c))
else ifeq ($(UNAME),SunOS)
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/libusb/*.c))
else
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/linux/*.c)) # we do not want the libusb version
endif
endif # USE_HID

ifeq ($(BITBAKE_ENV),1)
ar_option := "rc"
endif

cclasses := $(notdir $(wildcard $(top_srcdir)/cpp/src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard $(top_srcdir)/cpp/src/value_classes/*.cpp))
pform := $(notdir $(wildcard $(top_srcdir)/cpp/src/platform/*.cpp)) \
	$(notdir $(wildcard $(top_srcdir)/cpp/src/platform/unix/*.cpp))
indep := $(notdir $(filter-out $(top_srcdir)/cpp/src/vers.cpp, $(wildcard $(top_srcdir)/cpp/src/*.cpp)))
aes := $(notdir $(wildcard $(top_srcdir)/cpp/src/aes/*.c))


default: printversion $(LIBDIR)/libopenzwave.a $(LIBDIR)/$(SHARED_LIB_NAME) $(top_builddir)/ozw_config

clean:
	@rm -rf $(DEPDIR) $(OBJDIR) $(LIBDIR)/libopenzwave.so* $(LIBDIR)/libopenzwave*.dylib $(LIBDIR)/libopenzwave.a $(top_builddir)/libopenzwave.pc $(top_builddir)/docs/api $(top_builddir)/Doxyfile $(top_srcdir)/cpp/src/vers.cpp

printversion:
	@echo "Building OpenZWave Version $(GITVERSION)"	


-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(tinyxml))
-include $(patsubst %.c,$(DEPDIR)/%.d,$(hidapi))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(cclasses))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(vclasses))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(pform))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(indep))
-include $(patsubst %.c,$(DEPDIR)/%.d,$(aes))

#create a vers.cpp file that contains our version and subversion revisions
$(top_srcdir)/cpp/src/vers.cpp:
	@echo "Creating vers.cpp"
	@echo '#include "Defs.h"' > $(top_srcdir)/cpp/src/vers.cpp
	@echo 'uint16_t ozw_vers_major = $(VERSION_MAJ);' >> $(top_srcdir)/cpp/src/vers.cpp
	@echo 'uint16_t ozw_vers_minor = $(VERSION_MIN);' >> $(top_srcdir)/cpp/src/vers.cpp
	@echo 'uint16_t ozw_vers_revision = $(VERSION_REV);' >> $(top_srcdir)/cpp/src/vers.cpp
	@echo 'char ozw_version_string[] = "$(GITVERSION)";' >> $(top_srcdir)/cpp/src/vers.cpp
	@$(SED) \
		-e 's|[@]VERSION@|$(VERSION).$(VERSION_REV)|g' \
		< "$(top_srcdir)/dist/openzwave.spec.in" > "$(top_srcdir)/dist/openzwave.spec"


#$(OBJDIR)/vers.o:	$(top_builddir)/vers.cpp

$(LIBDIR)/libopenzwave.a:	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(aes)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) \
			$(OBJDIR)/vers.o
	@echo "Linking Static Library"
	@$(AR) $(ar_option) $@ $+
	@$(RANLIB) $@

$(LIBDIR)/$(SHARED_LIB_NAME):	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(aes)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) \
			$(OBJDIR)/vers.o
	@echo "Linking Shared Library"
	@$(LD) $(LDFLAGS) $(TARCH) -o $@ $+ $(LIBS)
	@ln -sf $(SHARED_LIB_NAME) $(LIBDIR)/$(SHARED_LIB_UNVERSIONED)

$(top_builddir)/libopenzwave.pc: $(top_srcdir)/cpp/build/libopenzwave.pc.in $(PKGCONFIG)
	@echo "Making libopenzwave pkg-config file"
	@$(SED) \
		-e 's|[@]prefix@|$(PREFIX)|g' \
		-e 's|[@]exec_prefix@|$(PREFIX)/bin|g' \
		-e 's|[@]libdir@|$(instlibdir)|g' \
		-e 's|[@]includedir@|$(includedir)/|g' \
                -e 's|[@]sysconfdir@|$(sysconfdir)/|g' \
                -e 's|[@]gitversion@|$(GITVERSION)|g' \
                -e 's|[@]docdir@|$(docdir)/|g' \
		-e 's|[@]VERSION@|$(VERSION)|g' \
		-e 's|[@]LIBS@|$(LIBS)|g' \
		< "$<" > "$@"
$(top_builddir)/ozw_config: $(top_srcdir)/cpp/build/ozw_config.in
	@echo "Making ozw_config file"
	@$(SED) \
		-e 's|[@]pkgconfigfile@|$(pkgconfigdir)/libopenzwave.pc|g' \
		< "$<" > "$@"
	@chmod +x $(top_builddir)/ozw_config

ifeq ($(DOT),)
HAVE_DOT = -e 's|[@]HAVE_DOT@|NO|g' 
else
HAVE_DOT = -e 's|[@]HAVE_DOT@|YES|g' 
endif

$(top_builddir)/Doxyfile: $(top_srcdir)/docs/Doxyfile.in $(top_srcdir)/cpp/src/vers.cpp
	@mkdir -p $(top_builddir)/docs/api/
	@$(SED) \
		-e 's|[@]OUTPUT_DIR@|$(top_builddir)/docs/api/|g' \
		-e 's|[@]TOPSRC_DIR@|$(top_srcdir)|g' \
		$(HAVE_DOT) \
		-e 's|[@]VERSION@|$(VERSION).$(VERSION_REV)|g' \
		< "$<" > "$@"

ifeq ($(DOXYGEN),)
doc:
		$(warning Documentation not being built)
else
doc: $(top_builddir)/Doxyfile
	@echo "Creating Documentation at $(top_builddir)/docs/api/"
	@mkdir -p $(top_builddir)/docs/api/
	@cd $(top_builddir); $(DOXYGEN)
endif

install: $(LIBDIR)/$(SHARED_LIB_NAME) doc $(top_builddir)/libopenzwave.pc $(top_builddir)/ozw_config
	@echo "Installing Shared Library"
	@install -d $(DESTDIR)/$(instlibdir)/
	@cp  $(LIBDIR)/$(SHARED_LIB_NAME) $(DESTDIR)/$(instlibdir)/$(SHARED_LIB_NAME)
	@ln -sf $(SHARED_LIB_NAME) $(DESTDIR)/$(instlibdir)/$(SHARED_LIB_UNVERSIONED)
	@echo "Installing Headers"
	@install -d $(DESTDIR)/$(includedir)
	@install -m 0644 $(top_srcdir)/cpp/src/*.h $(DESTDIR)/$(includedir)
	@install -d $(DESTDIR)/$(includedir)/command_classes/
	@install -m 0644 $(top_srcdir)/cpp/src/command_classes/*.h $(DESTDIR)/$(includedir)/command_classes/
	@install -d $(DESTDIR)/$(includedir)/value_classes/
	@install -m 0644 $(top_srcdir)/cpp/src/value_classes/*.h $(DESTDIR)/$(includedir)/value_classes/
	@install -d $(DESTDIR)/$(includedir)/platform/
	@install -m 0644 $(top_srcdir)/cpp/src/platform/*.h $(DESTDIR)/$(includedir)/platform/	
	@install -d $(DESTDIR)/$(includedir)/platform/unix/
	@install -m 0644 $(top_srcdir)/cpp/src/platform/unix/*.h $(DESTDIR)/$(includedir)/platform/unix/
	@install -d $(DESTDIR)/$(includedir)/aes/
	@install -m 0644 $(top_srcdir)/cpp/src/aes/*.h $(DESTDIR)/$(includedir)/aes/
	@install -d $(DESTDIR)/$(sysconfdir)/
	@echo "Installing Config Database"
	@cp -r $(top_srcdir)/config/* $(DESTDIR)/$(sysconfdir)
	@echo "Installing Documentation"
	@install -d $(DESTDIR)/$(docdir)/
	@cp -r $(top_srcdir)/docs/* $(DESTDIR)/$(docdir)
	@if [ -d "$(top_builddir)/docs/html/" ]; then cp -r $(top_builddir)/docs/html/* $(DESTDIR)/$(docdir); fi
	@echo "Installing Pkg-config Files"
	@install -d $(DESTDIR)/$(pkgconfigdir) 
	@cp $(top_builddir)/libopenzwave.pc $(DESTDIR)/$(pkgconfigdir)
	@install -d $(DESTDIR)/$(PREFIX)/bin/
	@cp $(top_builddir)/ozw_config $(DESTDIR)/$(PREFIX)/bin/ozw_config
	@chmod 755 $(DESTDIR)/$(PREFIX)/bin/ozw_config



.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean install doc
