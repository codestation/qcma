# 
# qcma spec file
# 

%define _version 0.5.2

%if "%{_version}" == "testing" || "%{_version}" == "master"
%define _verprefix %{_version}
%else
%define _verprefix v%{_version}
%endif

%if 0%{?fedora}
%define _pkgconfig pkgconfig
%define _qt6linguist qt6-linguist
%define _qt6basedevel qt6-qtbase-devel
%else
%define _pkgconfig pkg-config
%define _qt6linguist  qt6-tools-linguist
%define _qt6basedevel qt6-base-devel
%endif

Name:           qcma
Summary:        PSVita Content Manager Assistant
License:        GPL-3.0-or-later
Release:        1
Version:        %{_version}
URL:            https://github.com/codestation/qcma
Source:         https://github.com/codestation/qcma/archive/%{_verprefix}/qcma-%{_version}.tar.gz
Group:          Productivity/File utilities
Requires:       libvitamtp5 >= 2.5.11
BuildRequires:  gcc-c++
BuildRequires:  %{_pkgconfig}
BuildRequires:  libnotify-devel
BuildRequires:  libvitamtp-devel
BuildRequires:  %{_qt6linguist}
BuildRequires:  %{_qt6basedevel}
BuildRequires:  desktop-file-utils
%if 0%{?fedora}
BuildRequires:  ffmpeg-free-devel
%else
BuildRequires:  libavformat-devel
BuildRequires:  libavcodec-devel
BuildRequires:  libavutil-devel
BuildRequires:  libswscale-devel
%endif

%description
QCMA is an cross-platform application to provide a Open Source implementation
of the original Content Manager Assistant that comes with the PS Vita. QCMA
is meant to be compatible with Linux, Windows and MAC OS X.

%prep
%autosetup -n %{name}-%{version}

%build
%if 0%{?fedora}
lrelease-qt6 common/resources/translations/*.ts
%{qmake_qt6} PREFIX=%{_prefix} qcma.pro CONFIG+="QT5_SUFFIX" QMAKE_CXXFLAGS="%{optflags}" QMAKE_CFLAGS="%{optflags}"
%else
lrelease6 common/resources/translations/*.ts
%{qmake6} PREFIX=%{_prefix} qcma.pro CONFIG+="QT5_SUFFIX" QMAKE_CXXFLAGS="%{optflags}" QMAKE_CFLAGS="%{optflags}"
%endif
%make_build

%install
%make_install INSTALL_ROOT=%{buildroot}
desktop-file-validate %{buildroot}%{_datadir}/applications/qcma.desktop

%files
%license COPYING
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor
%{_mandir}/man1/qcma.1.*


%changelog


%package cli
Summary: Content Manager Assistant for the PS Vita (headless)

%description cli
Headless version of Content Manager Assistant for the PS Vita.

%files cli
%{_bindir}/qcma_cli
%{_mandir}/man1/qcma_cli.1.*
