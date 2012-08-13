# Webcamod, webcam capture plasmoid.
# Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
#
# Webcamod is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamod is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
#
# Email     : hipersayan DOT x AT gmail DOT com
# Web-Site 1: http://github.com/hipersayanX/Webcamoid
# Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796

SOURCES = \
    contents/code/about.py \
    contents/code/config.py \
    contents/code/effects.py \
    contents/code/infotools.py \
    contents/code/main.py \
    contents/code/translator.py \
    contents/code/upload.py \
    contents/code/v4l2tools.py \
    contents/code/videorecordconfig.py \
    contents/code/webcamoidgui.py

FORMS = \
    contents/ui/about.ui \
    contents/ui/effects.ui \
    contents/ui/videorecordconfig.ui \
    contents/ui/webcamoidgui.ui

# http://www.loc.gov/standards/iso639-2/php/code_list.php

TRANSLATIONS = \
    contents/ts/ca.ts \
    contents/ts/de.ts \
    contents/ts/el.ts \
    contents/ts/es.ts \
    contents/ts/fr.ts \
    contents/ts/gl.ts \
    contents/ts/it.ts \
    contents/ts/ja.ts \
    contents/ts/ko.ts \
    contents/ts/pt.ts \
    contents/ts/ru.ts \
    contents/ts/zh_CN.ts \
    contents/ts/zh_TW.ts

CODECFORTR = UTF-8
CODECFORSRC = UTF-8
