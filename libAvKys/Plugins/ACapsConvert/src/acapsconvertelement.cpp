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

#include <QSharedPointer>
#include <QMutex>
#include <akcaps.h>
#include <akpacket.h>
#include <akaudiocaps.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>

#include "acapsconvertelement.h"
#include "convertaudio.h"

using ConvertAudioPtr = QSharedPointer<ConvertAudio>;

class ACapsConvertElementPrivate
{
    public:
        ACapsConvertElement *self;
        AkAudioCaps m_caps;
        ConvertAudioPtr m_convertAudio;
        QString m_convertAudioImpl;
        QMutex m_mutex;

        explicit ACapsConvertElementPrivate(ACapsConvertElement *self);
        void linksChanged(const AkPluginLinks &links);
};

ACapsConvertElement::ACapsConvertElement():
    AkElement()
{
    this->d = new ACapsConvertElementPrivate(this);
    QObject::connect(akPluginManager,
                     &AkPluginManager::linksChanged,
                     this,
                     [this] (const AkPluginLinks &links) {
                        this->d->linksChanged(links);
                     });
}

ACapsConvertElement::~ACapsConvertElement()
{
    delete this->d;
}

AkAudioCaps ACapsConvertElement::caps() const
{
    return this->d->m_caps;
}

AkPacket ACapsConvertElement::iAudioStream(const AkAudioPacket &packet)
{
    AkPacket oPacket;

    this->d->m_mutex.lock();

    if (this->d->m_convertAudio)
        oPacket = this->d->m_convertAudio->convert(packet);

    this->d->m_mutex.unlock();

    akSend(oPacket)
}

void ACapsConvertElement::setCaps(const AkAudioCaps &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void ACapsConvertElement::resetCaps()
{
    this->setCaps({});
}

bool ACapsConvertElement::setState(AkElement::ElementState state)
{
    if (!this->d->m_convertAudio)
        return false;

    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            if (!this->d->m_convertAudio->init(this->d->m_caps))
                return false;

            return AkElement::setState(state);
        }
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_convertAudio->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
            this->d->m_convertAudio->uninit();

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

ACapsConvertElementPrivate::ACapsConvertElementPrivate(ACapsConvertElement *self):
    self(self)
{
    this->m_convertAudio = akPluginManager->create<ConvertAudio>("AudioFilter/AudioConvert/Impl/*");
    this->m_convertAudioImpl = akPluginManager->defaultPlugin("AudioFilter/AudioConvert/Impl/*",
                                                              {"AudioConvertImpl"}).id();
}

void ACapsConvertElementPrivate::linksChanged(const AkPluginLinks &links)
{
    if (!links.contains("AudioFilter/AudioConvert/Impl/*")
        || links["AudioFilter/AudioConvert/Impl/*"] != this->m_convertAudioImpl)
        return;

    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutex.lock();
    this->m_convertAudio =
            akPluginManager->create<ConvertAudio>("AudioFilter/AudioConvert/Impl/*");
    this->m_mutex.unlock();

    this->m_convertAudioImpl = links["AudioFilter/AudioConvert/Impl/*"];

    if (!this->m_convertAudio)
        return;

    self->setState(state);
}

#include "moc_acapsconvertelement.cpp"
