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

#include <QFuture>
#include <QMutex>
#include <QQmlContext>
#include <QThread>
#include <QThreadPool>
#include <QWaitCondition>
#include <QtConcurrent>
#include <akfrac.h>
#include <akpacket.h>

#include "packetsyncelement.h"

template <typename T>
inline void waitLoop(const QFuture<T> &loop)
{
    while (!loop.isFinished()) {
        auto eventDispatcher = QThread::currentThread()->eventDispatcher();

        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);

        QThread::msleep(1);
    }
}

class PacketSyncElementPrivate
{
    public:
        PacketSyncElement *self;
        bool m_audioEnabled {true};
        bool m_discardLast {false};
        qint64 m_audioClock {0};
        qint64 m_videoClock {0};
        // Time in seconds of the first packet of each stream. The timestamps
        // of the packets come from the encoders measured from a common
        // origin, so they are comparable between audio and video.
        bool m_hasAudioOrigin {false};
        bool m_hasVideoOrigin {false};
        qreal m_audioOrigin {0.0};
        qreal m_videoOrigin {0.0};
        // The earliest of the two. It is subtracted from all the packets, so
        // the first stream to start is at 0 and the other one keeps its real
        // offset from it.
        bool m_hasBase {false};
        qreal m_base {0.0};
        qint64 m_lastVideoPts {0};
        qint64 m_videoId {-1};
        QThreadPool m_threadPool;
        QMutex m_mutex;
        QWaitCondition m_packetAvailable;
        QVector<AkPacket> m_audioPackets;
        QVector<AkPacket> m_videoPackets;
        QFuture<void> m_packetLoopResult;
        bool m_initialized {false};
        bool m_run {false};

        explicit PacketSyncElementPrivate(PacketSyncElement *self);
        bool init();
        void uninit();
        void packetLoop();
        void emitPacket(AkPacket packet);
};

PacketSyncElement::PacketSyncElement():
    AkElement()
{
    this->d = new PacketSyncElementPrivate(this);
}

PacketSyncElement::~PacketSyncElement()
{
    delete this->d;
}

bool PacketSyncElement::audioEnabled() const
{
    return this->d->m_audioEnabled;
}

bool PacketSyncElement::discardLast() const
{
    return this->d->m_discardLast;
}

void PacketSyncElement::setAudioEnabled(bool audioEnabled)
{
    if (this->d->m_audioEnabled == audioEnabled)
        return;

    this->d->m_audioEnabled = audioEnabled;
    this->audioEnabledChanged(audioEnabled);
}

void PacketSyncElement::setDiscardLast(bool discardLast)
{
    if (this->d->m_discardLast == discardLast)
        return;

    this->d->m_discardLast = discardLast;
    this->discardLastChanged(discardLast);
}

void PacketSyncElement::resetAudioEnabled()
{
    this->setAudioEnabled(true);
}

void PacketSyncElement::resetDiscardLast()
{
    this->setDiscardLast(false);
}

AkPacket PacketSyncElement::iStream(const AkPacket &packet)
{
    QMutexLocker mutexLocker(&this->d->m_mutex);

    if (!this->d->m_initialized)
        return {};

    switch (packet.type()) {
    case AkPacket::PacketAudio:
    case AkPacket::PacketAudioCompressed: {
        if (!this->d->m_audioEnabled)
            break;

        auto pkt = packet;

        // The timeline starts at the pts of the first packet, the rest
        // continues counting the duration of the packets.
        if (!this->d->m_hasAudioOrigin) {
            this->d->m_hasAudioOrigin = true;
            this->d->m_audioOrigin = packet.pts() * packet.timeBase().value();
            this->d->m_audioClock = packet.pts();
        }

        pkt.setPts(this->d->m_audioClock);
        this->d->m_audioPackets << pkt;
        this->d->m_audioClock += packet.duration();
        this->d->m_packetAvailable.wakeAll();

        break;
    }

    case AkPacket::PacketVideo:
    case AkPacket::PacketVideoCompressed: {
        auto pkt = packet;

        if (!this->d->m_hasVideoOrigin) {
            this->d->m_hasVideoOrigin = true;
            this->d->m_videoOrigin = packet.pts() * packet.timeBase().value();
            this->d->m_videoClock = packet.pts();
            pkt.setPts(this->d->m_videoClock);
        } else {
            if (this->d->m_videoId == packet.id()) {
                auto duration = packet.pts() - this->d->m_lastVideoPts;
                this->d->m_videoPackets.last().setDuration(duration);
                this->d->m_videoClock += duration;
                pkt.setPts(this->d->m_videoClock);
            } else {
                this->d->m_videoClock += this->d->m_videoPackets.last().duration();
                pkt.setPts(this->d->m_videoClock);
            }
        }

        this->d->m_videoPackets << pkt;
        this->d->m_lastVideoPts = packet.pts();
        this->d->m_videoId = packet.id();
        this->d->m_packetAvailable.wakeAll();

        break;
    }

    default:
        break;
    }

    return {};
}

