/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "convertio.h"

ConvertIO::ConvertIO(QObject *parent): QObject(parent)
{
    this->m_iWidth = 0;
    this->m_iHeight = 0;
    this->m_iFormat = AV_PIX_FMT_NONE;
    this->m_oWidth = 0;
    this->m_oHeight = 0;
    this->m_oFormat = AV_PIX_FMT_NONE;
}

ConvertIO::ConvertIO(const QbPacket &iPacket, const QbCaps &oCaps)
{
    this->m_iWidth = iPacket.caps().property("width").toInt();
    this->m_iHeight = iPacket.caps().property("height").toInt();
    QString format = iPacket.caps().property("format").toString();

    this->m_iFormat = av_get_pix_fmt(format.toStdString().c_str());

    QList<QByteArray> props = oCaps.dynamicPropertyNames();

    this->m_oWidth = props.contains("width")?
                                    oCaps.property("width").toInt():
                                    this->m_iWidth;

    this->m_oHeight = props.contains("height")?
                                     oCaps.property("height").toInt():
                                     this->m_iHeight;

    if (props.contains("format")) {
        QString oFormatString = oCaps.property("format").toString();

        this->m_oFormat = av_get_pix_fmt(oFormatString.toStdString().c_str());
    }
    else
        this->m_oFormat = this->m_iFormat;
}

ConvertIO::ConvertIO(const ConvertIO &other):
    QObject(NULL),
    m_iWidth(other.m_iWidth),
    m_iHeight(other.m_iHeight),
    m_iFormat(other.m_iFormat),
    m_oWidth(other.m_oWidth),
    m_oHeight(other.m_oHeight),
    m_oFormat(other.m_oFormat)
{

}

ConvertIO &ConvertIO::operator =(const ConvertIO &other)
{
    if (this != &other) {
        this->m_iWidth = other.m_iWidth;
        this->m_iHeight = other.m_iHeight;
        this->m_iFormat = other.m_iFormat;
        this->m_oWidth = other.m_oWidth;
        this->m_oHeight = other.m_oHeight;
        this->m_oFormat = other.m_oFormat;
    }

    return *this;
}

int ConvertIO::iWidth() const
{
    return this->m_iWidth;
}

int ConvertIO::iHeight() const
{
    return this->m_iHeight;
}

PixelFormat ConvertIO::iFormat() const
{
    return this->m_iFormat;
}

int ConvertIO::oWidth() const
{
    return this->m_oWidth;
}

int ConvertIO::oHeight() const
{
    return this->m_oHeight;
}

PixelFormat ConvertIO::oFormat() const
{
    return this->m_oFormat;
}

QList<int> ConvertIO::check() const
{
    QList<int> checkList;

    checkList << this->m_iWidth;
    checkList << this->m_iHeight;
    checkList << this->m_iFormat;
    checkList << this->m_oWidth;
    checkList << this->m_oHeight;
    checkList << this->m_oFormat;

    return checkList;
}
