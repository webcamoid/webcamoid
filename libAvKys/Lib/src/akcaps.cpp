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

#include <QDataStream>
#include <QDebug>
#include <QStringList>
#include <QVariant>
#include <QQmlEngine>

#include "akcaps.h"
#include "akaudiocaps.h"
#include "akcompressedvideocaps.h"
#include "aksubtitlecaps.h"
#include "akvideocaps.h"

class AkCapsPrivate
{
    public:
        AkCaps::CapsType m_type {AkCaps::CapsUnknown};
        void *m_privateData {nullptr};
        AkCaps::DataCopy m_copyFunc {nullptr};
        AkCaps::DataDeleter m_deleterFunc {nullptr};
};

AkCaps::AkCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkCapsPrivate();
}

AkCaps::AkCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCapsPrivate();
    this->d->m_type = other.d->m_type;

    if (other.d->m_privateData && other.d->m_copyFunc)
        this->d->m_privateData = other.d->m_copyFunc(other.d->m_privateData);

    this->d->m_copyFunc = other.d->m_copyFunc;
    this->d->m_deleterFunc = other.d->m_deleterFunc;
}

AkCaps::~AkCaps()
{
    if (this->d->m_privateData && this->d->m_copyFunc)
        this->d->m_deleterFunc(this->d->m_privateData);

    delete this->d;
}

AkCaps &AkCaps::operator =(const AkCaps &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;

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

bool AkCaps::operator ==(const AkCaps &other) const
{
    if (this->d->m_type != other.d->m_type)
        return false;

    if (this->d->m_privateData == other.d->m_privateData)
        return true;

    switch (this->d->m_type) {
    case CapsAudio:
        if (AkAudioCaps(*this) == AkAudioCaps(other))
            return false;

        break;
    case CapsVideoCompressed:
        if (AkCompressedVideoCaps(*this) == AkCompressedVideoCaps(other))
            return false;

        break;
    case CapsSubtitle:
        if (AkSubtitleCaps(*this) == AkSubtitleCaps(other))
            return false;

        break;
    case CapsVideo:
        if (AkVideoCaps(*this) == AkVideoCaps(other))
            return false;

        break;
    default:
        break;
    }

    return false;
}

bool AkCaps::operator !=(const AkCaps &other) const
{
    return !(*this == other);
}

AkCaps::operator bool() const
{
    if (this->d->m_type == CapsUnknown || !this->d->m_privateData)
        return false;

    switch (this->d->m_type) {
    case CapsAudio:
        if (!AkAudioCaps(*this))
            return false;

        break;
    case CapsVideoCompressed:
        if (!AkCompressedVideoCaps(*this))
            return false;

        break;
    case CapsSubtitle:
        if (!AkSubtitleCaps(*this))
            return false;

        break;
    case CapsVideo:
        if (!AkVideoCaps(*this))
            return false;

        break;
    default:
        break;
    }

    return true;
}

QObject *AkCaps::create()
{
    return new AkCaps;
}

QObject *AkCaps::create(const AkCaps &caps)
{
    return new AkCaps(caps);
}

QVariant AkCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkCaps::CapsType AkCaps::type() const
{
    return this->d->m_type;
}

void *AkCaps::privateData() const
{
    return this->d->m_privateData;
}

void AkCaps::setPrivateData(void *data, DataCopy copyFunc, DataDeleter deleterFunc)
{
    this->d->m_privateData = data;
    this->d->m_copyFunc = copyFunc;
    this->d->m_deleterFunc = deleterFunc;
}

void AkCaps::setType(CapsType type)
{
    this->d->m_type = type;
}

void AkCaps::registerTypes()
{
    qRegisterMetaType<AkCaps>("AkCaps");
    qRegisterMetaTypeStreamOperators<AkCaps>("AkCaps");
    qmlRegisterSingletonType<AkCaps>("Ak", 1, 0, "AkCaps",
                                     [] (QQmlEngine *qmlEngine,
                                         QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCaps();
    });
}

QDebug operator <<(QDebug debug, const AkCaps &caps)
{
    debug.nospace() << "AkCaps(";

    switch (caps.d->m_type) {
    case AkCaps::CapsAudio:
        debug.nospace() << *reinterpret_cast<AkAudioCaps *>(caps.d->m_privateData);
        break;
    case AkCaps::CapsVideoCompressed:
        debug.nospace() << *reinterpret_cast<AkCompressedVideoCaps *>(caps.d->m_privateData);
        break;
    case AkCaps::CapsSubtitle:
        debug.nospace() << *reinterpret_cast<AkSubtitleCaps *>(caps.d->m_privateData);
        break;
    case AkCaps::CapsVideo:
        debug.nospace() << *reinterpret_cast<AkVideoCaps *>(caps.d->m_privateData);
        break;
    default:
        break;
    }

    debug.nospace() << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCaps &caps)
{
    AkCaps::CapsType type;
    istream >> type;

    switch (type) {
    case AkCaps::CapsAudio: {
        AkAudioCaps audioCaps;
        istream >> audioCaps;
        caps = audioCaps;
        break;
    }
    case AkCaps::CapsVideoCompressed: {
        AkCompressedVideoCaps videoCaps;
        istream >> videoCaps;
        caps = videoCaps;
        break;
    }
    case AkCaps::CapsSubtitle: {
        AkSubtitleCaps subtitleCaps;
        istream >> subtitleCaps;
        caps = subtitleCaps;
        break;
    }
    case AkCaps::CapsVideo: {
        AkVideoCaps videoCaps;
        istream >> videoCaps;
        caps = videoCaps;
        break;
    }
    default:
        break;
    }

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps)
{
    ostream << caps.d->m_type;

    switch (caps.d->m_type) {
    case AkCaps::CapsAudio:
        ostream << *reinterpret_cast<AkAudioCaps *>(caps.d->m_privateData);
        break;
    case AkCaps::CapsVideoCompressed:
        ostream << *reinterpret_cast<AkCompressedVideoCaps *>(caps.d->m_privateData);
        break;
    case AkCaps::CapsSubtitle:
        ostream << *reinterpret_cast<AkSubtitleCaps *>(caps.d->m_privateData);
        break;
    case AkCaps::CapsVideo:
        ostream << *reinterpret_cast<AkVideoCaps *>(caps.d->m_privateData);
        break;
    default:
        break;
    }

    return ostream;
}

#include "moc_akcaps.cpp"

