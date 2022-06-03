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
                                     const AkColorPlanes &planes)
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

AkColorPlanes AkVideoFormatSpec::planes() const
{
    return this->d->m_planes;
}

int AkVideoFormatSpec::bpp() const
{
    static const int k = 16;
    int bpp = 0;

    for (auto &plane: this->d->m_planes)
        for (auto &component: plane)
            bpp += k * component.length()
                   / (1 << (component.widthDiv() + component.heightDiv()));

    return bpp / k;
}

AkColorComponent AkVideoFormatSpec::component(AkColorComponent::ComponentType componentType) const
{
    for (auto &plane: this->d->m_planes)
        for (auto &component: plane)
            if (component.type() == componentType)
                return component;

    return {};
}

int AkVideoFormatSpec::componentPlane(AkColorComponent::ComponentType component) const
{
    for (int plane = 0; plane < this->d->m_planes.size(); plane++)
        for (auto &component_: this->d->m_planes[plane])
            if (component_.type() == component)
                return plane;

    return -1;
}

bool AkVideoFormatSpec::contains(AkColorComponent::ComponentType component) const
{
    for (int plane = 0; plane < this->d->m_planes.size(); plane++)
        for (auto &component_: this->d->m_planes[plane])
            if (component_.type() == component)
                return true;

    return false;
}

size_t AkVideoFormatSpec::rlength() const
{
    for (auto &plane: this->d->m_planes)
        for (auto &component: plane)
            return component.rlength();

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

void AkVideoFormatSpec::setType(VideoFormatType type)
{
    if (this->d->m_type == type)
        return;

    this->d->m_type = type;
    emit this->typeChanged(type);
}

void AkVideoFormatSpec::setEndianness(int endianness)
{
    if (this->d->m_endianness == endianness)
        return;

    this->d->m_endianness = endianness;
    emit this->endiannessChanged(endianness);
}

void AkVideoFormatSpec::setPlanes(const AkColorPlanes &planes)
{
    if (this->d->m_planes == planes)
        return;

    this->d->m_planes = planes;
    emit this->planesChanged(planes);
}

void AkVideoFormatSpec::resetType()
{
    this->setType(VFT_Unknown);
}

void AkVideoFormatSpec::resetEndianness()
{
    this->setEndianness(Q_BYTE_ORDER);
}

void AkVideoFormatSpec::resetPlanes()
{
    this->setPlanes({});
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
    int nProperties;
    istream >> nProperties;

    for (int i = 0; i < nProperties; i++) {
        QByteArray key;
        QVariant value;
        istream >> key;
        istream >> value;

        spec.setProperty(key.toStdString().c_str(), value);
    }

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoFormatSpec &spec)
{
    QVariantMap staticProperties {
        {"type"       , spec.type()                       },
        {"endianness" , spec.endianness()                 },
        {"planes"     , QVariant::fromValue(spec.planes())},
    };

    ostream << staticProperties.size();

    for (auto it = staticProperties.begin();
         it != staticProperties.end();
         it++) {
        ostream << it.key();
        ostream << spec.property(it.key().toUtf8());
    }

    return ostream;
}

#include "moc_akvideoformatspec.cpp"
