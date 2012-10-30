# Webcamoid, webcam capture plasmoid #

[Webcamoid](http://kde-apps.org/content/show.php/Webcamoid?content=144796) is a webcam plasmoid for the KDE desktop environment.

## Features ##

* Take pictures with the webcam.
* Record videos.
* Manages multiple webcams.
* Play/Stop capture, this saves resources while the plasmoid is not in use.
* Written in Python.
* Uses GStreamer as capture engine.
* Custom controls for each webcam.
* Popup applet support (you can embed Webcamoid in the panel).
* Add funny effects to the webcam (requires GStreamer plugins).
* +50 effects available.
* Effects with live previews.
* Translated to many languages.
* Stand alone installation mode (use it as a normal program).
* Use custom network and local files as capture devices.
* Capture from desktop.

## Installing ##

Webcamoid requires the _gst-launch-0.10_ executable (GStreamer), you can install it with:

* __Arch/Chakra/Parabola__: _pacman -S gstreamer0.10 gstreamer0.10-good gstreamer0.10-bad_
* __Debian/Ubuntu__: _apt-get install gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad_
* __Fedora/CentOS__: _yum install gstreamer gstreamer-plugins-good gstreamer-plugins-bad-free_
* __OpenSuSE__: _zypper install gstreamer-0_10-utils gstreamer-0_10-plugins-good gstreamer-0_10-plugins-bad_
* __Mandriva/Mageia__: _urpmi gstreamer0.10-tools gstreamer0.10-plugins-good gstreamer0.10-plugins-bad_

### The easy way ###

You can follow [these](http://userbase.kde.org/Plasma/Installing_Plasmoids) instructions to install Webcamoid. Alternatively, you can install/uninstall Webcamoid from console:

__install__

    plasmapkg -i webcamoid.plasmoid && kbuildsycoca4

__uninstall__

    plasmapkg -r webcamoid && kbuildsycoca4

### As stand alone program ###

This install mode is highly encouraged since it gives the best user experience.  
The stand alone installation mode allow you to use Webcamoid as a plasmoid, but also as a normal program. This installation way requires root access.

    unzip webcamoid.plasmoid
    cd Webcamoid
    su -c 'python2 setup.py install --prefix=/usr'
    kbuildsycoca4

### From Github ###

Be careful, the GIT repository is unstable, it could work or not.

    git clone https://github.com/hipersayanX/Webcamoid.git
    zip -r webcamoid.plasmoid Webcamoid -x \*.git\*
    plasmapkg -i webcamoid.plasmoid && kbuildsycoca4

### Custom Streams ###

Webcamoid is not just a webcam capture software. You can, for example, use it to capture from a video, image file, IP cameras, or the desktop.  
If you want to add a new stream, go to the "Configure Custom Streams" option, and click in the "Add" button. Give it a human readable "Device Name" and write the path to the stream in the "URI" field. Here are some examples:

* file:///home/YourHome/Videos/SomeFunnyVideo.ogv
* http://1.2.3.4:1234/mjpg/video.mjpg
* http://www.somesite.com/mjpg/video.mjpg
* rtsp://user:password@livecamera.somesite.com/mpeg4

You can search some online IP cameras [here](http://www.google.com/search?q=filetype:mjpg) and [here](http://www.google.com/search?q=rtsp+ip+cameras+demo)

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

## Reporting Bugs ##

In the root folder of webcamoid run this command:

    plasmoidviewer

use Webcamoid until an abnormal event happen, write a list with the following information:

1. Your Webcamoid version.
2. Your distribution name and version (numeric, please).
3. Your KDE and Qt version.
4. Your Python version.
5. The output of the previous command.
6. Any other useful information.

Send this list to the [issues page](http://github.com/hipersayanX/Webcamoid/issues).
