/* Webcamoid, camera capture application.
 * Copyright (C) 2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <QMutex>
#include <qrgb.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "screendev.h"

#define VALUE_SHIFT 8

class ScreenDevPrivate
{
    public:
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_rgba, 0, 0, {}}};
};

ScreenDev::ScreenDev(QObject *parent):
    QObject(parent)
{
    this->d = new ScreenDevPrivate;
}

ScreenDev::~ScreenDev()
{
    delete this->d;
}

#include "moc_screendev.cpp"
