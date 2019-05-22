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

#include "acapsconvertelement.h"
#include "acapsconvertelementsettings.h"
#include "convertaudio.h"

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

using ConvertAudioPtr = QSharedPointer<ConvertAudio>;

class ACapsConvertElementPrivate
{
    public:
        ACapsConvertElement *self;
        ACapsConvertElementSettings m_settings;
        AkAudioCaps m_caps;
        ConvertAudioPtr m_convertAudio;
        QMutex m_mutex;

        explicit ACapsConvertElementPrivate(ACapsConvertElement *self);
        void convertLibUpdated(const QString &convertLib);
};

ACapsConvertElement::ACapsConvertElement():
    AkElement()
{
    this->d = new ACapsConvertElementPrivate(this);
    QObject::connect(&this->d->m_settings,
                     &ACapsConvertElementSettings::convertLibChanged,
                     [this] (const QString &convertLib) {
                        this->d->convertLibUpdated(convertLib);
                     });

    this->d->convertLibUpdated(this->d->m_settings.convertLib());
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

}

void ACapsConvertElementPrivate::convertLibUpdated(const QString &convertLib)
{
    auto state = self->state();
    self->setState(AkElement::ElementStateNull);

    this->m_mutex.lock();
    this->m_convertAudio =
            ptr_cast<ConvertAudio>(ACapsConvertElement::loadSubModule("ACapsConvert",
                                                                      convertLib));
    this->m_mutex.unlock();

    self->setState(state);
}

#include "moc_acapsconvertelement.cpp"
