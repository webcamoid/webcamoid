/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <gst/app/gstappsink.h>
#include <gst/audio/audio.h>
#include <gst/video/video.h>

#include "mediasink.h"

MediaSink::MediaSink(QObject *parent): QObject(parent)
{
    gst_init(NULL, NULL);
}

MediaSink::~MediaSink()
{
}

QString MediaSink::location() const
{
    return this->m_location;
}

QString MediaSink::outputFormat() const
{
    return this->m_outputFormat;
}

QStringList MediaSink::supportedFormats()
{

}

QStringList MediaSink::fileExtensions(const QString &format)
{

}

QString MediaSink::formatDescription(const QString &format)
{

}

QStringList MediaSink::supportedCodecs(const QString &format,
                                       const QString &type)
{

}

QString MediaSink::defaultCodec(const QString &format, const QString &type)
{

}

QString MediaSink::codecDescription(const QString &codec)
{

}
QString MediaSink::codecType(const QString &codec)
{
}

QVariantMap MediaSink::defaultCodecParams(const QString &codec)
{

}

QVariantMap MediaSink::addStream(int streamIndex,
                                 const QbCaps &streamCaps,
                                 const QVariantMap &codecParams)
{

}

void MediaSink::setLocation(const QString &location)
{
    if (this->m_location == location)
        return;

    this->m_location = location;
    emit this->locationChanged(location);
}

void MediaSink::setOutputFormat(const QString &outputFormat)
{
    if (this->m_outputFormat == outputFormat)
        return;

    this->m_outputFormat = outputFormat;
    emit this->outputFormatChanged(outputFormat);
}

void MediaSink::resetLocation()
{
    this->setLocation("");
}

void MediaSink::resetOutputFormat()
{
    this->setOutputFormat("");
}

void MediaSink::writeAudioPacket(const QbAudioPacket &packet)
{

}

void MediaSink::writeVideoPacket(const QbVideoPacket &packet)
{

}

void MediaSink::writeSubtitlePacket(const QbPacket &packet)
{

}

void MediaSink::clearStreams()
{

}

bool MediaSink::init()
{

}

void MediaSink::uninit()
{

}
