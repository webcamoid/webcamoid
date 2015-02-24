/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include <QRegExp>
#include <QStringList>
#include <QVariant>

#include "qbcaps.h"

class QbCapsPrivate
{
    public:
        bool m_isValid;
        QString m_mimeType;
};

QbCaps::QbCaps(QObject *parent): QObject(parent)
{
    this->d = new QbCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_mimeType = "";
}

QbCaps::QbCaps(const QVariantMap &caps)
{
    this->d = new QbCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_mimeType = "";
    this->fromMap(caps);
}

QbCaps::QbCaps(const QString &caps)
{
    this->d = new QbCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_mimeType = "";
    this->fromString(caps);
}

QbCaps::QbCaps(const QbCaps &other):
    QObject(other.parent())
{
    this->d = new QbCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_mimeType = other.d->m_mimeType;
    this->update(other);
}

QbCaps::~QbCaps()
{
    delete this->d;
}

QbCaps &QbCaps::operator =(const QbCaps &other)
{
    if (this != &other) {
        this->clear();
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_mimeType = other.d->m_mimeType;
        this->update(other);
    }

    return *this;
}

bool QbCaps::operator ==(const QbCaps &other) const
{
    if (this->toString() == other.toString())
        return true;

    return false;
}

bool QbCaps::operator !=(const QbCaps &other) const
{
    return !(*this == other);
}

QbCaps::operator bool() const
{
    return this->d->m_isValid;
}

bool QbCaps::isValid() const
{
    return this->d->m_isValid;
}

QString QbCaps::mimeType() const
{
    return this->d->m_mimeType;
}

QbCaps &QbCaps::fromMap(const QVariantMap &caps)
{
    QList<QByteArray> properties = this->dynamicPropertyNames();

    foreach (QByteArray property, properties)
        this->setProperty(property, QVariant());

    if (!caps.contains("mimeType")) {
        this->d->m_isValid = false;
        this->d->m_mimeType = "";

        return *this;
    }

    foreach (QString key, caps.keys())
        if (key == "mimeType") {
            this->d->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*\\s*")
                                 .exactMatch(caps[key].toString());
            this->d->m_mimeType = caps[key].toString().trimmed();
        } else
            this->setProperty(key.trimmed().toStdString().c_str(), caps[key]);

    return *this;
}

QbCaps &QbCaps::fromString(const QString &caps)
{
    this->d->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*"
                                 "(?:\\s*,\\s*[a-zA-Z_]\\w*\\s*="
                                 "\\s*[^,=]+)*\\s*").exactMatch(caps);

    QList<QByteArray> properties = this->dynamicPropertyNames();

    foreach (QByteArray property, properties)
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

QVariantMap QbCaps::toMap() const
{
    if (!this->d->m_isValid)
        return QVariantMap();

    QVariantMap caps;
    caps["mimeType"] = this->d->m_mimeType;

    foreach (QByteArray property, this->dynamicPropertyNames()) {
        QString key = QString::fromUtf8(property.constData());
        caps[key] = this->property(property.toStdString().c_str());
    }

    return caps;
}

QString QbCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString caps = this->d->m_mimeType;
    QStringList properties;

    foreach (QByteArray property, this->dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    foreach (QString property, properties)
        caps.append(QString(",%1=%2").arg(property)
                                     .arg(this->property(property.toStdString().c_str()).toString()));

    return caps;
}

QbCaps &QbCaps::update(const QbCaps &other)
{
    if (this->d->m_mimeType != other.d->m_mimeType)
        return *this;

    foreach (QByteArray property, other.dynamicPropertyNames())
        this->setProperty(property.constData(),
                          other.property(property.constData()));

    return *this;
}

bool QbCaps::isCompatible(const QbCaps &other) const
{
    if (this->d->m_mimeType != other.d->m_mimeType)
        return false;

    foreach (QByteArray property, other.dynamicPropertyNames())
        if (!this->dynamicPropertyNames().contains(property) ||
            this->property(property.constData()) != other.property(property.constData()))
            return false;

    return true;
}

bool QbCaps::contains(const QString &property) const
{
    return this->dynamicPropertyNames().contains(property.toUtf8());
}

void QbCaps::setMimeType(const QString &mimeType)
{
    this->d->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*\\s*").exactMatch(mimeType);
    this->d->m_mimeType = this->d->m_isValid? mimeType.trimmed(): QString("");

    emit this->mimeTypeChanged();
}

void QbCaps::resetMimeType()
{
    this->setMimeType("");
}

void QbCaps::clear()
{
    this->d->m_mimeType.clear();
    this->d->m_isValid = false;

    QList<QByteArray> properties = this->dynamicPropertyNames();

    foreach (QByteArray property, properties)
        this->setProperty(property.constData(), QVariant());
}

QDebug operator <<(QDebug debug, const QbCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, QbCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const QbCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}
