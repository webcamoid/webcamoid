Name: webcamoid
Version: 5.0.0b1
Release: 1%{?dist}
Summary: Webcamoid, the full webcam and multimedia suite.
Group: Applications/Multimedia
License: GPLv3+
URL: http://kde-apps.org/content/show.php/Webcamoid?content=144796
Source0: https://github.com/hipersayanX/Webcamoid/archive/v${version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: qt-devel
BuildRequires: kdelibs-devel
BuildRequires: ffmpeg-devel
BuildRequires: bison
BuildRequires: flex
BuildRequires: frei0r-devel
BuildRequires: qimageblitz-devel
Requires: qt
Requires: kdelibs
Requires: ffmpeg-libs
Requires: frei0r-plugins
Requires: qimageblitz

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
%setup -q -n Webcamoid

%build
qmake-qt4 Webcamoid.pro \
    PREFIX="%{buildroot}/usr" \
    INSTALLPREFIX=/usr \
    INSTALLKDEINCLUDEDIR=%{_includedir}/kde4/KDE \
    LIBDIR=%{_libdir}

make

%install
rm -rf %{buildroot}
make install

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/webcamoid
%{_includedir}/Qb/qb.h
%{_includedir}/Qb/qbapplication.h
%{_includedir}/Qb/qbcaps.h
%{_includedir}/Qb/qbelement.h
%{_includedir}/Qb/qbfrac.h
%{_includedir}/Qb/qbpacket.h
%{_includedir}/Qb/qbplugin.h
%{_libdir}/Qb/libACapsConvert.so
%{_libdir}/Qb/libAging.so
%{_libdir}/Qb/libBin.so
%{_libdir}/Qb/libBlitzer.so
%{_libdir}/Qb/libFilter.so
%{_libdir}/Qb/libFire.so
%{_libdir}/Qb/libFrei0r.so
%{_libdir}/Qb/libMatrix.so
%{_libdir}/Qb/libMultiSink.so
%{_libdir}/Qb/libMultiSrc.so
%{_libdir}/Qb/libMultiplex.so
%{_libdir}/Qb/libProbe.so
%{_libdir}/Qb/libQImageConvert.so
%{_libdir}/Qb/libSync.so
%{_libdir}/Qb/libVCapsConvert.so
%{_libdir}/Qb/libWarhol.so
%{_libdir}/kde4/plasma_applet_webcamoid.so
%{_libdir}/libQb.so.5.0.0
%{_libdir}/libWebcamoid.so.5.0.0
%{_datadir}/applications/kde4/webcamoid.desktop
%{_defaultdocdir}/Qb/html/qb.index
%{_defaultdocdir}/Qb/html/qb.pageindex
%{_defaultdocdir}/Qb/html/qt-dita-map.xml
%{_defaultdocdir}/webcamoid/html/qt-dita-map.xml
%{_defaultdocdir}/webcamoid/html/webcamoid.index
%{_defaultdocdir}/webcamoid/html/webcamoid.pageindex
%{_datadir}/kde4/services/plasma-applet-webcamoid.desktop
%{_datadir}/licenses/webcamoid/COPYING
%{_datadir}/webcamoid/tr/ca.qm
%{_datadir}/webcamoid/tr/de.qm
%{_datadir}/webcamoid/tr/el.qm
%{_datadir}/webcamoid/tr/es.qm
%{_datadir}/webcamoid/tr/fr.qm
%{_datadir}/webcamoid/tr/gl.qm
%{_datadir}/webcamoid/tr/it.qm
%{_datadir}/webcamoid/tr/ja.qm
%{_datadir}/webcamoid/tr/ko.qm
%{_datadir}/webcamoid/tr/pt.qm
%{_datadir}/webcamoid/tr/ru.qm
%{_datadir}/webcamoid/tr/zh_CN.qm
%{_datadir}/webcamoid/tr/zh_TW.qm

%changelog
* Wed May 22 2013 Gonzalo Exequiel Pedone <hipersayan DOT x AT gmail DOT com> 5.0.0b1-1
- Initial release for Fedora 18
