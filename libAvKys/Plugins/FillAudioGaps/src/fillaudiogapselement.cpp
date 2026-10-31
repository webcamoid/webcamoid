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

#include <QtDebug>
#include <akaudiocaps.h>
#include <akaudioconverter.h>
#include <akaudiopacket.h>
#include <akfrac.h>
#include <akpacket.h>

#include "fillaudiogapselement.h"

class FillAudioGapsElementPrivate
{
    public:
        FillAudioGapsElement *self;
        AkAudioCaps m_outputCaps;
        int m_outputSamples {0};
        bool m_fillGaps {false};
        qint64 m_startTime {-1};
        bool m_first {true};
        AkAudioConverter m_audioConvert;
        AkAudioPacket m_audioBuffer;
        qint64 m_id {-1};
        qint64 m_pts {-1};
        qint64 m_prevPts {-1};
        qint64 m_prevDuration {-1};
        bool m_paused {false};

        explicit FillAudioGapsElementPrivate(FillAudioGapsElement *self);
        bool init();
        void uninit();
        qint64 firstPts(const AkAudioPacket &packet) const;
};

FillAudioGapsElement::FillAudioGapsElement():
    AkElement()
{
    this->d = new FillAudioGapsElementPrivate(this);
}

FillAudioGapsElement::~FillAudioGapsElement()
{
    delete this->d;
}

AkAudioCaps FillAudioGapsElement::outputCaps() const
{
    return this->d->m_outputCaps;
}

int FillAudioGapsElement::outputSamples() const
{
    return this->d->m_outputSamples;
}

bool FillAudioGapsElement::fillGaps() const
{
    return this->d->m_fillGaps;
}

qint64 FillAudioGapsElement::startTime() const
{
    return this->d->m_startTime;
}

AkPacket FillAudioGapsElement::iAudioStream(const AkAudioPacket &packet)
{
    if (this->d->m_paused)
        return {};

    AkAudioPacket src = this->d->m_audioConvert.convert(packet);

    if (!src)
        return {};

    if (this->d->m_fillGaps) {
        /* The packets of the mixer have the default id (-1), the same value
         * as the initial one, so the first packet must be checked
         * explicitly: otherwise the gap would be computed against the initial
         * values and a silence as long as the pts itself would be inserted.
         */
        qint64 samples =
                !this->d->m_first && this->d->m_id == src.id()?
                    qMax<qint64>(src.pts()
                                 - this->d->m_prevPts
                                 - this->d->m_prevDuration, 0):
                    0;

        /* The output timeline starts at the pts of the first packet measured
         * from the common origin (or at 0 if there is none).
         */
        if (this->d->m_first)
            this->d->m_audioBuffer.setPts(this->d->firstPts(src));

        this->d->m_audioBuffer.setId(src.id());
        this->d->m_audioBuffer.setIndex(src.pts());
        this->d->m_audioBuffer += AkAudioPacket(src.caps(), samples, true) + src;

        forever {
            if (this->d->m_outputSamples > 0) {
                if (this->d->m_outputSamples > this->d->m_audioBuffer.samples())
                    break;

                auto buffer = this->d->m_audioBuffer.pop(this->d->m_outputSamples);

                if (buffer)
                    emit this->oStream(buffer);
            } else {
                auto buffer =
                        this->d->m_audioBuffer.pop();

                if (buffer)
                    emit this->oStream(buffer);

                break;
            }
        }
    } else {
        if (this->d->m_pts < 0)
            this->d->m_pts = this->d->firstPts(src);
        else if (src.id() == this->d->m_id)
            this->d->m_pts += src.pts() - this->d->m_prevPts;
        else
            this->d->m_pts += this->d->m_prevDuration;

        src.setPts(this->d->m_pts);

        emit this->oStream(src);
    }

    this->d->m_first = false;
    this->d->m_id = src.id();
    this->d->m_prevPts = src.pts();
    this->d->m_prevDuration = src.duration();

    return {};
}

void FillAudioGapsElement::setOutputCaps(const AkAudioCaps &outputCaps)
{
    if (this->d->m_outputCaps == outputCaps)
        return;

    this->d->m_outputCaps = outputCaps;
    this->d->m_audioConvert.setOutputCaps(outputCaps);
    emit this->outputCapsChanged(outputCaps);
}

void FillAudioGapsElement::setOutputSamples(int outputSamples)
{
    if (this->d->m_outputSamples == outputSamples)
        return;

    this->d->m_outputSamples = outputSamples;
    emit this->outputSamplesChanged(outputSamples);
}

void FillAudioGapsElement::setFillGaps(bool fillGaps)
{
    if (this->d->m_fillGaps == fillGaps)
        return;

    this->d->m_fillGaps = fillGaps;
    emit this->fillGapsChanged(fillGaps);
}

void FillAudioGapsElement::setStartTime(qint64 startTime)
{
    if (this->d->m_startTime == startTime)
        return;

    this->d->m_startTime = startTime;
    emit this->startTimeChanged(startTime);
}

void FillAudioGapsElement::resetOutputCaps()
{
    this->setOutputCaps(AkAudioCaps());
}

void FillAudioGapsElement::resetStartTime()
{
    this->setStartTime(-1);
}

void FillAudioGapsElement::resetOutputSamples()
{
    this->setOutputSamples(0);
}

void FillAudioGapsElement::resetFillGaps()
{
    this->setFillGaps(false);
}

bool FillAudioGapsElement::setState(AkElement::ElementState state)
{
    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_paused = state == AkElement::ElementStatePaused;
        case AkElement::ElementStatePlaying:
            if (!this->d->init()) {
                this->d->m_paused = false;

                return false;
            }

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
            this->d->m_paused = false;

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
            this->d->m_paused = true;

            return AkElement::setState(state);
        default:
            break;
        }

        break;
    }
    }

    return false;
}

FillAudioGapsElementPrivate::FillAudioGapsElementPrivate(FillAudioGapsElement *self):
    self(self)
{

}

bool FillAudioGapsElementPrivate::init()
{
    this->m_audioConvert.reset();
    this->m_audioBuffer = AkAudioPacket(this->m_audioConvert.outputCaps());
    this->m_first = true;
    this->m_id = -1;
    this->m_pts = -1;
    this->m_prevPts = -1;
    this->m_prevDuration = -1;

    return true;
}

qint64 FillAudioGapsElementPrivate::firstPts(const AkAudioPacket &packet) const
{
    if (this->m_startTime < 0)
        return 0;

    auto timeBase = packet.timeBase().value();

    if (timeBase <= 0.0)
        return 0;

    // Same units as the packet pts.
    auto originPts = qint64(double(this->m_startTime) * 1e-9 / timeBase);

    return qMax<qint64>(packet.pts() - originPts, 0);
}

void FillAudioGapsElementPrivate::uninit()
{
    if (this->m_audioBuffer)
        emit self->oStream(this->m_audioBuffer);

    this->m_audioBuffer = AkAudioPacket();
}

#include "moc_fillaudiogapselement.cpp"
