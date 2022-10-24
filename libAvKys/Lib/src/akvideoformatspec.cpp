/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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
#include <QMetaEnum>
#include <QQmlEngine>

#include "akvideoformatspec.h"

class AkVideoFormatSpecPrivate
{
    public:
        AkVideoFormatSpec::VideoFormatType m_type {AkVideoFormatSpec::VFT_Unknown};
        int m_endianness {Q_BYTE_ORDER};
        AkColorPlanes m_planes;
};

AkVideoFormatSpec::AkVideoFormatSpec(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoFormatSpecPrivate();
}

AkVideoFormatSpec::AkVideoFormatSpec(VideoFormatType type,
                                     int endianness,
                                     const AkColorPlanes &planes):
    QObject()
{
    this->d = new AkVideoFormatSpecPrivate();
    this->d->m_type = type;
    this->d->m_endianness = endianness;
    this->d->m_planes = planes;
}

AkVideoFormatSpec::AkVideoFormatSpec(const AkVideoFormatSpec &other):
    QObject()
{
    this->d = new AkVideoFormatSpecPrivate();
    this->d->m_type = other.d->m_type;
    this->d->m_endianness = other.d->m_endianness;
    this->d->m_planes = other.d->m_planes;
}

AkVideoFormatSpec::~AkVideoFormatSpec()
{
    delete this->d;
}

AkVideoFormatSpec &AkVideoFormatSpec::operator =(const AkVideoFormatSpec &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;
        this->d->m_endianness = other.d->m_endianness;
        this->d->m_planes = other.d->m_planes;
    }

    return *this;
}

bool AkVideoFormatSpec::operator ==(const AkVideoFormatSpec &other) const
{
    return this->d->m_type == other.d->m_type
            && this->d->m_endianness == other.d->m_endianness
            && this->d->m_planes == other.d->m_planes;
}

bool AkVideoFormatSpec::operator !=(const AkVideoFormatSpec &other) const
{
    return !(*this == other);
}

QObject *AkVideoFormatSpec::create()
{
    return new AkVideoFormatSpec();
}

QObject *AkVideoFormatSpec::create(const AkVideoFormatSpec &other)
{
    return new AkVideoFormatSpec(other);
}

QObject *AkVideoFormatSpec::create(VideoFormatType type,
                                   int endianness,
                                   const AkColorPlanes &planes)
{
    return new AkVideoFormatSpec(type, endianness, planes);
}

QVariant AkVideoFormatSpec::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkVideoFormatSpec::VideoFormatType AkVideoFormatSpec::type() const
{
    return this->d->m_type;
}

int AkVideoFormatSpec::endianness() const
{
    return this->d->m_endianness;
}

size_t AkVideoFormatSpec::planes() const
{
    return this->d->m_planes.size();
}

const AkColorPlane &AkVideoFormatSpec::plane(size_t plane) const
{
    return this->d->m_planes[plane];
}

int AkVideoFormatSpec::bpp() const
{
    int bpp = 0;

    for (auto &plane: this->d->m_planes)
        bpp += plane.bitsSize();

    return bpp;
}

AkColorComponent AkVideoFormatSpec::component(AkColorComponent::ComponentType componentType) const
{
    for (auto &plane: this->d->m_planes)
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component = plane.component(i);

            if (component.type() == componentType)
                return component;
        }

    return {};
}

int AkVideoFormatSpec::componentPlane(AkColorComponent::ComponentType component) const
{
    int planeIndex = 0;

    for (auto &plane: this->d->m_planes) {
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component_ = plane.component(i);

            if (component_.type() == component)
                return planeIndex;
        }

        planeIndex++;
    }

    return -1;
}

bool AkVideoFormatSpec::contains(AkColorComponent::ComponentType component) const
{
    for (auto &plane: this->d->m_planes)
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component_ = plane.component(i);

            if (component_.type() == component)
                return true;
        }

    return false;
}

size_t AkVideoFormatSpec::byteLength() const
{
    for (auto &plane: this->d->m_planes)
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component = plane.component(i);

            return component.byteLength();
        }

    return 0;
}

size_t AkVideoFormatSpec::numberOfComponents() const
{
    auto n = this->mainComponents();

    if (this->contains(AkColorComponent::CT_A))
        n++;

    return n;
}

size_t AkVideoFormatSpec::mainComponents() const
{
    size_t n = 0;

    switch (this->d->m_type) {
    case VFT_RGB:
    case VFT_YUV:
        n = 3;

        break;

    case VFT_Gray:
        n = 1;

        break;

    default:
        break;
    }

    return n;
}

void AkVideoFormatSpec::registerTypes()
{
    qRegisterMetaType<AkVideoFormatSpec>("AkVideoFormatSpec");
    qRegisterMetaType<AkColorPlanes>("AkColorPlanes");
    qRegisterMetaTypeStreamOperators<AkVideoFormatSpec>("AkVideoFormatSpec");
    qRegisterMetaType<VideoFormatType>("AkVideoFormatType");
    QMetaType::registerDebugStreamOperator<VideoFormatType>();
    qmlRegisterSingletonType<AkVideoFormatSpec>("Ak", 1, 0, "AkVideoFormatSpec",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkVideoFormatSpec();
    });
}

QDebug operator <<(QDebug debug, const AkVideoFormatSpec &spec)
{
    debug.nospace() << "AkVideoFormatSpec("
                    << "type="
                    << spec.type()
                    << ",endianness="
                    << spec.endianness()
                    << ",planes="
                    << spec.planes()
                    << ",bpp="
                    << spec.bpp()
                    << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkVideoFormatSpec::VideoFormatType format)
{
    AkVideoFormatSpec spec;
    int videoFormatTypeIndex =
            spec.metaObject()->indexOfEnumerator("VideoFormatType");
    auto typeEnum = spec.metaObject()->enumerator(videoFormatTypeIndex);
    QString fmt(typeEnum.valueToKey(format));
    fmt.remove("VFT_");
    debug.nospace() << fmt.toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkVideoFormatSpec &spec)
{
    auto type = AkVideoFormatSpec::VFT_Unknown;
    istream >> type;
    int endianness = Q_BYTE_ORDER;
    istream >> endianness;
    int nPlanes = 0;
    istream >> nPlanes;
    AkColorPlanes planes;

    for (int i = 0; i < nPlanes; i++) {
        AkColorPlane plane;
        istream >> plane;
        planes << plane;
    }

    spec = {type, endianness, planes};

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoFormatSpec &spec)
{
    ostream << spec.type();
    ostream << spec.endianness();
    auto nPlanes = spec.planes();
    ostream << int(nPlanes);

    for (size_t i = 0; i < nPlanes; ++i)
        ostream << spec.plane(i);

    return ostream;
}

#include "moc_akvideoformatspec.cpp"
