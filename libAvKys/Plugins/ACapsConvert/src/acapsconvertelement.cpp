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

Q_GLOBAL_STATIC_WITH_ARGS(QStringList, preferredFramework, ({"ffmpeg", "gstreamer"}))

template<typename T>
inline QSharedPointer<T> ptr_init(QObject *obj=nullptr)
{
    if (!obj)
        return QSharedPointer<T>(new T());

    return QSharedPointer<T>(dynamic_cast<T *>(obj));
}

ACapsConvertElement::ACapsConvertElement():
    AkElement(),
    m_convertAudio(ptr_init<ConvertAudio>())
{
    QObject::connect(this,
                     &ACapsConvertElement::convertLibChanged,
                     this,
                     &ACapsConvertElement::convertLibUpdated);

    this->resetConvertLib();
}

QString ACapsConvertElement::caps() const
{
    return this->m_caps.toString();
}

QString ACapsConvertElement::convertLib() const
{
    return this->m_convertLib;
}

void ACapsConvertElement::setCaps(const QString &caps)
{
    if (this->m_caps == caps)
        return;

    this->m_mutex.lock();
    this->m_caps = caps;
    this->m_mutex.unlock();
    emit this->capsChanged(caps);
}

void ACapsConvertElement::setConvertLib(const QString &convertLib)
{
    if (this->m_convertLib == convertLib)
        return;

    this->m_convertLib = convertLib;
    emit this->convertLibChanged(convertLib);
}

void ACapsConvertElement::resetCaps()
{
    this->setCaps("");
}

void ACapsConvertElement::resetConvertLib()
{
    auto subModules = this->listSubModules("ACapsConvert");

    for (const QString &framework: *preferredFramework)
        if (subModules.contains(framework)) {
            this->setConvertLib(framework);

            return;
        }

    if (this->m_convertLib.isEmpty() && !subModules.isEmpty())
        this->setConvertLib(subModules.first());
    else
        this->setConvertLib("");
}

AkPacket ACapsConvertElement::iStream(const AkAudioPacket &packet)
{
    this->m_mutex.lock();
    auto caps = this->m_caps;
    this->m_mutex.unlock();

    this->m_mutex.lock();
    auto oPacket = this->m_convertAudio->convert(packet, caps);
    this->m_mutex.unlock();

    akSend(oPacket)
}

void ACapsConvertElement::convertLibUpdated(const QString &convertLib)
{
    this->m_mutex.lock();

    this->m_convertAudio =
            ptr_init<ConvertAudio>(this->loadSubModule("ACapsConvert",
                                                       convertLib));
    this->m_mutex.unlock();
}
