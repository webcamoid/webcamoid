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

#include <QtDebug>
#include <akaudioconverter.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akaudiopacket.h>

#include "fillaudiogapselement.h"

class FillAudioGapsElementPrivate
{
    public:
        FillAudioGapsElement *self;
        AkAudioCaps m_outputCaps;
        int m_outputSamples {0};
        bool m_fillGaps {false};
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

AkPacket FillAudioGapsElement::iAudioStream(const AkAudioPacket &packet)
{
    if (this->d->m_paused)
        return {};

    AkAudioPacket src = this->d->m_audioConvert.convert(packet);

    if (!src)
        return {};

    if (this->d->m_fillGaps) {
        qint64 samples =
                this->d->m_id == src.id()?
                    qMax<qint64>(src.pts()
                                 - this->d->m_prevPts
                                 - this->d->m_prevDuration, 0):
                    0;

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
            this->d->m_pts = 0;
        else if (src.id() == this->d->m_id)
            this->d->m_pts += src.pts() - this->d->m_prevPts;
        else
            this->d->m_pts += this->d->m_prevDuration;

        src.setPts(this->d->m_pts);

        emit this->oStream(src);
    }

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

void FillAudioGapsElement::resetOutputCaps()
{
    this->setOutputCaps(AkAudioCaps());
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
    this->m_id = -1;
    this->m_pts = -1;
    this->m_prevPts = -1;
    this->m_prevDuration = -1;

    return true;
}

void FillAudioGapsElementPrivate::uninit()
{
    if (this->m_audioBuffer)
        emit self->oStream(this->m_audioBuffer);

    this->m_audioBuffer = AkAudioPacket();
}

#include "moc_fillaudiogapselement.cpp"
