/* Webcamoid, webcam capture application.
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
        pkt.setPts(this->d->m_audioClock);
        this->d->m_audioPackets << pkt;
        this->d->m_audioClock += packet.duration();
        this->d->m_packetAvailable.wakeAll();

        break;
    }

    case AkPacket::PacketVideo:
    case AkPacket::PacketVideoCompressed: {
        auto pkt = packet;

        if (this->d->m_videoPackets.isEmpty()) {
            pkt.setPts(0);
            this->d->m_videoClock = 0;
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

            emit self->oStream(packet);
        } else {
            emit self->oStream(this->m_videoPackets.takeFirst());
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

        emit self->oStream(packet);

        if (this->m_discardLast
            && (this->m_audioPackets.isEmpty()
                || this->m_videoPackets.isEmpty())) {
            break;
        }
    }
}

#include "moc_packetsyncelement.cpp"
