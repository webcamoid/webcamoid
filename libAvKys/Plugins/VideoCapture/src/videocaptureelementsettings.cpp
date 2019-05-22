/* Webcamoid, webcam capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#include "videocaptureelementsettings.h"
#include "videocaptureglobals.h"

Q_GLOBAL_STATIC(VideoCaptureGlobals, globalVideoCapture)

VideoCaptureElementSettings::VideoCaptureElementSettings(QObject *parent):
    QObject(parent)
{
    QObject::connect(globalVideoCapture,
                     &VideoCaptureGlobals::codecLibChanged,
                     this,
                     &VideoCaptureElementSettings::codecLibChanged);
    QObject::connect(globalVideoCapture,
                     &VideoCaptureGlobals::captureLibChanged,
                     this,
                     &VideoCaptureElementSettings::captureLibChanged);
}

QString VideoCaptureElementSettings::codecLib() const
{
    return globalVideoCapture->codecLib();
}

QString VideoCaptureElementSettings::captureLib() const
{
    return globalVideoCapture->captureLib();
}

QStringList VideoCaptureElementSettings::codecSubModules() const
{
    return globalVideoCapture->codecSubModules();
}

QStringList VideoCaptureElementSettings::captureSubModules() const
{
    return globalVideoCapture->captureSubModules();
}

void VideoCaptureElementSettings::setCodecLib(const QString &codecLib)
{
    globalVideoCapture->setCodecLib(codecLib);
}

void VideoCaptureElementSettings::setCaptureLib(const QString &captureLib)
{
    globalVideoCapture->setCaptureLib(captureLib);
}

void VideoCaptureElementSettings::resetCodecLib()
{
    globalVideoCapture->resetCodecLib();
}

void VideoCaptureElementSettings::resetCaptureLib()
{
    globalVideoCapture->resetCaptureLib();
}

#include "moc_videocaptureelementsettings.cpp"
