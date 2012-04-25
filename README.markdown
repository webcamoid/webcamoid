# Webcamoid, Show and take Photos with your webcam #

[Webcamoid](http://kde-apps.org/content/show.php/Webcamoid?content=144796) is a plasmoid for the KDE desktop environment.

## Features ##

* Take pictures with the webcam.
* Manages multiple webcams.
* Play/Stop capture, this saves resources while the plasmoid is not in use.
* Written in Python.
* Requires FFmpeg to capture images from the webcam.
* Custom controls for each webcam.
* Popup applet support (you can embed Webcamoid in the panel).

## Installing ##

Webcamoid requires the FFmpeg executable, you can install it with:

* __Arch/Chakra__: _pacman -S ffmpeg_
* __Debian/Ubuntu__: _apt-get install ffmpeg_
* __Fedora__: _yum install ffmpeg_
* __OpenSuSE__: _zypper install ffmpeg_
* __Mandriva__: _urpmi ffmpeg_
* __Pardus__: _pisi it ffmpeg_

You can follow [these](http://userbase.kde.org/Plasma/Installing_Plasmoids) instructions to install Webcamoid. Alternatively, you can install/uninstall Webcamoid from console:

__install__

	plasmapkg -i webcamoid.plasmoid && kbuildsycoca4

__uninstall__

	plasmapkg -r webcamoid && kbuildsycoca4

## Installing from Github ##

Be careful, the GIT repository is unstable, it could work or not.

    git clone https://github.com/hipersayanX/Webcamoid.git
    zip -r webcamoid.plasmoid Webcamoid
    plasmapkg -i webcamoid.plasmoid && kbuildsycoca4

## Translating ##

In the root directory of the project run:

    pylupdate4 Webcamoid.pro

Then a _contents/ts/lang.ts_ will be created. Translate that file to your language using Qt Linguist, and change _lang.ts_ to your code language, for instance, if you speak Spanish call it _es.ts_. Then run the following command:

    lrelease contents/ts/lang.ts
