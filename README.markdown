# Webcamoid, Show and take Photos with your webcam #

[Webcamoid](http://kde-apps.org/content/show.php/Webcamoid?content=144796) is a plasmoid for the KDE desktop environment.

## Features ##

* Take pictures with the webcam.
* Manages multiple webcams.
* Play/Stop capture, this saves resources while the plasmoid is not in use.
* Written in Python.
* Requires FFmpeg to capture images from the webcam.
* Custom controls for each webcam.

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
