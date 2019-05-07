#!/usr/bin/env python

# Webcamoid, webcam capture application.
# Copyright (C) 2017  Gonzalo Exequiel Pedone
#
# Webcamoid is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Webcamoid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
#
# Web-Site: http://webcamoid.github.io/

import subprocess # nosec
import sys


process = subprocess.Popen(['git', 'shortlog', '--summary', '-e'], # nosec
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
stdout, _ = process.communicate()
contributors = stdout.decode(sys.getdefaultencoding()).split('\n')
contributors_list = []

exclude = ['hipersayan.x@gmail.com',
           'noreply@weblate.org',
           'hosted@weblate.org']

for contributor in contributors:
    contributor = ' '.join(contributor.split()[1:])

    if len(contributor) > 0:
        mail = contributor[contributor.rfind('<') + 1: ].replace('>', '')

        if not mail in exclude:
            contributors_list.append(contributor)

contributors_list = sorted(contributors_list, key=str.lower)

with open('../StandAlone/share/contributors.txt', 'w') as f:
    for contributor in contributors_list:
        f.write(contributor  + '\n')
