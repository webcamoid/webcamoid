Name: webcamoid
Version: 5.0.0b2
Release: 1%{?dist}
Summary: The full webcam and multimedia suite
Group: Productivity/Multimedia/Video/Players
License: GPL-3.0+
URL: http://kde-apps.org/content/show.php/Webcamoid?content=144796
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%if %{defined suse_version}
AutoReqProv: no

BuildRequires: fdupes
BuildRequires: bison
BuildRequires: flex
BuildRequires: make
BuildRequires: libqt4-devel
BuildRequires: libkde4-devel
BuildRequires: frei0r-plugins-devel
BuildRequires: libqimageblitz-devel
BuildRequires: libffmpeg-devel
BuildRequires: libv4l-devel

Requires: libqt4
Requires: kdelibs4
Requires: frei0r-plugins
Requires: libqimageblitz4
Requires: libavcodec55
Requires: libavdevice55
Requires: libavfilter3
Requires: libavformat55
Requires: libavresample1
Requires: libavutil52
Requires: libpostproc52
Requires: libswresample0
Requires: libswscale2
Requires: libv4l1-0
Requires: libv4l2-0
Requires: libv4lconvert0

%kde4_runtime_requires
%endif

%description
Webcamoid is a webcam plasmoid for the KDE desktop environment.

Features:

    * Take pictures with the webcam.
    * Record videos.
    * Manages multiple webcams.
    * Play/Stop capture, this saves resources while the plasmoid is not in use.
    * Written in C++.
    * 100%% Qt based software, for KDE/Qt purists.
    * Custom controls for each webcam.
    * Popup applet support (you can embed Webcamoid in the panel).
    * Add funny effects to the webcam (requires Frei0r plugins and QImageBlitz).
    * +50 effects available.
    * Effects with live previews.
    * Translated to many languages.
    * Stand alone installation mode (use it as a normal program).
    * Use custom network and local files as capture devices.
    * Capture from desktop.

%prep
%setup -q -n %{name}-%{version}

%build
%if %{defined suse_version}
qmake Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid \
    QMAKE_LRELEASE=/usr/bin/lrelease
%endif

make

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
%{_datadir}/applications/kde4/webcamoid.desktop
%{_datadir}/kde4/services/plasma-applet-webcamoid.desktop
%{_datadir}/webcamoid/
%{_defaultdocdir}/webcamoid/
%{_includedir}/Qb/
%{_libdir}/Qb/
%{_libdir}/kde4/plasma_applet_webcamoid.so
%{_libdir}/lib*Qb.so*
%{_libdir}/libWebcamoid.so*

%changelog
* Wed Oct 16 2013 Gonzalo Exequiel Pedone <hipersayan DOT x AT gmail DOT com> 5.0.0b2-1
- Working with OBS.
