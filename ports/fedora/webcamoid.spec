Name: webcamoid
Version: 5.0.0b2
Release: 1%{?dist}
Summary: Webcamoid, the full webcam and multimedia suite.
Group: Applications/Multimedia
License: GPLv3+
URL: http://kde-apps.org/content/show.php/Webcamoid?content=144796
Source0: webcamoid_%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-build

%if %{defined fedora}
BuildRequires: bison
BuildRequires: bison-devel
BuildRequires: flex
BuildRequires: flex-devel
BuildRequires: wget
BuildRequires: yasm
BuildRequires: libass-devel
BuildRequires: libbluray-devel
BuildRequires: gsm-devel
BuildRequires: libmodplug-devel
BuildRequires: openjpeg-devel
BuildRequires: opus-devel
BuildRequires: pulseaudio-libs-devel
BuildRequires: schroedinger-devel
BuildRequires: speex-devel
BuildRequires: libtheora-devel
BuildRequires: libv4l-devel
BuildRequires: libvorbis-devel
BuildRequires: libvpx-devel
BuildRequires: qt-devel
BuildRequires: kdelibs-devel
BuildRequires: frei0r-devel
BuildRequires: qimageblitz-devel

Requires: qt
Requires: kdelibs
Requires: frei0r-plugins
Requires: qimageblitz
%endif

%if %{defined suse_version}
BuildRequires: bison
BuildRequires: flex
BuildRequires: wget
BuildRequires: make
BuildRequires: yasm
BuildRequires: libass-devel
BuildRequires: libbluray-devel
BuildRequires: libgsm-devel
BuildRequires: libmodplug-devel
BuildRequires: openjpeg-devel
BuildRequires: libopus-devel
BuildRequires: libpulse-devel
BuildRequires: schroedinger-devel
BuildRequires: libspeex-devel
BuildRequires: libtheora-devel
BuildRequires: libv4l-devel
BuildRequires: libvorbis-devel
BuildRequires: libvpx-devel
BuildRequires: libqt4-devel
BuildRequires: libkde4-devel
BuildRequires: frei0r-plugins-devel
BuildRequires: libqimageblitz-devel

Requires: libqt4
Requires: libkdecore4
Requires: frei0r-plugins
Requires: libqimageblitz4
%endif

%description
Webcamoid is a webcam plasmoid for the KDE desktop environment.

Features:

    * Take pictures with the webcam.
    * Record videos.
    * Manages multiple webcams.
    * Play/Stop capture, this saves resources while the plasmoid is not in use.
    * Written in C++.
    * 100% Qt based software, for KDE/Qt purists.
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
%setup -q -n %{name}_%{version}

%build
%if %{defined fedora}
qmake-qt4 Webcamoid.pro \
    LIBDIR=%{_libdir} \
    USE3DPARTYLIBS=1 \
    KDEINCLUDEDIR=/usr/include/kde4 \
    KDELIBDIR=/usr/lib/kde4/devel
%endif

%if %{defined suse_version}
qmake Webcamoid.pro \
    LIBDIR=%{_libdir} \
    USE3DPARTYLIBS=1 \
    QMAKE_LRELEASE=/usr/bin/lrelease
%endif

make

%install
rm -rf %{buildroot}
make INSTALL_ROOT="%{buildroot}" install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/webcamoid
%{_datadir}/applications/kde4/webcamoid.desktop
%{_datadir}/kde4/services/plasma-applet-webcamoid.desktop
%{_datadir}/licenses/webcamoid/COPYING
%{_datadir}/webcamoid/
%{_includedir}/Qb/
%{_libdir}/Qb
%{_libdir}/kde4/plasma_applet_webcamoid.so
%{_libdir}/libQb.so
%{_libdir}/libQb.so.5
%{_libdir}/libQb.so.5.0
%{_libdir}/libQb.so.5.0.0
%{_libdir}/libWebcamoid.so
%{_libdir}/libWebcamoid.so.5
%{_libdir}/libWebcamoid.so.5.0
%{_libdir}/libWebcamoid.so.5.0.0

%changelog
* Wed Oct 16 2013 Gonzalo Exequiel Pedone <hipersayan DOT x AT gmail DOT com> 5.0.0b2-1
- Working with OBS.
