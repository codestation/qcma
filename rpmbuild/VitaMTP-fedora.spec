# 
# VitaMTP spec file
# 

Name:           libvitamtp
Version:        2.5.1
Release:        0
%define sonum   3
Summary:        Low-level Vita communication library
License:        GPL-3.0
Group:          System/Libraries
URL:            https://github.com/codestation/VitaMTP
Source:         https://github.com/codestation/VitaMTP.git
BuildRequires:  pkgconfig
BuildRequires:  libxml2-devel
BuildRequires:  libusbx-devel

%description
libVitaMTP is a library based off of libMTP that does low level USB
communications with the Vita. It can read and receive MTP commands that
the Vita sends, which are a proprietary set of commands that is based on
the MTP open standard.

%package -n %{name}%{sonum}
Summary:        Low-level Vita communication library
Group:          System/Libraries

%description -n %{name}%{sonum}
libVitaMTP is a library based off of libMTP that does low level USB
communications with the Vita. It can read and receive MTP commands that
the Vita sends, which are a proprietary set of commands that is based on
the MTP open standard.

%package devel
Summary:        Low-level Vita communication library - development files
Group:          Development/Libraries/C and C++
Requires:       %{name}%{sonum} = %{version}
Requires:       libxml2-devel
Requires:       libusbx-devel

%description devel
libVitaMTP is a library based off of libMTP that does low level USB
communications with the Vita. It can read and receive MTP commands that
the Vita sends, which are a proprietary set of commands that is based on
the MTP open standard.
This package contains only the files necessary for development.

%prep
rm -rf $RPM_SOURCE_DIR/%{name}%{sonum}
%{!?_vitamtp_repo:%define _vitamtp_repo https://github.com/codestation/VitaMTP.git}
git clone "%{_vitamtp_repo}" $RPM_SOURCE_DIR/%{name}%{sonum}
cp -r $RPM_SOURCE_DIR/%{name}%{sonum} $RPM_BUILD_DIR/%{name}%{sonum}

%setup -n %{name}%{sonum} -DT

%build
./autogen.sh
./configure --prefix=/usr --libdir=%{_libdir}
make %{?_smp_mflags}

%install
%makeinstall
rm -rf %{buildroot}/%{_libdir}/*.la
mkdir -p %{buildroot}/usr/lib/udev/rules.d
cp debian/vitamtp%{sonum}.udev %{buildroot}/usr/lib/udev/rules.d/80-psvita.rules

%post -n %{name}%{sonum} -p /sbin/ldconfig
%postun -n %{name}%{sonum} -p /sbin/ldconfig

%files -n %{name}%{sonum}
%defattr(-,root,root)
%doc README.md ChangeLog COPYING
%{_libdir}/lib*.so.*
%{_udevrulesdir}/80-psvita.rules

%files devel
%defattr(-,root,root)
%{_prefix}/include/vitamtp.h
%{_libdir}/libvitamtp.a
%{_libdir}/libvitamtp.so
%{_libdir}/pkgconfig/libvitamtp.pc

%changelog
