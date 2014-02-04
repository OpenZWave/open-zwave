#
# Makefile for OpenZWave Linux build
# Greg Satz

# GNU make only

# Linux requires libudev-dev
# FreeBSD requires port misc/e2fsprogs-libuuid



#these variables can be modified by the user if needed


# what flags we will use for compiling in debug mode
DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-inline -Wno-format -Werror -g -DDEBUG -fPIC  -DSYSCONFDIR="\"$(PREFIX)/etc/openzwave/\""
# what flags we will use for compiling in release mode
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Werror -Wno-format -O3 -DNDEBUG -fPIC  -DSYSCONFDIR="\"$(PREFIX)/etc/openzwave/\""
#what flags we will use for linking in debug mode
DEBUG_LDFLAGS	:= -g

top_srcdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../../)

include $(top_srcdir)/cpp/build/support.mk


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
LIBDIR	?= $(top_builddir)

INCLUDES	:= -I $(top_srcdir)/cpp/src -I $(top_srcdir)/cpp/src/command_classes/ -I $(top_srcdir)/cpp/src/value_classes/ \
	-I $(top_srcdir)/cpp/src/platform/ -I $(top_srcdir)/cpp/src/platform/unix -I $(top_srcdir)/cpp/tinyxml/ -I $(top_srcdir)/cpp/hidapi/hidapi/

ifeq ($(UNAME),Darwin)
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/mac
else ifeq ($(UNAME),FreeBSD)
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/libusb
else
SOURCES_HIDAPI =$(top_srcdir)/cpp/hidapi/linux
endif


SOURCES		:= $(top_srcdir)/cpp/src $(top_srcdir)/cpp/src/command_classes $(top_srcdir)/cpp/tinyxml \
	$(top_srcdir)/cpp/src/value_classes $(top_srcdir)/cpp/src/platform $(top_srcdir)/cpp/src/platform/unix $(SOURCES_HIDAPI)
VPATH = $(top_srcdir)/cpp/src:$(top_srcdir)/cpp/src/command_classes:$(top_srcdir)/cpp/tinyxml:\
	$(top_srcdir)/cpp/src/value_classes:$(top_srcdir)/cpp/src/platform:$(top_srcdir)/cpp/src/platform/unix:$(SOURCES_HIDAPI)
	

