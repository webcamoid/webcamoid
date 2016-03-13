%if %{defined mgaversion}
%define debug_package %{nil}
%endif

Name: webcamoid
Version: 7.0.0
Release: 1%{?dist}
Summary: The full webcam and multimedia suite

%if %{defined fedora}
Group: Applications/Multimedia
License: GPL
%endif

%if %{defined suse_version}
Group: Productivity/Multimedia/Video/Players
License: GPL-3.0+
%endif

%if %{defined mgaversion}
Group: Video/Utilities
License: GPLv3+
%endif

Url: https://webcamoid.github.io/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-build
# AutoReqProv: no

%if %{defined fedora}
Requires: v4l2loopback

BuildRequires: fdupes
BuildRequires: gcc-c++
BuildRequires: qt5-qttools-devel
BuildRequires: qt5-qtdeclarative-devel
BuildRequires: qt5-qtsvg-devel
BuildRequires: ffmpeg-devel
BuildRequires: libv4l-devel
BuildRequires: pulseaudio-libs-devel
%endif

%if %{defined suse_version}
Requires: v4l2loopback-utils

BuildRequires: fdupes
BuildRequires: update-desktop-files
BuildRequires: libqt5-linguist
BuildRequires: libqt5-qtbase-devel
BuildRequires: libqt5-qtdeclarative-devel
BuildRequires: libqt5-qtsvg-devel
BuildRequires: ffmpeg-devel
BuildRequires: libv4l-devel
BuildRequires: libpulse-devel
%endif

%if %{defined mgaversion}
Requires: v4l2loopback

BuildRequires: fdupes
BuildRequires: qttools5

%ifarch i586
BuildRequires: libqt5qml-devel
BuildRequires: libqt5svg-devel
BuildRequires: libffmpeg-devel
BuildRequires: libv4l-devel
BuildRequires: libpulseaudio-devel
%else
BuildRequires: lib64qt5qml-devel
BuildRequires: lib64qt5svg-devel
BuildRequires: lib64ffmpeg-devel
BuildRequires: lib64v4l-devel
BuildRequires: lib64pulseaudio-devel
%endif
%endif

Conflicts: plasmoid-webcamoid

%description
Webcamoid is a full featured webcam capture application.

Features:

    * Cross-platform (GNU/Linux, Windows)
    * Take pictures and record videos with the webcam.
    * Manages multiple webcams.
    * Written in C++/Qt.
    * Custom controls for each webcam.
    * Add funny effects to the webcam.
    * +60 effects available.
    * Translated to many languages.
    * Use custom network and local files as capture devices.
    * Capture from desktop.
    * Many recording formats.
    * Virtual webcam support for feeding other programs.

%prep
%setup -q -n %{name}-%{version}
# delete not needed files
find . -name ".gitignore" -exec rm {} \;

%build
%if %{defined fedora}
qmake-qt5 Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid
%endif

%if %{defined suse_version}
qmake-qt5 Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid
%endif

%if %{defined mgaversion}
%{_libdir}/qt5/bin/qmake Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid
%endif

make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make INSTALL_ROOT="%{buildroot}" install

%fdupes %{buildroot}

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/webcamoid
%{_datadir}/applications/webcamoid.desktop
#%{_datadir}/webcamoid/
%{_defaultdocdir}/webcamoid/
%{_libdir}/AvKys/
%{_libdir}/qt5/qml/AkQml
%{_libdir}/qt5/qml/AkQml/libAkQml.so
%{_libdir}/qt5/qml/AkQml/qmldir
%{_libdir}/lib*AvKys.so*

%changelog
* Tue Feb 23 2016 Gonzalo Exequiel Pedone <hipersayan DOT x AT gmail DOT com> 7.0.0-1
- Final Release.
