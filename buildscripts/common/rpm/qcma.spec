# 
# qcma spec file
# 

%define _version 0.5.4

%if "%{_version}" == "testing" || "%{_version}" == "master"
%define _verprefix %{_version}
%else
%define _verprefix v%{_version}
%endif

%if 0%{?fedora}
%define _pkgconfig pkgconfig
%define _qt6linguist qt6-linguist
%define _qt6basedevel qt6-qtbase-devel
%define _qt6toolsdevel qt6-qttools-devel
%define _ninja ninja-build
%else
%define _pkgconfig pkg-config
%define _qt6linguist  qt6-tools-linguist
%define _qt6basedevel qt6-base-devel
%define _qt6toolsdevel qt6-linguist-devel
%define _ninja ninja
%endif

Name:           qcma
Summary:        PSVita Content Manager Assistant
License:        GPL-3.0-or-later
Release:        1
Version:        %{_version}
URL:            https://github.com/codestation/qcma
Source:         https://github.com/codestation/qcma/archive/%{_verprefix}/qcma-%{_version}.tar.gz
Group:          Productivity/File utilities
Requires:       libvitamtp5 >= 2.5.12
BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.21
BuildRequires:  %{_ninja}
BuildRequires:  %{_pkgconfig}
BuildRequires:  libnotify-devel
BuildRequires:  libvitamtp-devel >= 2.5.12
BuildRequires:  %{_qt6linguist}
BuildRequires:  %{_qt6basedevel}
BuildRequires:  %{_qt6toolsdevel}
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
%cmake -G Ninja -DBUILD_TESTING=ON
%if 0%{?fedora}
%cmake_build
%ctest
%else
cmake --build . --parallel %{_smp_build_ncpus}
ctest --test-dir . --output-on-failure
%endif

%install
%if 0%{?fedora}
%cmake_install
%else
DESTDIR=%{buildroot} cmake --install build
%endif
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
