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

#include "akcolorcomponent.h"

class AkColorComponentPrivate
{
    public:
        AkColorComponent::ComponentType m_type {AkColorComponent::ComponentType(0)};
        size_t m_step {0};       // Bytes to increment for reading th next pixel.
        size_t m_offset {0};     // Bytes to skip before reading the component.
        size_t m_shift {0};      // Shift the value n-bits to the left before reading the component.
        size_t m_byteLength {0}; // Read n-bytes for the value.
        size_t m_length {0};     // Size of the component in bits.
        size_t m_widthDiv {0};   // Plane width should be divided by 2^widthDiv
        size_t m_heightDiv {0};  // Plane height should be divided by 2^heightDiv
};

AkColorComponent::AkColorComponent(QObject *parent):
    QObject(parent)
{
    this->d = new AkColorComponentPrivate();
}

AkColorComponent::AkColorComponent(ComponentType type,
                                   size_t step,
                                   size_t offset,
                                   size_t shift,
                                   size_t byteLength,
                                   size_t length,
                                   size_t widthDiv,
                                   size_t heightDiv)
{
    this->d = new AkColorComponentPrivate();
    this->d->m_type = type;
    this->d->m_step = step;
    this->d->m_offset = offset;
    this->d->m_shift = shift;
    this->d->m_byteLength = byteLength;
    this->d->m_length = length;
    this->d->m_widthDiv = widthDiv;
    this->d->m_heightDiv = heightDiv;
}

AkColorComponent::AkColorComponent(const AkColorComponent &other):
    QObject()
{
    this->d = new AkColorComponentPrivate();
    this->d->m_type = other.d->m_type;
    this->d->m_step = other.d->m_step;
    this->d->m_offset = other.d->m_offset;
    this->d->m_shift = other.d->m_shift;
    this->d->m_byteLength = other.d->m_byteLength;
    this->d->m_length = other.d->m_length;
    this->d->m_widthDiv = other.d->m_widthDiv;
    this->d->m_heightDiv = other.d->m_heightDiv;
}

AkColorComponent::~AkColorComponent()
{
    delete this->d;
}

AkColorComponent &AkColorComponent::operator =(const AkColorComponent &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;
        this->d->m_step = other.d->m_step;
        this->d->m_offset = other.d->m_offset;
        this->d->m_shift = other.d->m_shift;
        this->d->m_byteLength = other.d->m_byteLength;
        this->d->m_length = other.d->m_length;
        this->d->m_widthDiv = other.d->m_widthDiv;
        this->d->m_heightDiv = other.d->m_heightDiv;
    }

    return *this;
}

bool AkColorComponent::operator ==(const AkColorComponent &other) const
{
    return this->d->m_type == other.d->m_type
           && this->d->m_step == other.d->m_step
           && this->d->m_offset == other.d->m_offset
           && this->d->m_shift == other.d->m_shift
           && this->d->m_byteLength == other.d->m_byteLength
           && this->d->m_length == other.d->m_length
           && this->d->m_widthDiv == other.d->m_widthDiv
           && this->d->m_heightDiv == other.d->m_heightDiv;
}

bool AkColorComponent::operator !=(const AkColorComponent &other) const
{
    return !(*this == other);
}

QObject *AkColorComponent::create()
{
    return new AkColorComponent();
}

QObject *AkColorComponent::create(const AkColorComponent &colorComponent)
{
    return new AkColorComponent(colorComponent);
}

QObject *AkColorComponent::create(ComponentType type,
                                  size_t step,
                                  size_t offset,
                                  size_t shift,
                                  size_t byteLength,
                                  size_t length,
                                  size_t widthDiv,
                                  size_t heightDiv)
{
    return new AkColorComponent(type,
                                step,
                                offset,
                                shift,
                                byteLength,
                                length,
                                widthDiv,
                                heightDiv);
}

QVariant AkColorComponent::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkColorComponent::ComponentType AkColorComponent::type() const
{
    return this->d->m_type;
}

size_t AkColorComponent::step() const
{
    return this->d->m_step;
}

size_t AkColorComponent::offset() const
{
    return this->d->m_offset;
}

size_t AkColorComponent::shift() const
{
    return this->d->m_shift;
}

size_t AkColorComponent::byteLength() const
{
    return this->d->m_byteLength;
}

size_t AkColorComponent::length() const
{
    return this->d->m_length;
}

size_t AkColorComponent::widthDiv() const
{
    return this->d->m_widthDiv;
}

size_t AkColorComponent::heightDiv() const
{
    return this->d->m_heightDiv;
}

void AkColorComponent::registerTypes()
{
    qRegisterMetaType<AkColorComponent>("AkColorComponent");
    qRegisterMetaType<AkColorComponentList>("AkColorComponentList");
    qRegisterMetaTypeStreamOperators<AkColorComponent>("AkColorComponent");
    qRegisterMetaType<ComponentType>("AkColorComponentType");
    QMetaType::registerDebugStreamOperator<ComponentType>();
    qmlRegisterSingletonType<AkColorComponent>("Ak", 1, 0, "AkColorComponent",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkColorComponent();
    });
}

QDebug operator <<(QDebug debug, const AkColorComponent &colorComponent)
{
    debug.nospace() << "AkColorComponent("
                    << "type="
                    << colorComponent.type()
                    << ",step="
                    << colorComponent.step()
                    << ",offset="
                    << colorComponent.offset()
                    << ",shift="
                    << colorComponent.shift()
                    << ",byteLength="
                    << colorComponent.byteLength()
                    << ",length="
                    << colorComponent.length()
                    << ",widthDiv="
                    << colorComponent.widthDiv()
                    << ",heightDiv="
                    << colorComponent.heightDiv()
                    << ")";

    return debug.space();
}

QDebug operator <<(QDebug debug, AkColorComponent::ComponentType type)
{
    AkColorComponent colorComponent;
    int componentTypeIndex =
            colorComponent.metaObject()->indexOfEnumerator("ComponentType");
    auto typeEnum = colorComponent.metaObject()->enumerator(componentTypeIndex);
    QString format(typeEnum.valueToKey(type));
    format.remove("CT_");
    debug.nospace() << format.toStdString().c_str();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkColorComponent &colorComponent)
{
    auto type = AkColorComponent::ComponentType(0);
    istream >> type;
    int step = 0;
    istream >> step;
    int offset = 0;
    istream >> offset;
    int shift = 0;
    istream >> shift;
    int byteLength = 0;
    istream >> byteLength;
    int length = 0;
    istream >> length;
    int widthDiv = 0;
    istream >> widthDiv;
    int heightDiv = 0;
    istream >> heightDiv;

    colorComponent = {type,
                      size_t(step),
                      size_t(offset),
                      size_t(shift),
                      size_t(byteLength),
                      size_t(length),
                      size_t(widthDiv),
                      size_t(heightDiv)};

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkColorComponent &colorComponent)
{
    ostream << colorComponent.type();
    ostream << int(colorComponent.step());
    ostream << int(colorComponent.offset());
    ostream << int(colorComponent.shift());
    ostream << int(colorComponent.byteLength());
    ostream << int(colorComponent.length());
    ostream << int(colorComponent.widthDiv());
    ostream << int(colorComponent.heightDiv());

    return ostream;
}

#include "moc_akcolorcomponent.cpp"
