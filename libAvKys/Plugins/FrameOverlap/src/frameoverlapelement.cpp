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

#include <QQmlContext>
#include <QSize>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "frameoverlapelement.h"

struct SumPixel
{
    quint64 r;
    quint64 g;
    quint64 b;
    quint64 a;
};

class FrameOverlapElementPrivate
{
    public:
        int m_nFrames {16};
        int m_stride {4};
        QVector<AkVideoPacket> m_frames;
        QSize m_frameSize;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
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
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_frameSize = frameSize;
    }

    this->d->m_frames << src;
    int diff = this->d->m_frames.size() - this->d->m_nFrames;

    for (int i = 0; i < diff; i++)
        this->d->m_frames.removeFirst();

    int stride = qMax(this->d->m_stride, 1);

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    auto sumFrameSize = size_t(dst.caps().width())
                        * size_t(dst.caps().height());
    auto sumFrame = new SumPixel[sumFrameSize];
    memset(sumFrame, 0, sumFrameSize * sizeof(SumPixel));
    int nFrames = 0;

    for (int i = this->d->m_frames.size() - 1;
         i >= 0;
         i -= stride) {
        auto &frame = this->d->m_frames[i];
        auto dstLine = sumFrame;

        for (int y = 0; y < dst.caps().height(); y++) {
            auto srcLine = reinterpret_cast<const QRgb *>(frame.constLine(0, y));

            for (int x = 0; x < dst.caps().width(); x++) {
                auto &ipixel = srcLine[x];
                auto &opixel = dstLine[x];

                opixel.r += qRed(ipixel);
                opixel.g += qGreen(ipixel);
                opixel.b += qBlue(ipixel);
                opixel.a += qAlpha(ipixel);
            }

            dstLine += dst.caps().width();
        }

        nFrames++;
    }

    if (nFrames < 1) {
        delete [] sumFrame;

        if (src)
            emit this->oStream(src);

        return src;
    }

    auto srcLine = sumFrame;

    for (int y = 0; y < dst.caps().height(); y++) {
        auto dstBits = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < dst.caps().width(); x++) {
            auto &ipixel = srcLine[x];
            dstBits[x] = qRgba(ipixel.r / nFrames,
                               ipixel.g / nFrames,
                               ipixel.b / nFrames,
                               ipixel.a / nFrames);
        }

        srcLine += dst.caps().width();
    }

    delete [] sumFrame;

    if (dst)
        emit this->oStream(dst);

    return dst;
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
