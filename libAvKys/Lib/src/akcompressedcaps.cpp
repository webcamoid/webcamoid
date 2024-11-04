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

#include <QDataStream>
#include <QDebug>
#include <QStringList>
#include <QVariant>
#include <QQmlEngine>

#include "akcompressedcaps.h"
#include "akcompressedaudiocaps.h"
#include "akcompressedvideocaps.h"

class AkCompressedCapsPrivate
{
    public:
        AkCompressedCaps::CapsType m_type {AkCompressedCaps::CapsType_Unknown};
        AkCodecID m_codecID {0};
        void *m_privateData {nullptr};
        AkCompressedCaps::DataCopy m_copyFunc {nullptr};
        AkCompressedCaps::DataDeleter m_deleterFunc {nullptr};
};

AkCompressedCaps::AkCompressedCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkCompressedCapsPrivate();
}

AkCompressedCaps::AkCompressedCaps(const AkCompressedCaps &other):
    QObject()
{
    this->d = new AkCompressedCapsPrivate();
    this->d->m_type = other.d->m_type;
    this->d->m_codecID = other.d->m_codecID;

    if (other.d->m_privateData && other.d->m_copyFunc)
        this->d->m_privateData = other.d->m_copyFunc(other.d->m_privateData);

    this->d->m_copyFunc = other.d->m_copyFunc;
    this->d->m_deleterFunc = other.d->m_deleterFunc;
}

AkCompressedCaps::~AkCompressedCaps()
{
    delete this->d;
}

AkCompressedCaps &AkCompressedCaps::operator =(const AkCompressedCaps &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;
        this->d->m_codecID = other.d->m_codecID;

        if (this->d->m_privateData && this->d->m_copyFunc) {
            this->d->m_deleterFunc(this->d->m_privateData);
            this->d->m_privateData = nullptr;
        }

        if (other.d->m_privateData && other.d->m_copyFunc)
            this->d->m_privateData = other.d->m_copyFunc(other.d->m_privateData);

        this->d->m_copyFunc = other.d->m_copyFunc;
        this->d->m_deleterFunc = other.d->m_deleterFunc;
    }

    return *this;
}

bool AkCompressedCaps::operator ==(const AkCompressedCaps &other) const
{
    if (this->d->m_type != other.d->m_type
        || this->d->m_codecID != other.d->m_codecID)
        return false;

    if (this->d->m_privateData == other.d->m_privateData)
        return true;

    switch (this->d->m_type) {
        case CapsType_Audio:
            return AkCompressedAudioCaps(*this) == AkCompressedAudioCaps(other);

        case CapsType_Video:
            return AkCompressedVideoCaps(*this) == AkCompressedVideoCaps(other);

        default:
            break;
    }

    return false;
}

bool AkCompressedCaps::operator !=(const AkCompressedCaps &other) const
{
    return !(*this == other);
}

AkCompressedCaps::operator bool() const
{
    if (this->d->m_type == CapsType_Unknown || !this->d->m_privateData)
        return false;

    switch (this->d->m_type) {
        case CapsType_Audio:
            return AkCompressedAudioCaps(*this);

        case CapsType_Video:
            return AkCompressedVideoCaps(*this);

        default:
            break;
    }

    return true;
}

QObject *AkCompressedCaps::create()
{
    return new AkCompressedCaps();
}

QObject *AkCompressedCaps::create(const AkCompressedCaps &caps)
{
    return new AkCompressedCaps(caps);
}

QVariant AkCompressedCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkCompressedCaps::CapsType AkCompressedCaps::type() const
{
    return this->d->m_type;
}

AkCodecID AkCompressedCaps::codecID() const
{
    return this->d->m_codecID;
}

void *AkCompressedCaps::privateData() const
{
    return this->d->m_privateData;
}

void AkCompressedCaps::setPrivateData(void *data,
                                      DataCopy copyFunc,
                                      DataDeleter deleterFunc)
{
    this->d->m_privateData = data;
    this->d->m_copyFunc = copyFunc;
    this->d->m_deleterFunc = deleterFunc;
}

void AkCompressedCaps::setType(CapsType type)
{
    this->d->m_type = type;
}

void AkCompressedCaps::setCodecID(AkCodecID codecID)
{
    this->d->m_codecID = codecID;
}

void AkCompressedCaps::registerTypes()
{
    qRegisterMetaType<AkCompressedCaps>("AkCompressedCaps");
    qmlRegisterSingletonType<AkCompressedCaps>("Ak", 1, 0, "AkCompressedCaps",
                                               [] (QQmlEngine *qmlEngine,
                                                   QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedCaps();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedCaps &caps)
{
    debug.nospace() << "AkCompressedCaps(";

    switch (caps.d->m_type) {
        case AkCompressedCaps::CapsType_Audio:
            debug.nospace() << *reinterpret_cast<AkCompressedAudioCaps *>(caps.d->m_privateData);
            break;
        case AkCompressedCaps::CapsType_Video:
            debug.nospace() << *reinterpret_cast<AkCompressedVideoCaps *>(caps.d->m_privateData);
            break;
        default:
            break;
    }

    debug.nospace() << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCompressedCaps &caps)
{
    AkCompressedCaps::CapsType type;
    istream >> type;

    switch (type) {
        case AkCompressedCaps::CapsType_Audio: {
            AkCompressedAudioCaps audioCaps;
            istream >> audioCaps;
            caps = audioCaps;
            break;
        }
        case AkCompressedCaps::CapsType_Video: {
            AkCompressedVideoCaps videoCaps;
            istream >> videoCaps;
            caps = videoCaps;
            break;
        }
        default:
            break;
    }

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCompressedCaps &caps)
{
    ostream << caps.d->m_type;

    switch (caps.d->m_type) {
        case AkCompressedCaps::CapsType_Audio:
            ostream << *reinterpret_cast<AkCompressedAudioCaps *>(caps.d->m_privateData);
            break;
        case AkCompressedCaps::CapsType_Video:
            ostream << *reinterpret_cast<AkCompressedVideoCaps *>(caps.d->m_privateData);
            break;
        default:
            break;
    }

    return ostream;
}

#include "moc_akcompressedcaps.cpp"
