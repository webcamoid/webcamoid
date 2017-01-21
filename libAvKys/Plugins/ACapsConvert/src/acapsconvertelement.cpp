/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include "acapsconvertelement.h"
#include "acapsconvertglobals.h"

Q_GLOBAL_STATIC(ACapsConvertGlobals, globalACapsConvert)

template<typename T>
inline QSharedPointer<T> ptr_init(QObject *obj=nullptr)
{
    if (!obj)
        return QSharedPointer<T>(new T());

    return QSharedPointer<T>(static_cast<T *>(obj));
}

ACapsConvertElement::ACapsConvertElement():
    AkElement(),
    m_convertAudio(ptr_init<ConvertAudio>())
{
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

QString ACapsConvertElement::caps() const
{
    return this->m_caps.toString();
}

QString ACapsConvertElement::convertLib() const
{
    return globalACapsConvert->convertLib();
}

void ACapsConvertElement::setCaps(const QString &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_caps = caps;
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
    this->m_mutex.lock();
    auto oPacket = this->m_convertAudio->convert(packet);
    this->m_mutex.unlock();

    akSend(oPacket)
}

bool ACapsConvertElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
        case AkElement::ElementStatePlaying: {
            if (!this->m_convertAudio->init(this->m_caps))
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
            this->m_convertAudio->uninit();

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
            this->m_convertAudio->uninit();

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

    this->m_mutex.lock();

    this->m_convertAudio =
            ptr_init<ConvertAudio>(this->loadSubModule("ACapsConvert",
                                                       convertLib));
    this->m_mutex.unlock();

    this->setState(state);
}
