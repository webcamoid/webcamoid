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

#include <QSize>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "frameoverlap.h"

struct SumPixel
{
    quint64 r;
    quint64 g;
    quint64 b;
    quint64 a;
};

class FrameOverlapPrivate
{
    public:
        FrameOverlap *self {nullptr};
        QString m_description {QObject::tr("Kung Fu Master")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        int m_nFrames {16};
        int m_stride {4};
        QVector<AkVideoPacket> m_frames;
        QSize m_frameSize;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit FrameOverlapPrivate(FrameOverlap *self);
};

FrameOverlap::FrameOverlap(QObject *parent):
    QObject(parent)
{
    this->d = new FrameOverlapPrivate(this);
}

FrameOverlap::~FrameOverlap()
{
    delete this->d;
}

QString FrameOverlap::description() const
{
    return this->d->m_description;
}

AkElementType FrameOverlap::type() const
{
    return this->d->m_type;
}

AkElementCategory FrameOverlap::category() const
{
    return this->d->m_category;
}

void *FrameOverlap::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *FrameOverlap::create(const QString &id)
{
    Q_UNUSED(id)

    return new FrameOverlap;
}

int FrameOverlap::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/FrameOverlap",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

int FrameOverlap::nFrames() const
{
    return this->d->m_nFrames;
}

int FrameOverlap::stride() const
{
    return this->d->m_stride;
}

void FrameOverlap::deleteThis(void *userData) const
{
    delete reinterpret_cast<FrameOverlap *>(userData);
}

QString FrameOverlap::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FrameOverlap/share/qml/main.qml");
}

void FrameOverlap::controlInterfaceConfigure(QQmlContext *context,
                                                    const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FrameOverlap", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket FrameOverlap::iVideoStream(const AkVideoPacket &packet)
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
            this->oStream(src);

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
        this->oStream(dst);

    return dst;
}

void FrameOverlap::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
}

void FrameOverlap::setStride(int stride)
{
    if (this->d->m_stride == stride)
        return;

    this->d->m_stride = stride;
    emit this->strideChanged(stride);
}

void FrameOverlap::resetNFrames()
{
    this->setNFrames(16);
}

void FrameOverlap::resetStride()
{
    this->setStride(4);
}

FrameOverlapPrivate::FrameOverlapPrivate(FrameOverlap *self):
    self(self)
{

}

#include "moc_frameoverlap.cpp"
