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

#include <QRegExp>
#include <QStringList>
#include <QVariant>

#include "akcaps.h"

class AkCapsPrivate
{
    public:
        bool m_isValid;
        QString m_mimeType;
};

AkCaps::AkCaps(QObject *parent): QObject(parent)
{
    this->d = new AkCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_mimeType = "";
}

AkCaps::AkCaps(const QVariantMap &caps)
{
    this->d = new AkCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_mimeType = "";
    this->fromMap(caps);
}

AkCaps::AkCaps(const QString &caps)
{
    this->d = new AkCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_mimeType = "";
    this->fromString(caps);
}

AkCaps::AkCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
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
        this->clear();
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_mimeType = other.d->m_mimeType;
        this->update(other);
    }

    return *this;
}

AkCaps &AkCaps::operator =(const QString &other)
{
    return this->operator =(AkCaps(other));
}

bool AkCaps::operator ==(const AkCaps &other) const
{
    return this->toString() == other.toString();
}

bool AkCaps::operator ==(const QString &caps) const
{
    return this->toString() == caps;
}

bool AkCaps::operator !=(const AkCaps &other) const
{
    return !(*this == other);
}

bool AkCaps::operator !=(const QString &caps) const
{
    return !(*this == caps);
}

AkCaps::operator bool() const
{
    return this->d->m_isValid;
}

bool AkCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &AkCaps::isValid()
{
    return this->d->m_isValid;
}

QString AkCaps::mimeType() const
{
    return this->d->m_mimeType;
}

AkCaps &AkCaps::fromMap(const QVariantMap &caps)
{
    QList<QByteArray> properties = this->dynamicPropertyNames();

    for (const QByteArray &property: properties)
        this->setProperty(property, QVariant());

    if (!caps.contains("mimeType")) {
        this->d->m_isValid = false;
        this->d->m_mimeType = "";

        return *this;
    }

    for (const QString &key: caps.keys())
        if (key == "mimeType") {
            this->d->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*\\s*")
                                 .exactMatch(caps[key].toString());
            this->d->m_mimeType = caps[key].toString().trimmed();
        } else
            this->setProperty(key.trimmed().toStdString().c_str(), caps[key]);

    return *this;
}

AkCaps &AkCaps::fromString(const QString &caps)
{
    this->d->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*"
                                 "(?:\\s*,\\s*[a-zA-Z_]\\w*\\s*="
                                 "\\s*[^,=]+)*\\s*").exactMatch(caps);

    QList<QByteArray> properties = this->dynamicPropertyNames();

    for (const QByteArray &property: properties)
        this->setProperty(property, QVariant());

    QStringList capsChunks;

    if (this->d->m_isValid)
        capsChunks = caps.split(QRegExp("\\s*,\\s*"),
                                      QString::SkipEmptyParts);

    for (int i = 1; i < capsChunks.length(); i++) {
        QStringList pair = capsChunks[i].split(QRegExp("\\s*=\\s*"),
                                               QString::SkipEmptyParts);

        this->setProperty(pair[0].trimmed().toStdString().c_str(),
                          pair[1].trimmed());
    }

    this->setMimeType(this->d->m_isValid? capsChunks[0].trimmed(): QString(""));

    return *this;
}

QVariantMap AkCaps::toMap() const
{
    if (!this->d->m_isValid)
        return QVariantMap();

    QVariantMap caps;
    caps["mimeType"] = this->d->m_mimeType;

    for (const QByteArray &property: this->dynamicPropertyNames()) {
        QString key = QString::fromUtf8(property.constData());
        caps[key] = this->property(property.toStdString().c_str());
    }

    return caps;
}

QString AkCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString caps = this->d->m_mimeType;
    QStringList properties;

    for (const QByteArray &property: this->dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    for (const QString &property: properties)
        caps.append(QString(",%1=%2").arg(property)
                                     .arg(this->property(property.toStdString().c_str()).toString()));

    return caps;
}

AkCaps &AkCaps::update(const AkCaps &other)
{
    if (this->d->m_mimeType != other.d->m_mimeType)
        return *this;

    for (const QByteArray &property: other.dynamicPropertyNames())
        this->setProperty(property.constData(),
                          other.property(property.constData()));

    return *this;
}

bool AkCaps::isCompatible(const AkCaps &other) const
{
    if (this->d->m_mimeType != other.d->m_mimeType)
        return false;

    for (const QByteArray &property: other.dynamicPropertyNames())
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
    this->d->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*\\s*").exactMatch(mimeType);
    QString _mimeType = this->d->m_isValid? mimeType.trimmed(): QString("");

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
    this->d->m_mimeType.clear();
    this->d->m_isValid = false;

    QList<QByteArray> properties = this->dynamicPropertyNames();

    for (const QByteArray &property: properties)
        this->setProperty(property.constData(), QVariant());
}

QDebug operator <<(QDebug debug, const AkCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}
