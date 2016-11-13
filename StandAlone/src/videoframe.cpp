/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "videoframe.h"

VideoFrame::VideoFrame(const AkPacket &packet)
{
    this->m_image = AkUtils::packetToImage(packet)
                        .convertToFormat(QImage::Format_ARGB32)
                        .mirrored();
    this->m_textureId = 0;
}

VideoFrame::~VideoFrame()
{
    if (this->m_textureId < 1)
        return;

    auto context = const_cast<QGLContext *>(QGLContext::currentContext());

    if (!context)
        return;

    context->deleteTexture(this->m_textureId);
}

VideoFrame &VideoFrame::operator =(const AkPacket &packet)
{
    auto image = AkUtils::packetToImage(packet);

    if (!image.isNull())
        this->m_image = image.convertToFormat(QImage::Format_ARGB32).mirrored();

    return *this;
}

void VideoFrame::bind()
{
    if (this->m_image.isNull())
        return;

    auto context = const_cast<QGLContext *>(QGLContext::currentContext());

    if (!context)
        return;

    if (this->m_textureId > 0)
        context->deleteTexture(this->m_textureId);

    this->m_textureId = context->bindTexture(this->m_image);
}

bool VideoFrame::hasAlphaChannel() const
{
    return this->m_image.hasAlphaChannel();
}

bool VideoFrame::hasMipmaps() const
{
    return false;
}

int VideoFrame::textureId() const
{
    return int(this->m_textureId);
}

QSize VideoFrame::textureSize() const
{
    return this->m_image.size();
}
