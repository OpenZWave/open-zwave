#
# Makefile for OpenzWave Mac OS X applications
# Greg Satz

# GNU make only

# requires libudev-dev

.SUFFIXES:	.d .cpp .o .a
.PHONY:	default clean


DEBUG_CFLAGS    := -Wall -Wno-format -ggdb -DDEBUG $(CPPFLAGS)
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3 $(CPPFLAGS)

DEBUG_LDFLAGS	:= -g

top_srcdir := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../../../)

#where is put the temporary library
LIBDIR  	?= $(top_builddir)

INCLUDES	:= -I $(top_srcdir)/cpp/src -I $(top_srcdir)/cpp/tinyxml/ -I $(top_srcdir)/cpp/hidapi/hidapi/
LIBS =  $(wildcard $(LIBDIR)/*.so $(LIBDIR)/*.dylib $(top_builddir)/cpp/build/*.so $(top_builddir)/cpp/build/*.dylib )
LIBSDIR = $(abspath $(dir $(firstword $(LIBS))))
minozwsrc := $(notdir $(wildcard $(top_srcdir)/cpp/examples/MinOZW/*.cpp))
VPATH := $(top_srcdir)/cpp/examples/MinOZW

top_builddir ?= $(CURDIR)

default: $(top_builddir)/MinOZW

include $(top_srcdir)/cpp/build/support.mk

-include $(patsubst %.cpp,$(DEPDIR)/%.d,$(minozwsrc))

#if we are on a Mac, add these flags and libs to the compile and link phases 
ifeq ($(UNAME),Darwin)
CFLAGS += -DDARWIN
TARCH += -arch i386 -arch x86_64
endif

# Dup from main makefile, but that is not included when building here..
ifeq ($(UNAME),FreeBSD)
LDFLAGS+= -lusb

ifeq ($(shell test $$(uname -U) -ge 1002000; echo $$?),1)
ifeq (,$(wildcard /usr/local/include/iconv.h))
$(error FreeBSD pre 10.2: Please install libiconv from ports)
else
CFLAGS += -I/usr/local/include
LDFLAGS+= -L/usr/local/lib -liconv
endif
endif

else ifeq ($(UNAME),NetBSD)
LDFLAGS+= -L/usr/pkg/lib -lusb-1.0
else ifeq ($(UNAME),SunOS)
LDFLAGS+= -lusb-1.0
endif

$(OBJDIR)/MinOZW:	$(patsubst %.cpp,$(OBJDIR)/%.o,$(minozwsrc))
	@echo "Linking $(OBJDIR)/MinOZW"
	$(LD) $(LDFLAGS) $(TARCH) -o $@ $< $(LIBS) -pthread

$(top_builddir)/MinOZW: $(top_srcdir)/cpp/examples/MinOZW/MinOZW.in $(OBJDIR)/MinOZW
	@echo "Creating Temporary Shell Launch Script"
	@$(SED) \
		-e 's|[@]LDPATH@|$(LIBSDIR)|g' \
		< "$<" > "$@"
	@chmod +x $(top_builddir)/MinOZW

clean:
	@rm -rf $(DEPDIR) $(OBJDIR) $(top_builddir)/MinOZW

ifeq ($(XMLLINT),)
xmltest:	$(XMLLINT)
	$(error xmllint command not found.)
else
xmltest:	$(XMLLINT)
	@$(XMLLINT) --noout --schema ../../../config/zwcfg.xsd zwcfg_*.xml
	@$(XMLLINT) --noout --schema ../../../config/zwscene.xsd zwscene.xml
endif

install: $(OBJDIR)/MinOZW
	@echo "Installing into Prefix: $(PREFIX)"
	@install -d $(DESTDIR)/$(PREFIX)/bin/
	@cp $(OBJDIR)/MinOZW $(DESTDIR)/$(PREFIX)/bin/MinOZW
	@chmod 755 $(DESTDIR)/$(PREFIX)/bin/MinOZW
