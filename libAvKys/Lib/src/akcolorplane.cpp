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

#include "akcolorplane.h"

class AkColorPlanePrivate
{
    public:
        AkColorComponentList m_components;
        size_t m_bitsSize {0};
};

AkColorPlane::AkColorPlane(QObject *parent):
    QObject(parent)
{
    this->d = new AkColorPlanePrivate();
}

AkColorPlane::AkColorPlane(const AkColorComponentList &components,
                           size_t bitsSize)
{
    this->d = new AkColorPlanePrivate();
    this->d->m_components = components;
    this->d->m_bitsSize = bitsSize;
}

AkColorPlane::AkColorPlane(const AkColorPlane &other):
    QObject()
{
    this->d = new AkColorPlanePrivate();
    this->d->m_components = other.d->m_components;
    this->d->m_bitsSize = other.d->m_bitsSize;
}

AkColorPlane::~AkColorPlane()
{
    delete this->d;
}

AkColorPlane &AkColorPlane::operator =(const AkColorPlane &other)
{
    if (this != &other) {
        this->d->m_components = other.d->m_components;
        this->d->m_bitsSize = other.d->m_bitsSize;
    }

    return *this;
}

bool AkColorPlane::operator ==(const AkColorPlane &other) const
{
    return this->d->m_components == other.d->m_components
           && this->d->m_bitsSize == other.d->m_bitsSize;
}

bool AkColorPlane::operator !=(const AkColorPlane &other) const
{
    return !(*this == other);
}

QObject *AkColorPlane::create()
{
    return new AkColorComponent();
}

QObject *AkColorPlane::create(const AkColorPlane &colorComponent)
{
    return new AkColorPlane(colorComponent);
}

QObject *AkColorPlane::create(const AkColorComponentList &components,
                              size_t bitsSize)
{
    return new AkColorPlane(components, bitsSize);
}

QVariant AkColorPlane::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkColorComponentList AkColorPlane::components() const
{
    return this->d->m_components;
}

size_t AkColorPlane::bitsSize() const
{
    return this->d->m_bitsSize;
}

size_t AkColorPlane::heightDiv() const
{
    size_t heightDiv = 0;

    for (auto &component: this->d->m_components)
        heightDiv = qMax(heightDiv, component.heightDiv());

    return heightDiv;
}

void AkColorPlane::setComponents(const AkColorComponentList &components)
{
    if (this->d->m_components == components)
        return;

    this->d->m_components = components;
    emit this->componentsChanged(components);
}

void AkColorPlane::setBitsSize(size_t bitsSize)
{
    if (this->d->m_bitsSize == bitsSize)
        return;

    this->d->m_bitsSize = bitsSize;
    emit this->bitsSizeChanged(bitsSize);
}

void AkColorPlane::resetComponents()
{
    this->setComponents({});
}

void AkColorPlane::resetBitsSize()
{
    this->setBitsSize(0);
}

void AkColorPlane::registerTypes()
{
    qRegisterMetaType<AkColorPlane>("AkColorPlane");
    qRegisterMetaType<AkColorPlanes>("AkColorPlanes");
    qRegisterMetaTypeStreamOperators<AkColorPlane>("AkColorPlane");
    qmlRegisterSingletonType<AkColorPlane>("Ak", 1, 0, "AkColorPlane",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkColorPlane();
    });
}

QDebug operator <<(QDebug debug, const AkColorPlane &colorPlane)
{
    debug.nospace() << "AkColorPlane("
                    << "components="
                    << colorPlane.components()
                    << ",bitsSize="
                    << colorPlane.bitsSize()
                    << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkColorPlane &colorPlane)
{
    int nComponents = 0;
    istream >> nComponents;
    AkColorComponentList components;

    for (int i = 0; i < nComponents; i++) {
        AkColorComponent component;
        istream >> component;
        components << component;
    }

    colorPlane.setComponents(components);
    int bitsSize = 0;
    istream >> bitsSize;
    colorPlane.setBitsSize(bitsSize);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkColorPlane &colorPlane)
{
    ostream << colorPlane.components().size();

    for (auto &component: colorPlane.components())
        ostream << component;

    AkColorComponentList m_components;
    ostream << int(colorPlane.bitsSize());

    return ostream;
}

#include "moc_akcolorplane.cpp"
