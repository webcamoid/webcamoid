#!/usr/bin/env python

# Webcamoid, webcam capture application.
# Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

import os
import socks
import urllib.request
import mimetypes
import json
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom
import socket
import magic

defaultAvatar = ''
userAgent = 'Mozilla/5.0 (X11; Linux x86_64; rv:52.0) Gecko/20100101 Firefox/52.0'
useProxy = False

def addResource(qresource, resource):
    hasResource = False

    for fentry in qresource.findall('file'):
        if fentry.text == resource:
            hasResource = True

            break

    if not hasResource:
        resourceFile = ET.SubElement(qresource, 'file')
        resourceFile.text = resource

def createQRC(where):
    tree = ET.ElementTree()

    if os.path.exists(where):
        tree.parse(where)

    root = tree.getroot()

    if not root:
        root = ET.Element('RCC')
        tree._setroot(root)

    qresources = root.findall('qresource')

    if len(qresources) > 0:
        qresource = qresources[0]
    else:
        qresource = ET.SubElement(root,
                                'qresource',
                                {'prefix': '/Webcamoid/Contributors'})

    addResource(qresource, 'contributors.json')

    return root, qresource

def writeQRC(where, root):
    fdir = os.path.dirname(where)

    if not os.path.exists(fdir):
        os.makedirs(fdir)

    with open(where, 'w') as f:
        dom = minidom.parseString(ET.tostring(root, 'utf-8'))

        for line in dom.toprettyxml(indent=4 * " ").split('\n')[1:]:
            if line.strip() != '':
                f.write('{}\n'.format(line))

if useProxy:
    socks.set_default_proxy(socks.SOCKS5, "localhost", 9050)
    socket.socket = socks.socksocket

# Reatrieve contributors info.
with urllib.request.urlopen(urllib.request.Request('https://api.github.com/users/octocat',
                                                   data=None,
                                                   headers={'User-Agent': userAgent})) as urldata:
    userdata = json.load(urldata)
    defaultAvatar = userdata["avatar_url"]

with urllib.request.urlopen(urllib.request.Request('https://api.github.com/repos/webcamoid/webcamoid/contributors?anon=1',
                                                   data=None,
                                                   headers={'User-Agent': userAgent})) as urldata:
    data = json.load(urldata)

contributors = []

for user in data:
    if 'login' in user and user['login'] == 'hipersayanX':
        continue

    login = ''
    name = ''
    avatar = ''
    website = ''

    if user['type'] == 'Anonymous':
        login = 'octocat'
        name = user['name']
        avatar = defaultAvatar
        website = 'mailto:{}'.format(user['email'])
    else:
        with urllib.request.urlopen(urllib.request.Request('https://api.github.com/users/{}'.format(user['login']),
                                                           data=None,
                                                           headers={'User-Agent': userAgent})) as userurldata:
            userdata = json.load(userurldata)
            login = user['login']
            name = userdata['name'] if userdata['name'] else user['login']
            avatar = userdata['avatar_url']
            website = userdata['blog'] if userdata['blog'] and userdata['blog'] != '' else 'https://github.com/{}'.format(user['login'])

    contributors.append({'name': name,
                         'login': login,
                         'avatar': avatar,
                         'website': website})

contributors = sorted(contributors, key=lambda contributor: contributor['name'])

mimetypes.init()
rootdir = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))

# Create QRC file and fill basic tree.
rccFile = os.path.join(rootdir, 'StandAlone/share/contributors/contributors.qrc')
root, qresource = createQRC(rccFile)
_contributors = []

# Read avatars from github and fill QRC file.
for contributor in contributors:
    print(contributor)

    with urllib.request.urlopen(urllib.request.Request(contributor['avatar'],
                                                       data=None,
                                                       headers={'User-Agent': userAgent})) as avatarurl:
        fdata = avatarurl.read()
        finfo = magic.detect_from_content(fdata)
        ext = mimetypes.guess_extension(finfo.mime_type).replace('.jpe', '.jpg')

        if ext != '':
            fname = '{}{}'.format(contributor['login'], ext)
            fdir = os.path.join(rootdir, 'StandAlone/share/contributors/avatars')
            fpath = os.path.join(fdir, fname)
            resource = 'avatars/{}'.format(fname)
            addResource(qresource, resource)
            _contributor = contributor.copy()
            _contributor['avatar'] = resource
            _contributors.append(_contributor)

            if not os.path.exists(fpath):
                if not os.path.exists(fdir):
                    os.makedirs(fdir)

                with open(fpath, 'wb') as f:
                    f.write(fdata)

jsonFile = os.path.join(rootdir, 'StandAlone/share/contributors/contributors.json')

with open(jsonFile, 'w') as f:
    json.dump(_contributors, f, sort_keys=True, indent=4)

# Write QRC file.
writeQRC(rccFile, root)
