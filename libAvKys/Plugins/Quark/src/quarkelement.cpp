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
#include <QRandomGenerator>
#include <QSize>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "quarkelement.h"

class QuarkElementPrivate
{
    public:
        QVector<AkVideoPacket> m_frames;
        QSize m_frameSize;
        int m_nFrames {16};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
};

QuarkElement::QuarkElement(): AkElement()
{
    this->d = new QuarkElementPrivate;
}

QuarkElement::~QuarkElement()
{
    delete this->d;
}

int QuarkElement::nFrames() const
{
    return this->d->m_nFrames;
}

QString QuarkElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Quark/share/qml/main.qml");
}

void QuarkElement::controlInterfaceConfigure(QQmlContext *context, const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Quark", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket QuarkElement::iVideoStream(const AkVideoPacket &packet)
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
    int diff = this->d->m_frames.size() - qMax(this->d->m_nFrames, 1);

    for (int i = 0; i < diff; i++)
        this->d->m_frames.removeFirst();

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); y++) {
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            int frame = QRandomGenerator::global()->bounded(this->d->m_frames.size());
            dstLine[x] = this->d->m_frames[frame].pixel<QRgb>(0, x, y);
        }
    }

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void QuarkElement::setNFrames(int nFrames)
{
    if (this->d->m_nFrames == nFrames)
        return;

    this->d->m_nFrames = nFrames;
    emit this->nFramesChanged(nFrames);
}

void QuarkElement::resetNFrames()
{
    this->setNFrames(16);
}

#include "moc_quarkelement.cpp"
