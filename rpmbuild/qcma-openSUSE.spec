# 
# qcma spec file
# 

Name:           qcma
Summary:        PSVita Content Manager Assistant
License:        GPL-3.0
Release:        1
Version:        0.3.0
URL:            https://github.com/codestation/qcma
Source:         https://github.com/codestation/qcma.git
Group:          Productivity/File utilities
Requires:       ffmpeg
Requires:       libqt5-qtbase
Requires:       libqt5-qtimageformats
Requires:       libvitamtp3 >= 2.5.0
BuildRequires:  pkg-config
BuildRequires:  ffmpeg-devel
BuildRequires:  libvitamtp-devel
BuildRequires:  libqt5-qttools
BuildRequires:  libqt5-qtbase-devel

%description
QCMA is an cross-platform application to provide a Open Source implementation
of the original Content Manager Assistant that comes with the PS Vita. QCMA
is meant to be compatible with Linux, Windows and MAC OS X.

%prep
rm -rf $RPM_SOURCE_DIR/%{name}-%{version}
%{!?_qcma_repo:%define _qcma_repo https://github.com/codestation/qcma.git}
git clone "%{_qcma_repo}" $RPM_SOURCE_DIR/%{name}-%{version}
cp -r $RPM_SOURCE_DIR/%{name}-%{version} $RPM_BUILD_DIR/%{name}-%{version}

%setup -n %{name}-%{version} -DT

%build
lrelease-qt5 resources/translations/*.ts
qmake-qt5 PREFIX=/usr qcma.pro CONFIG+=QT5_SUFFIX
make %{?_smp_mflags}

%install
make install INSTALL_ROOT=%{buildroot}

%files
%defattr(-,root,root)
%{_bindir}/qcma
%{_bindir}/qcma_cli
%{_prefix}/share/applications/qcma/qcma.desktop
%{_prefix}/share/icons/hicolor/64x64/apps/qcma.png

%changelog
