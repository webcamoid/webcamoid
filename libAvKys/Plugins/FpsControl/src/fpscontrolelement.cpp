/* Webcamoid, camera capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "fpscontrolelement.h"

class FpsControlElementPrivate
{
    public:
        AkFrac m_fps {30, 1};
        bool m_fillGaps {false};
        qint64 m_pts {0};
        qint64 m_prevPts {-1};
        qint64 m_id {-1};
        AkVideoPacket m_prevPacket;
};

FpsControlElement::FpsControlElement():
    AkElement()
{
    this->d = new FpsControlElementPrivate;
}

FpsControlElement::~FpsControlElement()
{
    delete this->d;
}

AkFrac FpsControlElement::fps() const
{
    return this->d->m_fps;
}

bool FpsControlElement::fillGaps() const
{
    return this->d->m_fillGaps;
}

bool FpsControlElement::discard(const AkVideoPacket &packet)
{
    if (!packet)
        return true;

    auto pts = qint64(packet.pts()
                      * packet.timeBase().value()
                      * this->d->m_fps.value());

    if (this->d->m_prevPts >= 0
        && this->d->m_id == packet.id()
        && pts == this->d->m_prevPts) {
        return true;
    }

    return false;
}

QString FpsControlElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/FpsControl/share/qml/main.qml");
}

void FpsControlElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("FpsControl", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket FpsControlElement::iVideoStream(const AkVideoPacket &packet)
{
    if (!packet)
        return {};

    auto pts = qint64(packet.pts()
                      * packet.timeBase().value()
                      * this->d->m_fps.value());

    /* The source frame rate is much higher than the output frame rate,
     * discard the exedent frame.
     */
    if (this->d->m_prevPts >= 0
        && this->d->m_id == packet.id()
        && pts == this->d->m_prevPts) {
        return {};
    }

    // Calculate the number of frames between the previous and the current one.
    qint64 framesDiff =
            this->d->m_prevPts < 0
            || pts <= this->d->m_prevPts
            || this->d->m_id != packet.id()?
                1:
                pts - this->d->m_prevPts;
    quint64 fill = framesDiff - 1;

    /* If the fillGaps option is enabled, repeat the previous frame until
     * complete the missings one.
     */
    if (this->d->m_fillGaps && fill > 0)
        for (quint64 i = 0; i < fill; ++i) {
            this->d->m_prevPacket.setPts(this->d->m_pts);
            emit this->oStream(this->d->m_prevPacket);
            ++this->d->m_pts;
        }

    if (this->d->m_prevPts >= 0 && !this->d->m_fillGaps)
        this->d->m_pts += framesDiff;

    this->d->m_prevPacket = packet;
    this->d->m_prevPacket.setPts(this->d->m_pts);
    this->d->m_prevPacket.setDuration(1);
    this->d->m_prevPacket.setTimeBase(this->d->m_fps.invert());
    emit this->oStream(this->d->m_prevPacket);

    if (this->d->m_prevPts < 0 || this->d->m_fillGaps)
        ++this->d->m_pts;

    this->d->m_id = packet.id();
    this->d->m_prevPts = pts;

    return this->d->m_prevPacket;
}

void FpsControlElement::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void FpsControlElement::setFillGaps(bool fillGaps)
{
    if (this->d->m_fillGaps == fillGaps)
        return;

    this->d->m_fillGaps = fillGaps;
    emit this->fillGapsChanged(fillGaps);
}

void FpsControlElement::resetFps()
{
    this->setFps({30, 1});
}

void FpsControlElement::resetFillGaps()
{
    this->setFillGaps(false);
}

void FpsControlElement::restart()
{
    this->d->m_pts = 0;
    this->d->m_prevPacket = AkVideoPacket();
}

#include "moc_fpscontrolelement.cpp"
