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

#include <QImage>
#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "frameoverlapelement.h"

class FrameOverlapElementPrivate
{
    public:
        int m_nFrames {16};
        int m_stride {4};
        QVector<QImage> m_frames;
        QSize m_frameSize;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argb, 0, 0, {}}};
};

FrameOverlapElement::FrameOverlapElement(): AkElement()
{
    this->d = new FrameOverlapElementPrivate;
}

FrameOverlapElement::~FrameOverlapElement()
{
    delete this->d;
}

int FrameOverlapElement::nFrames() const
{
    return this->d->m_nFrames;
}

int FrameOverlapElement::stride() const
{
    return this->d->m_stride;
}

QString FrameOverlapElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FrameOverlap/share/qml/main.qml");
}

void FrameOverlapElement::controlInterfaceConfigure(QQmlContext *context,
                                                    const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FrameOverlap", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket FrameOverlapElement::iVideoStream(const AkVideoPacket &packet)
{
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    QImage oFrame(src.size(), src.format());

    if (src.size() != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_frameSize = src.size();
    }

    this->d->m_frames << src.copy();
    int diff = this->d->m_frames.size() - this->d->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->d->m_frames.removeFirst();

    int stride = this->d->m_stride > 0? this->d->m_stride: 1;

    for (int y = 0; y < oFrame.height(); y++) {
        auto dstBits = reinterpret_cast<QRgb *>(oFrame.scanLine(y));

        for (int x = 0; x < oFrame.width(); x++) {
            int r = 0;
            int g = 0;
            int b = 0;
            int a = 0;
            int n = 0;

            for (int frame = this->d->m_frames.size() - 1;
                 frame >= 0;
                 frame -= stride) {
                QRgb pixel = this->d->m_frames[frame].pixel(x, y);

                r += qRed(pixel);
                g += qGreen(pixel);
                b += qBlue(pixel);
                a += qAlpha(pixel);
                n++;
            }

            if (n > 0) {
                r /= n;
                g /= n;
                b /= n;
                a /= n;

                dstBits[x] = qRgba(r, g, b, a);
            } else {
                dstBits[x] = qRgba(0, 0, 0, 0);
            }
        }
    }

    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
}

void FrameOverlapElement::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
}

void FrameOverlapElement::setStride(int stride)
{
    if (this->d->m_stride == stride)
        return;

    this->d->m_stride = stride;
    emit this->strideChanged(stride);
}

void FrameOverlapElement::resetNFrames()
{
    this->setNFrames(16);
}

void FrameOverlapElement::resetStride()
{
    this->setStride(4);
}

#include "moc_frameoverlapelement.cpp"
