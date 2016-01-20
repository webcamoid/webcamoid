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

Url: https://github.com/hipersayanX/webcamoid
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-build
# AutoReqProv: no

%if %{defined fedora}
BuildRequires: fdupes
BuildRequires: gcc-c++
BuildRequires: qt5-qttools-devel
BuildRequires: ffmpeg-devel
BuildRequires: libv4l-devel

# Requires: ffmpeg-libs
# Requires: libv4l
%endif

%if %{defined suse_version}
BuildRequires: fdupes
BuildRequires: update-desktop-files
BuildRequires: libQt5Concurrent-devel
BuildRequires: libQt5Svg-devel
BuildRequires: libQt5OpenGL-devel
BuildRequires: libQt5Xml-devel
BuildRequires: libqt5-qttools
BuildRequires: ffmpeg-devel
BuildRequires: libv4l-devel

# Requires: libavcodec55
# Requires: libavdevice55
# Requires: libavfilter3
# Requires: libavformat55
# Requires: libavutil52
# Requires: libpostproc52
# Requires: libswresample0
# Requires: libswscale2
# Requires: libv4l1-0
# Requires: libv4l2-0
# Requires: libv4lconvert0
%endif

%if %{defined mgaversion}
BuildRequires: fdupes
BuildRequires: qttools5
BuildRequires: libv4l-devel

%ifarch i586
BuildRequires: libqt5widgets-devel
BuildRequires: libqt5concurrent-devel
BuildRequires: libqt5xml-devel
BuildRequires: libffmpeg-devel
%else
BuildRequires: lib64qt5widgets-devel
BuildRequires: lib64qt5concurrent-devel
BuildRequires: lib64qt5xml-devel
BuildRequires: lib64ffmpeg-devel
%endif

# Requires: libv4l0
#
# %ifarch i586
# Requires: libqt5widgets5
# Requires: libqt5concurrent5
# Requires: libqt5xml5
# Requires: libavcodec55
# Requires: libavformat55
# Requires: libavutil52
# Requires: libpostproc52
# Requires: libswscaler2
# Requires: libavfilter3
# Requires: libswresample0
# %else
# Requires: lib64qt5widgets5
# Requires: lib64qt5concurrent5
# Requires: lib64qt5xml5
# Requires: lib64avcodec55
# Requires: lib64avformat55
# Requires: lib64avutil52
# Requires: lib64postproc52
# Requires: lib64swscaler2
# Requires: lib64avfilter3
# Requires: lib64swresample0
# %endif
%endif

Conflicts: plasmoid-webcamoid

%description
Webcamoid is a full featured webcam capture application.

Features:

    * Take pictures and record videos with the webcam.
    * Manages multiple webcams.
    * Written in C++/Qt.
    * Custom controls for each webcam.
    * Add funny effects to the webcam.
    * +60 effects available.
    * Effects with live previews.
    * Translated to many languages.
    * Use custom network and local files as capture devices.
    * Capture from desktop.

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
%{_libdir}/qt5/qml/AkQml/
%{_libdir}/lib*AvKys.so*

%changelog
* Wed Jan 20 2016 Gonzalo Exequiel Pedone <hipersayan DOT x AT gmail DOT com> 7.0.0-1
- Final Release.