bool PacketSyncElement::setState(AkElement::ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            if (!this->d->init())
                return false;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    }

    return false;
}

PacketSyncElementPrivate::PacketSyncElementPrivate(PacketSyncElement *self):
    self(self)
{

}

bool PacketSyncElementPrivate::init()
{
    this->m_run = true;
    this->m_packetLoopResult =
            QtConcurrent::run(&this->m_threadPool,
                              &PacketSyncElementPrivate::packetLoop,
                              this);

    this->m_audioClock = 0;
    this->m_videoClock = 0;
    this->m_hasAudioOrigin = false;
    this->m_hasVideoOrigin = false;
    this->m_audioOrigin = 0.0;
    this->m_videoOrigin = 0.0;
    this->m_hasBase = false;
    this->m_base = 0.0;
    this->m_lastVideoPts = 0;
    this->m_videoId = 1;
    this->m_audioPackets.clear();
    this->m_videoPackets.clear();

    this->m_initialized = true;

    return true;
}

void PacketSyncElementPrivate::uninit()
{
    if (!this->m_initialized)
        return;

    this->m_initialized = false;

    this->m_run = false;
    waitLoop(this->m_packetLoopResult);

    this->m_audioPackets.clear();
    this->m_videoPackets.clear();
}

void PacketSyncElementPrivate::emitPacket(AkPacket packet)
{
    // The base is the start of the first stream to start. In normal operation
    // both streams are known when the first packet is sent.
    if (!this->m_hasBase) {
        if (this->m_audioEnabled
            && this->m_hasAudioOrigin
            && this->m_hasVideoOrigin) {
            this->m_base = qMin(this->m_audioOrigin, this->m_videoOrigin);
        } else if (this->m_hasVideoOrigin) {
            this->m_base = this->m_videoOrigin;
        } else {
            this->m_base = this->m_audioOrigin;
        }

        this->m_hasBase = true;
    }

    auto timeBase = packet.timeBase().value();

    if (timeBase > 0.0)
        packet.setPts(qMax<qint64>(packet.pts()
                                   - qRound64(this->m_base / timeBase),
                                   0));

    emit self->oStream(packet);
}

void PacketSyncElementPrivate::packetLoop()
{
    while (this->m_run) {
        QMutexLocker mutexLocker(&this->m_mutex);

        if ((this->m_audioEnabled && this->m_audioPackets.isEmpty())
            || this->m_videoPackets.size() < 2) {
            this->m_packetAvailable.wait(&this->m_mutex, 3000);

            if (this->m_audioPackets.isEmpty()
                || this->m_videoPackets.size() < 2) {
                continue;
            }
        }

        if (this->m_audioEnabled) {
            auto &audioPacket = this->m_audioPackets.first();
            auto &videoPacket = this->m_videoPackets.first();
            auto audioPts = audioPacket.pts() * audioPacket.timeBase().value();
            auto videoPts = videoPacket.pts() * videoPacket.timeBase().value();
            AkPacket packet =
                    videoPts <= audioPts?
                        this->m_videoPackets.takeFirst():
                        this->m_audioPackets.takeFirst();

            this->emitPacket(packet);
        } else {
            this->emitPacket(this->m_videoPackets.takeFirst());
        }
    }

    while (!this->m_audioPackets.isEmpty()
           || !this->m_videoPackets.isEmpty()) {
        qreal audioPts = 0.0;

        if (this->m_audioEnabled && !this->m_audioPackets.isEmpty()) {
            auto &audioPacket = this->m_audioPackets.first();
            audioPts = audioPacket.pts() * audioPacket.timeBase().value();
        }

        qreal videoPts = 0.0;

        if (!this->m_videoPackets.isEmpty()) {
            auto &videoPacket = this->m_videoPackets.first();
            videoPts = videoPacket.pts() * videoPacket.timeBase().value();
        }

        AkPacket packet;

        if ((this->m_audioEnabled
             && !this->m_audioPackets.isEmpty()
             && audioPts <= videoPts)
            || this->m_videoPackets.isEmpty()) {
            packet = this->m_audioPackets.takeFirst();
        } else {
            packet = this->m_videoPackets.takeFirst();
        }

        this->emitPacket(packet);

        if (this->m_discardLast
            && (this->m_audioPackets.isEmpty()
                || this->m_videoPackets.isEmpty())) {
            break;
        }
    }
}

#include "moc_packetsyncelement.cpp"
