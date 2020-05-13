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

class AkCapsPrivate
{
    public:
        QString m_mimeType;
};

AkCaps::AkCaps(const QString &mimeType, QObject *parent):
    QObject(parent)
{
    this->d = new AkCapsPrivate();
    this->d->m_mimeType = mimeType;
}

AkCaps::AkCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCapsPrivate();
    this->d->m_mimeType = other.d->m_mimeType;
    this->update(other);
}

AkCaps::~AkCaps()
{
    delete this->d;
}

AkCaps &AkCaps::operator =(const AkCaps &other)
{
    if (this != &other) {
        this->d->m_mimeType = other.d->m_mimeType;
        this->clear();
        this->update(other);
    }

    return *this;
}

bool AkCaps::operator ==(const AkCaps &other) const
{
    if (this->dynamicPropertyNames() != other.dynamicPropertyNames())
        return false;

    for (auto &property: this->dynamicPropertyNames())
        if (this->property(property) != other.property(property))
            return false;

    return this->d->m_mimeType == other.d->m_mimeType;
}

bool AkCaps::operator !=(const AkCaps &other) const
{
    return !(*this == other);
}

QObject *AkCaps::create(const QString &mimeType)
{
    return new AkCaps(mimeType);
}

QObject *AkCaps::create(const AkCaps &caps)
{
    return new AkCaps(caps);
}

QVariant AkCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

AkCaps::operator bool() const
{
    return !this->d->m_mimeType.isEmpty();
}

QString AkCaps::mimeType() const
{
    return this->d->m_mimeType;
}

AkCaps AkCaps::fromMap(const QVariantMap &caps)
{
    AkCaps akCaps;

    if (!caps.contains("mimeType"))
        return akCaps;

    for (auto it = caps.begin(); it != caps.end(); it++)
        akCaps.setProperty(it.key().toStdString().c_str(), it.value());

    return akCaps;
}

QVariantMap AkCaps::toMap() const
{
    QVariantMap map {
        {"mimeType", this->d->m_mimeType},
    };

    for (auto &property: this->dynamicPropertyNames()) {
        auto key = QString::fromUtf8(property.constData());
        map[key] = this->property(property);
    }

    return map;
}

AkCaps &AkCaps::update(const AkCaps &other)
{
    if (this->d->m_mimeType != other.d->m_mimeType)
        return *this;

    for (auto &property: other.dynamicPropertyNames())
        this->setProperty(property.constData(),
                          other.property(property.constData()));

    return *this;
}

bool AkCaps::isCompatible(const AkCaps &other) const
{
    if (this->d->m_mimeType != other.d->m_mimeType)
        return false;

    for (auto &property: other.dynamicPropertyNames())
        if (!this->dynamicPropertyNames().contains(property) ||
            this->property(property.constData()) != other.property(property.constData()))
            return false;

    return true;
}

bool AkCaps::contains(const QString &property) const
{
    return this->dynamicPropertyNames().contains(property.toUtf8());
}

void AkCaps::setMimeType(const QString &mimeType)
{
    QString _mimeType = mimeType.trimmed();

    if (this->d->m_mimeType == _mimeType)
        return;

    this->d->m_mimeType = _mimeType;
    emit this->mimeTypeChanged(this->d->m_mimeType);
}

void AkCaps::resetMimeType()
{
    this->setMimeType("");
}

void AkCaps::clear()
{
    for (auto &property: this->dynamicPropertyNames())
        this->setProperty(property.constData(), QVariant());
}

void AkCaps::registerTypes()
{
    qRegisterMetaType<AkCaps>("AkCaps");
    qRegisterMetaTypeStreamOperators<AkCaps>("AkCaps");
    qRegisterMetaType<CapsType>("CapsType");
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
    debug.nospace() << "AkCaps("
                    << "mimeType="
                    << caps.mimeType();

    QStringList properties;

    for (auto &property: caps.dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    for (auto &property: properties)
        debug.nospace() << ","
                        << property.toStdString().c_str()
                        << "="
                        << caps.property(property.toStdString().c_str());

    debug.nospace() << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCaps &caps)
{
    int nProperties;
    istream >> nProperties;

    for (int i = 0; i < nProperties; i++) {
        QByteArray key;
        QVariant value;
        istream >> key;
        istream >> value;

        caps.setProperty(key.toStdString().c_str(), value);
    }

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps)
{
    QVariantMap staticProperties {
        {"mimeType", caps.mimeType()},
    };

    int nProperties =
            staticProperties.size() + caps.dynamicPropertyNames().size();
    ostream << nProperties;

    for (auto &key: caps.dynamicPropertyNames()) {
        ostream << key;
        ostream << caps.property(key);
    }

    return ostream;
}

#include "moc_akcaps.cpp"
