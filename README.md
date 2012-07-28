# Webcamoid, Show and take Photos with your webcam #

[Webcamoid](http://kde-apps.org/content/show.php/Webcamoid?content=144796) is a plasmoid for the KDE desktop environment.

## Features ##

* Take pictures with the webcam.
* Record videos.
* Manages multiple webcams.
* Play/Stop capture, this saves resources while the plasmoid is not in use.
* Written in Python.
* Requires GStreamer to capture images from the webcam.
* Custom controls for each webcam.
* Popup applet support (you can embed Webcamoid in the panel).
* Add funny effects to the webcam (requires GStreamer plugins).
* Translated to many languages.

## Installing ##

Webcamoid requires the _gst-launch-0.10_ executable (GStreamer), you can install it with:

* __Arch/Chakra/Parabola__: _pacman -S gstreamer0.10 gstreamer0.10-good gstreamer0.10-bad_
* __Debian/Ubuntu__: _apt-get install gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad_
* __Fedora/CentOS__: _yum install gstreamer gstreamer-plugins-good gstreamer-plugins-bad-free_
* __OpenSuSE__: _zypper install gstreamer-0_10-utils gstreamer-0_10-plugins-good gstreamer-0_10-plugins-bad_
* __Mandriva/Mageia__: _urpmi gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad_

You can follow [these](http://userbase.kde.org/Plasma/Installing_Plasmoids) instructions to install Webcamoid. Alternatively, you can install/uninstall Webcamoid from console:

__install__

	plasmapkg -i webcamoid.plasmoid && kbuildsycoca4

__uninstall__

	plasmapkg -r webcamoid && kbuildsycoca4

## Installing from Github ##

Be careful, the GIT repository is unstable, it could work or not.

    git clone https://github.com/hipersayanX/Webcamoid.git
    zip -r webcamoid.plasmoid Webcamoid -x \*.git\*
    plasmapkg -i webcamoid.plasmoid && kbuildsycoca4

## Translating ##

Edit the _Webcamoid.pro_ file and add you language code to the _TRANSLATIONS_ macro, then, in the root directory of the project run:

    pylupdate4 -verbose -noobsolete Webcamoid.pro

A _contents/ts/your_lang_code.ts_ will be created. Translate that file to your language using Qt Linguist. Then run the following command:

    lrelease -removeidentical Webcamoid.pro

### Supported languages ###

Some languages are officially supported and other are supported through [Google Translator](http://translate.google.com/).

* Catalan
* Chinese Simplified
* Chinese Traditional
* English (official)
* French
* Galician
* German
* Greek
* Italian
* Japanese (official)
* Korean
* Portuguese
* Russian
* Spanish (official)

## Adding distro detection support ##

If you had no installed the required GStreamer packages, Webcamoid will show you a dialog telling what packages you must install. The problem is that there are a lot of GNU/Linux distros arround the world, and I can't waste my time testing one-by-one each of these.  
If you want support for your distro distro, please follow these steps:

### For newest distributions ###

Since February of 2012 the /etc/os-release file becomes the standard way of detecting the current working distribution. See more at [this link](http://www.freedesktop.org/software/systemd/man/os-release.html).

Give me the output of this command:

    cat /etc/os-release

If your distro doesn't contains this file please go to de next step.

### For oldest distributions ###

Give the output of this command:

    python2 -c 'import platform; print(platform.linux_distribution())'

Also, you must find wich packages contains these files:

    /usr/bin/gst-launch-0.10
    /usr/lib/gstreamer-0.10/libgstcoreelements.so
    /usr/lib/gstreamer-0.10/libgstalsa.so
    /usr/lib/gstreamer-0.10/libgstaudioconvert.so
    /usr/lib/gstreamer-0.10/libgstffmpegcolorspace.so
    /usr/lib/gstreamer-0.10/libgsttheora.so
    /usr/lib/gstreamer-0.10/libgstvorbis.so
    /usr/lib/gstreamer-0.10/libgsteffectv.so
    /usr/lib/gstreamer-0.10/libgstvideo4linux2.so
    /usr/lib/gstreamer-0.10/libgstgaudieffects.so
    /usr/lib/gstreamer-0.10/libgstgeometrictransform.so
    /usr/lib/gstreamer-0.10/libgstvp8.so

Please provide all this info in the [issues page](http://github.com/hipersayanX/Webcamoid/issues).
