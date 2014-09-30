# Webcamoid, webcam capture application #

[Webcamoid](http://kde-apps.org/content/show.php/Webcamoid?content=144796) is a full featured webcam capture application.

## Features ##

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

## Build and install ##

Visit the [wiki](https://github.com/hipersayanX/Webcamoid/wiki) for compile and install instructions.

### Precompiled Packages ###

You can download precompiled packages for Arch, Debian, OpenSUSE and Ubuntu from my [OwnDrive](https://my.owndrive.com/public.php?service=files&t=11c46708181f96b4ec052ae74b7b8bef)

### Custom Streams ###

Webcamoid is not just a webcam capture software. You can, for example, use it to capture from a video, image file, IP cameras, or the desktop. 
If you want to add a new stream, go to the "Configure Custom Streams" option, and click in the "Add" button. Give it a human readable "Device Name" and write the path to the stream in the "URI" field. Here are some examples:

* file:///home/YourHome/Videos/SomeFunnyVideo.ogv
* http://1.2.3.4:1234/mjpg/video.mjpg
* http://www.somesite.com/mjpg/video.mjpg
* rtsp://user:password@livecamera.somesite.com/mpeg4

You can search some online IP cameras [here](http://www.google.com/search?q=filetype:mjpg) and [here](http://www.google.com/search?q=rtsp+ip+cameras+demo)

## Translating ##

Edit the _Lib.pro_ file and add you language code to the _TRANSLATIONS_ macro, then, in the root directory of the project run:

    lupdate-qt5 -verbose -noobsolete Webcamoid.pro

A _share/ts/your_lang_code.ts_ will be created. Translate that file to your language using Qt Linguist.

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
* Italian (official)
* Japanese (official)
* Korean
* Portuguese
* Russian
* Spanish (official)

## Reporting Bugs ##

Use Webcamoid until an abnormal event happen, write a list with the following information:

1. Your Webcamoid version.
2. Your distribution name and version (numeric, please).
3. Your KDE and Qt version.
4. Your GCC version.
5. The output of the previous command.
6. Any other useful information.

For debuging the plasmoid, run this command:

    plasma-windowed webcamoid

Send this list to the [issues page](http://github.com/hipersayanX/Webcamoid/issues).

## Donate ##

Can you share some BTC with me to keep alive the project? :)

[Donate Now](https://blockchain.info/address/1Gj7THPrfrXFQ5BzVSVmRu2GxvwzYzbacj)
