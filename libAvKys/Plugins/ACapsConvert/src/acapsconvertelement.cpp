/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#include "acapsconvertelement.h"
#include "acapsconvertglobals.h"
#include "convertaudio.h"
#include "akcaps.h"
#include "akpacket.h"
#include "akaudiocaps.h"

Q_GLOBAL_STATIC(ACapsConvertGlobals, globalACapsConvert)

template<typename T>
inline QSharedPointer<T> ptr_cast(QObject *obj=nullptr)
{
    return QSharedPointer<T>(static_cast<T *>(obj));
}

typedef QSharedPointer<ConvertAudio> ConvertAudioPtr;

class ACapsConvertElementPrivate
{
    public:
        AkCaps m_caps;
        ConvertAudioPtr m_convertAudio;
        QMutex m_mutex;
};

ACapsConvertElement::ACapsConvertElement():
    AkElement()
{
    this->d = new ACapsConvertElementPrivate;

    QObject::connect(globalACapsConvert,
                     SIGNAL(convertLibChanged(const QString &)),
                     this,
                     SIGNAL(convertLibChanged(const QString &)));
    QObject::connect(globalACapsConvert,
                     SIGNAL(convertLibChanged(const QString &)),
                     this,
                     SLOT(convertLibUpdated(const QString &)));

    this->convertLibUpdated(globalACapsConvert->convertLib());
}

ACapsConvertElement::~ACapsConvertElement()
{
    delete this->d;
}

QString ACapsConvertElement::caps() const
{
    return this->d->m_caps.toString();
}

QString ACapsConvertElement::convertLib() const
{
    return globalACapsConvert->convertLib();
}

void ACapsConvertElement::setCaps(const QString &caps)
{
    if (this->d->m_caps == caps)
        return;

    this->d->m_caps = caps;
    emit this->capsChanged(caps);
}

void ACapsConvertElement::setConvertLib(const QString &convertLib)
{
    globalACapsConvert->setConvertLib(convertLib);
}

void ACapsConvertElement::resetCaps()
{
    this->setCaps("");
}

void ACapsConvertElement::resetConvertLib()
{
    globalACapsConvert->resetConvertLib();
}

AkPacket ACapsConvertElement::iStream(const AkAudioPacket &packet)
{
    AkPacket oPacket;

    this->d->m_mutex.lock();

    if (this->d->m_convertAudio)
        oPacket = this->d->m_convertAudio->convert(packet);

    this->d->m_mutex.unlock();

    akSend(oPacket)
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

void ACapsConvertElement::convertLibUpdated(const QString &convertLib)
{
    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    this->d->m_mutex.lock();

    this->d->m_convertAudio =
            ptr_cast<ConvertAudio>(this->loadSubModule("ACapsConvert",
                                                       convertLib));
    this->d->m_mutex.unlock();

    this->setState(state);
}

#include "moc_acapsconvertelement.cpp"
