# 
# VitaMTP spec file
# 

Name:           libvitamtp2
Summary:        Low-level Vita communication library
License:        GPL-3.0
Release:        2
Version:        2.1.0
URL:            https://github.com/codestation/VitaMTP
Source:         https://github.com/codestation/VitaMTP.git
Prefix:         /usr
Group:          System/Libraries

%package devel
Summary:        Low-level Vita communication library - development files
Group:          Development/Libraries/C and C++
Requires:       libvitamtp2 == 2.1.0

%description
libVitaMTP is a library based off of libMTP that does low level USB 
communications with the Vita. It can read and receive MTP commands that 
the Vita sends, which are a proprietary set of commands that is based on 
the MTP open standard.

%description devel
libVitaMTP is a library based off of libMTP that does low level USB 
communications with the Vita. It can read and receive MTP commands that 
the Vita sends, which are a proprietary set of commands that is based on 
the MTP open standard.
This package contains only the files necessary for development.

%changelog
* Tue Nov 05 2013 codestation <codestation> - 2.1.0
- Added new CMA version.

%prep
rm -rf $RPM_SOURCE_DIR/libvitamtp2
git clone https://github.com/codestation/VitaMTP.git $RPM_SOURCE_DIR/libvitamtp2
cp -r $RPM_SOURCE_DIR/libvitamtp2 $RPM_BUILD_DIR/libvitamtp2
%setup -n libvitamtp2 -DT

%build
./autogen.sh
./configure --prefix=/usr --disable-opencma
make

%install
make DESTDIR=%{buildroot} install
mkdir -p %{buildroot}/lib/udev/rules.d
cp debian/vitamtp1.udev %{buildroot}/lib/udev/rules.d/80-psvita.rules

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
/lib/udev/rules.d/80-psvita.rules
/usr/lib64/libvitamtp.la
/usr/lib64/libvitamtp.so.2
/usr/lib64/libvitamtp.so.2.0.0

%files devel
%defattr(-,root,root,-)
/usr/include/vitamtp.h
/usr/lib64/libvitamtp.a
/usr/lib64/libvitamtp.so
/usr/lib64/pkgconfig/libvitamtp.pc