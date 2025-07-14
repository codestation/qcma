# 
# qcma spec file
# 

%define _version 0.5.0

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
%define qmake_qt6 qmake-qt6
%define _pkgconfig pkg-config
%define _qt6linguist  qt6-tools-linguist
%define _qt6basedevel qt6-base-devel
%endif

Name:           qcma
Summary:        PSVita Content Manager Assistant
License:        GPL-3.0
Release:        1
Version:        %{_version}
URL:            https://github.com/codestation/qcma
Source:         https://github.com/codestation/qcma/archive/%{_verprefix}/qcma-%{_version}.tar.gz
Group:          Productivity/File utilities
Requires:       libvitamtp5 >= 2.5.9
BuildRequires:  gcc-c++ 
BuildRequires:  %{_pkgconfig}
BuildRequires:  libnotify-devel
BuildRequires:  libvitamtp-devel
BuildRequires:  %{_qt6linguist}
BuildRequires:  %{_qt6basedevel}
BuildRequires:  desktop-file-utils
%if ! 0%{?fedora}
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
%setup -n %{name}-%{version}

%build
lrelease-qt6 common/resources/translations/*.ts
%if 0%{?fedora}
%{qmake_qt6} PREFIX=/usr qcma.pro CONFIG+="QT5_SUFFIX" CONFIG+="DISABLE_FFMPEG" QMAKE_CXXFLAGS="%{optflags}" QMAKE_CFLAGS="%{optflags}"
%else
%{qmake_qt6} PREFIX=/usr qcma.pro CONFIG+="QT5_SUFFIX" QMAKE_CXXFLAGS="%{optflags}" QMAKE_CFLAGS="%{optflags}"
%endif
make %{?_smp_mflags}

%install
make install INSTALL_ROOT=%{buildroot}
desktop-file-edit --remove-key=Categories --add-category=System --add-category=FileManager --remove-key=Path --set-icon=qcma %{buildroot}%{_prefix}/share/applications/qcma.desktop

%files
%defattr(-,root,root)
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
%{_prefix}/share/man/man1/qcma_cli.1.gz
