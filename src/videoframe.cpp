/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#include "videoframe.h"

VideoFrame::VideoFrame(const QbPacket &packet)
{
    this->m_image = QbUtils::packetToImage(packet).mirrored();
    this->m_textureId = 0;
}

VideoFrame::~VideoFrame()
{
    if (this->m_textureId < 1)
        return;

    QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());

    if (!context)
        return;

    context->deleteTexture(this->m_textureId);
}

VideoFrame &VideoFrame::operator =(const QbPacket &packet)
{
    QImage image = QbUtils::packetToImage(packet);

    if (!image.isNull())
        this->m_image = image.mirrored();

    return *this;
}

void VideoFrame::bind()
{
    if (this->m_image.isNull())
        return;

    QGLContext *context = const_cast<QGLContext *>(QGLContext::currentContext());

    if (!context)
        return;

    if (this->m_textureId >= 0)
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
    return this->m_textureId;
}

QSize VideoFrame::textureSize() const
{
    return this->m_image.size();
}