tinyxml := $(notdir $(wildcard $(top_srcdir)/cpp/tinyxml/*.cpp))

ifeq ($(UNAME),Darwin)
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/mac/*.c))
else ifeq ($(UNAME),FreeBSD)
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/libusb/*.c))
else
hidapi := $(notdir $(wildcard $(top_srcdir)/cpp/hidapi/linux/*.c)) # we do not want the libusb version
endif

cclasses := $(notdir $(wildcard $(top_srcdir)/cpp/src/command_classes/*.cpp))
vclasses := $(notdir $(wildcard $(top_srcdir)/cpp/src/value_classes/*.cpp))
pform := $(notdir $(wildcard $(top_srcdir)/cpp/src/platform/*.cpp)) \
	$(notdir $(wildcard $(top_srcdir)/cpp/src/platform/unix/*.cpp))
indep := $(notdir $(filter-out $(top_srcdir)/cpp/src/vers.cpp, $(wildcard $(top_srcdir)/cpp/src/*.cpp)))

default: $(LIBDIR)/libopenzwave.a $(LIBDIR)/libopenzwave.so.$(VERSION)

clean:
	@rm -rf $(DEPDIR) $(OBJDIR) $(LIBDIR)/libopenzwave.so* $(LIBDIR)/libopenzwave.a $(top_builddir)/libopenzwave.pc $(top_builddir)/docs/api $(top_builddir)/Doxyfile


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


-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(tinyxml))
-include $(patsubst %.c,$(DEPDIR)/%.d,$(hidapi))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(cclasses))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(vclasses))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(pform))
-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(indep))

#create a vers.cpp file that contains our version and subversion revisions
$(top_srcdir)/cpp/src/vers.cpp:
	@echo "Creating $(top_srcdir)/cpp/src/vers.cpp file"
	@echo '#include "Defs.h"' > $(top_srcdir)/cpp/src/vers.cpp
	@echo 'uint16_t ozw_vers_major = $(VERSION_MAJ);' >> $(top_srcdir)/cpp/src/vers.cpp
	@echo 'uint16_t ozw_vers_minor = $(VERSION_MIN);' >> $(top_srcdir)/cpp/src/vers.cpp
	@echo 'uint16_t ozw_vers_revision = $(VERSION_REV);' >> $(top_srcdir)/cpp/src/vers.cpp

#$(OBJDIR)/vers.o:	$(top_builddir)/vers.cpp

$(LIBDIR)/libopenzwave.a:	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) \
			$(OBJDIR)/vers.o
	@echo "Linking Static Library"
	@$(AR) $@ $+
	@$(RANLIB) $@

$(LIBDIR)/libopenzwave.so.$(VERSION):	$(patsubst %.cpp,$(OBJDIR)/%.o,$(tinyxml)) \
			$(patsubst %.c,$(OBJDIR)/%.o,$(hidapi)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(cclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(vclasses)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(pform)) \
			$(patsubst %.cpp,$(OBJDIR)/%.o,$(indep)) \
			$(OBJDIR)/vers.o
	@echo "Linking Shared Library"
	@$(LD) $(LDFLAGS) $(LIBS) -shared  -o $@ $+
	@ln -sf libopenzwave.so.$(VERSION) $(LIBDIR)/libopenzwave.so

ifeq ($(PKGCONFIG),)
$(top_builddir)/libopenzwave.pc: $(top_srcdir)/cpp/build/libopenzwave.pc.in
	$(warning Not Installing Package Config File)
else
$(top_builddir)/libopenzwave.pc: $(top_srcdir)/cpp/build/libopenzwave.pc.in $(PKGCONFIG)
	@echo "Making libopenzwave pkg-config file"
	@$(SED) \
		-e 's|[@]prefix@|$(PREFIX)|g' \
		-e 's|[@]exec_prefix@|$(PREFIX)/bin|g' \
		-e 's|[@]libdir@|$(instlibdir)|g' \
		-e 's|[@]includedir@|$(PREFIX)/include/openzwave/|g' \
		-e 's|[@]VERSION@|$(VERSION)|g' \
		< "$<" > "$@"
endif

$(top_builddir)/Doxyfile: $(top_srcdir)/docs/Doxyfile.in
	@mkdir -p $(top_builddir)/docs/api/
	@$(SED) \
		-e 's|[@]OUTPUT_DIR@|$(top_builddir)/docs/api/|g' \
		-e 's|[@]TOPSRC_DIR@|$(top_srcdir)|g' \
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

install: $(LIBDIR)/libopenzwave.so.$(VERSION) doc $(top_builddir)/libopenzwave.pc 
	@install -D $(LIBDIR)/libopenzwave.so.$(VERSION) $(DESTDIR)/$(instlibdir)/libopenzwave.so.$(VERSION)
	@ln -sf libopenzwave.so.$(VERSION) $(DESTDIR)/$(instlibdir)/libopenzwave.so
	@install -d $(DESTDIR)/$(includedir)/openzwave/
	@install -m 0644 $(top_srcdir)/cpp/src/*.h $(DESTDIR)/$(includedir)/openzwave/
	@install -d $(DESTDIR)/$(includedir)/openzwave/command_classes/
	@install -m 0644 $(top_srcdir)/cpp/src/command_classes/*.h $(DESTDIR)/$(includedir)/openzwave/command_classes/
	@install -d $(DESTDIR)/$(includedir)/openzwave/value_classes/
	@install -m 0644 $(top_srcdir)/cpp/src/value_classes/*.h $(DESTDIR)/$(includedir)/openzwave/value_classes/
	@install -d $(DESTDIR)/$(includedir)/openzwave/platform/
	@install -m 0644 $(top_srcdir)/cpp/src/platform/*.h $(DESTDIR)/$(includedir)/openzwave/platform/	
	@install -d $(DESTDIR)/$(includedir)/openzwave/platform/unix/
	@install -m 0644 $(top_srcdir)/cpp/src/platform/unix/*.h $(DESTDIR)/$(includedir)/openzwave/platform/unix/
	@install -d $(DESTDIR)/$(sysconfdir)/
	@cp -r $(top_srcdir)/config/* $(DESTDIR)/$(sysconfdir)
	@install -d $(DESTDIR)/$(docdir)/
	@cp -r $(top_srcdir)/docs/* $(DESTDIR)/$(docdir)
	@if [ -d "$(top_builddir)/docs/html/" ]; then cp -r $(top_builddir)/docs/html/* $(DESTDIR)/$(docdir); fi
	install -d $(DESTDIR)/$(instlibdir)/pkgconfig/
	@if [ -a "$(PKGCONFIG)" ]; then install -d $(DESTDIR)/$(instlibdir)/pkgconfig/; cp $(top_builddir)/libopenzwave.pc $(DESTDIR)/$(instlibdir)/pkgconfig/; fi

	

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean install doc
