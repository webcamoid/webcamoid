/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "videostream.h"

VideoStream::VideoStream(QObject *parent): AbstractStream(parent)
{
    this->m_scaleContext = NULL;
    this->m_picture = NULL;
    this->m_pictureRgb = NULL;
    this->m_bufferRgb = NULL;
}

VideoStream::VideoStream(AVFormatContext *formatContext, uint index): AbstractStream(formatContext, index)
{
    this->m_scaleContext = NULL;
    this->m_picture = NULL;
    this->m_pictureRgb = NULL;
    this->m_bufferRgb = NULL;

    if (!this->isValid())
        return;

    this->m_isValid = false;

    this->m_picture = avcodec_alloc_frame();

    if (!this->m_picture)
        return;

    this->m_pictureRgb = avcodec_alloc_frame();

    if(!this->m_pictureRgb)
    {
        avcodec_free_frame(&this->m_picture);

        return;
    }

    this->m_scaleContext = NULL;

    int pictureRgbSize = avpicture_get_size(PIX_FMT_RGB24,
                                            this->codecContext()->width,
                                            this->codecContext()->height);

    this->m_bufferRgb = (uint8_t *) av_malloc(pictureRgbSize * sizeof(uint8_t));

    if (!this->m_bufferRgb)
    {
        avcodec_free_frame(&this->m_pictureRgb);
        avcodec_free_frame(&this->m_picture);

        return;
    }

    avpicture_fill((AVPicture *) this->m_pictureRgb,
                   this->m_bufferRgb,
                   PIX_FMT_RGB24,
                   this->codecContext()->width,
                   this->codecContext()->height);

    this->m_isValid = true;
}

VideoStream::VideoStream(const VideoStream &other):
    AbstractStream(other),
    m_scaleContext(other.m_scaleContext),
    m_picture(other.m_picture),
    m_pictureRgb(other.m_pictureRgb),
    m_bufferRgb(other.m_bufferRgb)
{
}

VideoStream::~VideoStream()
{
    if (this->m_orig || !this->m_copy.isEmpty() || !this->isValid())
        return;

    if (this->m_bufferRgb)
        av_free(this->m_bufferRgb);

    if (this->m_scaleContext)
        sws_freeContext(this->m_scaleContext);

    if (this->m_pictureRgb)
        avcodec_free_frame(&this->m_pictureRgb);

    if (this->m_picture)
        avcodec_free_frame(&this->m_picture);
}

VideoStream &VideoStream::operator =(const VideoStream &other)
{
    if (this != &other)
    {
        this->m_scaleContext = other.m_scaleContext;
        this->m_picture = other.m_picture;
        this->m_pictureRgb = other.m_pictureRgb;
        this->m_bufferRgb = other.m_bufferRgb;

        AbstractStream::operator =(other);
    }

    return *this;
}

QImage VideoStream::readFrame(AVPacket *packet)
{
    if (!this->isValid())
        return QImage();

    int gotPicture;

    avcodec_decode_video2(this->codecContext(),
                          this->m_picture,
                          &gotPicture,
                          packet);

    if (!gotPicture)
        return QImage();

    this->m_scaleContext = sws_getCachedContext(this->m_scaleContext,
                                                this->codecContext()->width,
                                                this->codecContext()->height,
                                                this->codecContext()->pix_fmt,
                                                this->codecContext()->width,
                                                this->codecContext()->height,
                                                PIX_FMT_RGB24,
                                                SWS_FAST_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    sws_scale(this->m_scaleContext,
              (uint8_t **) this->m_picture->data,
              this->m_picture->linesize,
              0,
              this->codecContext()->height,
              this->m_pictureRgb->data,
              this->m_pictureRgb->linesize);

    QImage image = QImage(this->codecContext()->width,
                          this->codecContext()->height,
                          QImage::Format_RGB888);

    for(int y = 0; y < this->codecContext()->height; y++)
       memcpy(image.scanLine(y),
              this->m_pictureRgb->data[0] + y * this->m_pictureRgb->linesize[0],
              3 * this->codecContext()->width);

    return image;
}
