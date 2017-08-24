/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "screendev.h"

ScreenDev::ScreenDev(QObject *parent):
    QObject(parent)
{
}

ScreenDev::~ScreenDev()
{
}

AkFrac ScreenDev::fps() const
{
    return AkFrac();
}

QStringList ScreenDev::medias()
{
    return QStringList();
}

QString ScreenDev::media() const
{
    return QString();
}

QList<int> ScreenDev::streams() const
{
    return QList<int>();
}

int ScreenDev::defaultStream(const QString &mimeType)
{
    Q_UNUSED(mimeType)

    return -1;
}

QString ScreenDev::description(const QString &media)
{
    Q_UNUSED(media)

    return QString();
}

AkCaps ScreenDev::caps(int stream)
{
    Q_UNUSED(stream)

    return AkCaps();
}

void ScreenDev::setFps(const AkFrac &fps)
{
    Q_UNUSED(fps)
}

void ScreenDev::resetFps()
{
}

void ScreenDev::setMedia(const QString &media)
{
    Q_UNUSED(media)
}

void ScreenDev::resetMedia()
{
}

void ScreenDev::setStreams(const QList<int> &streams)
{
    Q_UNUSED(streams)
}

void ScreenDev::resetStreams()
{

}

bool ScreenDev::init()
{
    return false;
}

bool ScreenDev::uninit()
{
    return true;
}
