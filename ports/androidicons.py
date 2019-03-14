#!/usr/bin/env python

import subprocess # nosec

assets = [("ldpi"   , "120",  "36"),
          ("mdpi"   , "160",  "48"),
          ("hdpi"   , "240",  "72"),
          ("xhdpi"  , "320",  "96"),
          ("xxhdpi" , "480", "144"),
          ("xxxhdpi", "640", "192")]

for folder, dpi, size in assets:
    icon = '../StandAlone/share/android/res/drawable-{}/icon.png'.format(folder)
    subprocess.Popen(['convert',  # nosec
                      '-density', dpi,
                      '-resize', '{0}x{0}'.format(size),
                      '-background', 'none',
                      '../StandAlone/share/icons/hicolor/scalable/webcamoid.svg',
                      icon]).communicate()
    subprocess.Popen(['optipng',  # nosec
                      '-O7',
                      '-zw', '32k',
                      icon]).communicate()
