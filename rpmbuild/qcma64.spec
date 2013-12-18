# 
# qcma spec file
# 

Name:           qcma
Summary:        PSVita Content Manager Assistant
License:        GPL-3.0
Release:        1
Version:        0.2.4
URL:            https://github.com/codestation/qcma
Source:         https://github.com/codestation/qcma.git
Prefix:         /usr
Group:          Productivity/File utilities

%description
QCMA is an cross-platform application to provide a Open Source implementation
of the original Content Manager Assistant that comes with the PS Vita. QCMA
is meant to be compatible with Linux, Windows and MAC OS X.

%changelog
* Tue Nov 06 2013 codestation <codestation> - 0.2.4
- QCMA in WiFi mode is compatible with FW 3.00.

%prep
rm -rf $RPM_SOURCE_DIR/qcma
git clone https://github.com/codestation/qcma.git $RPM_SOURCE_DIR/qcma
cp -r $RPM_SOURCE_DIR/qcma $RPM_BUILD_DIR/qcma
%setup -n qcma -DT

%build
lrelease resources/translations/*.ts
qmake PREFIX=/usr
make

%install
make INSTALL_ROOT=%{buildroot} install

%files
%defattr(-,root,root,-)
/usr/bin/qcma
/usr/share/applications/qcma/qcma.desktop
/usr/share/icons/hicolor/64x64/apps/qcma.png