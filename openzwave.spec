%define version 1.0

Name: libopenzwave
Group: Home Automation
Summary: libary to access Z-Wave interfaces
License: LGPL
Version: %{version}
Release: 3
BuildRequires: gcc-c++ make libudev-devel
Requires: libudev
Source0: libopenzwave-%{version}.tar.gz
Patch0: debian/patches/soname.patch

BuildRoot: %{_tmppath}/libopenzwave-root

%description
Control your Z-Wave devices with this library

%package -n libopenzwave-devel
Summary: header files
Group: Home Automation

%description -n libopenzwave-devel
header files needed when you want to compile your own application using open zwave

%prep

%setup -q

%patch0 -p1

%build
make -j 5 -f debian/Makefile

%install
DESTDIR=${RPM_BUILD_ROOT} make -f debian/Makefile install

%files
%defattr(-,root,root,-)
/usr/lib
/usr/share

%files -n libopenzwave-devel
%defattr(-,root,root,-)
/usr/include

%doc

%post
/sbin/ldconfig -X
test -d /etc/openzwave || mkdir -p /etc/openzwave
test -e /etc/openzwave/config || ln -s /usr/share/openzwave/config /etc/openzwave/config

%postun
/sbin/ldconfig -X

%changelog
* Thu Oct 29 2012 Harald Klein <hari at vt100.at>
- initial version
