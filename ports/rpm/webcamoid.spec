Name: webcamoid
Version: 5.0.0
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

URL: https://github.com/hipersayanX/Webcamoid
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-build
AutoReqProv: no

%if %{defined fedora}
BuildRequires: fdupes
BuildRequires: bison
BuildRequires: bison-devel
BuildRequires: flex
BuildRequires: flex-devel
BuildRequires: make
BuildRequires: gcc-c++
BuildRequires: qt-devel
BuildRequires: kdelibs-devel
BuildRequires: frei0r-devel
BuildRequires: ffmpeg-devel
BuildRequires: libv4l-devel

Requires: qt-x11
Requires: kdelibs
Requires: frei0r-plugins
Requires: frei0r-plugins-opencv
Requires: ffmpeg-libs
Requires: libv4l
%endif

%if %{defined suse_version}
BuildRequires: fdupes
BuildRequires: bison
BuildRequires: flex
BuildRequires: make
BuildRequires: libqt4-devel
BuildRequires: libkde4-devel
BuildRequires: frei0r-plugins-devel
BuildRequires: libffmpeg-devel
BuildRequires: libv4l-devel

Requires: libqt4
Requires: kdelibs4
Requires: frei0r-plugins
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

%if %{defined mgaversion}
BuildRequires: fdupes
BuildRequires: bison
BuildRequires: flex
BuildRequires: libqt4-devel
BuildRequires: kdelibs4-core
BuildRequires: kdelibs4-devel
BuildRequires: wget
BuildRequires: yasm
BuildRequires: libass-devel
BuildRequires: libbluray-devel
BuildRequires: libgsm-devel
BuildRequires: libmodplug-devel
BuildRequires: liblame-devel
BuildRequires: librtmp-devel
BuildRequires: libschroedinger-devel
BuildRequires: libspeex-devel
BuildRequires: libtheora-devel
BuildRequires: libv4l-devel
BuildRequires: libvorbis-devel
BuildRequires: libvpx-devel
BuildRequires: libx264-devel

%ifarch i586
BuildRequires: libbison-static-devel
BuildRequires: libopencore-amr-devel
BuildRequires: libopenjpeg-devel
BuildRequires: libopus-devel
BuildRequires: libxvidcore-devel
%else
BuildRequires: lib64bison-static-devel
BuildRequires: lib64opencore-amr-devel
BuildRequires: lib64openjpeg-devel
BuildRequires: lib64opus-devel
BuildRequires: lib64xvidcore-devel
%endif

Requires: libqtgui4
Requires: libkdeui5
Requires: libass4
Requires: libbluray1
Requires: libgsm1
Requires: libmodplug1
Requires: liblame0
Requires: librtmp0
Requires: libschroedinger1.0_0
Requires: libspeex1
Requires: libtheora0
Requires: libtheoradec1
Requires: libtheoraenc1
Requires: libv4l0
Requires: libvorbis0
Requires: libvorbisenc2
Requires: libvorbisfile3
Requires: libvpx1
Requires: libx264_133

%ifarch i586
Requires: libopencore-amr0
Requires: libopenjpeg5
Requires: libopus0
Requires: libxvidcore4
%else
Requires: lib64opencore-amr0
Requires: lib64openjpeg5
Requires: lib64opus0
Requires: lib64xvidcore4
%endif
%endif

%description
Webcamoid is a full featured webcam capture application.

Features:

    * Take pictures and record videos with the webcam.
    * Manages multiple webcams.
    * Written in C++/Qt.
    * Custom controls for each webcam.
    * Add funny effects to the webcam (requires Frei0r plugins).
    * +50 effects available.
    * Effects with live previews.
    * Translated to many languages.
    * Provides a nice plasmoid for KDE desktop.
    * Use custom network and local files as capture devices.
    * Capture from desktop.

%prep
%setup -q -n %{name}-%{version}

%build
%if %{defined fedora}
qmake-qt4 Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid \
    KDEINCLUDEDIR=%{_includedir}/kde4 \
    KDELIBDIR=%{_libdir}/kde4/devel \
%endif

%if %{defined suse_version}
qmake Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid \
    QMAKE_LRELEASE=%{_bindir}/lrelease
%endif

%if %{defined mgaversion}
qmake Webcamoid.pro \
    LIBDIR=%{_libdir} \
    LICENSEDIR=%{_defaultdocdir}/webcamoid \
    QMAKE_LRELEASE=%{_libdir}/qt4/bin/lrelease \
    USE3DPARTYLIBS=1
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
* Wed Aug 6 2014 Gonzalo Exequiel Pedone <hipersayan DOT x AT gmail DOT com> 5.0.0-1
- Final Release.
