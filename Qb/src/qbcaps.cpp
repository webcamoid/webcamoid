/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "qbcaps.h"

QbCaps::QbCaps(QObject *parent): QObject(parent)
{
    this->resetMimeType();
    this->m_isValid = false;
}

QbCaps::QbCaps(QString capsString)
{
    this->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*"
                              "(?:\\s*,\\s*[a-zA-Z_]\\w*\\s*="
                              "\\s*(?:\\d+/\\d+|\\w+))*\\s*").exactMatch(capsString);

    QStringList capsChunks;

    if (this->m_isValid)
        capsChunks = capsString.split(QRegExp("\\s*,\\s*"),
                                      QString::SkipEmptyParts);

    for (int i = 1; i < capsChunks.length(); i++)
    {
        QStringList pair = capsChunks[i].split(QRegExp("\\s*=\\s*"),
                                               QString::SkipEmptyParts);

        this->setProperty(pair[0].trimmed().toUtf8().constData(),
                          pair[1].trimmed());
    }

    this->setMimeType(this->m_isValid? capsChunks[0].trimmed(): "");
}

QbCaps::QbCaps(const QbCaps &other):
    QObject(other.parent()),
    m_isValid(other.m_isValid),
    m_mimeType(other.m_mimeType)
{
    this->update(other);
}

QbCaps::~QbCaps()
{
}

QbCaps &QbCaps::operator =(const QbCaps &other)
{
    if (this != &other)
    {
        this->m_isValid = other.m_isValid;
        this->m_mimeType = other.m_mimeType;
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

bool QbCaps::isValid() const
{
    return this->m_isValid;
}

QString QbCaps::mimeType() const
{
    return this->m_mimeType;
}

QString QbCaps::toString() const
{
    if (!this->isValid())
        return "";

    QString caps = this->mimeType();
    QStringList properties;

    foreach (QByteArray property, this->dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    foreach (QString property, properties)
        caps.append(QString(",%1=%2").arg(property)
                                     .arg(this->property(property.toUtf8().constData()).toString()));

    return caps;
}

QbCaps &QbCaps::update(const QbCaps &other)
{
    if (this->mimeType() != other.mimeType())
        return *this;

    foreach (QByteArray property, other.dynamicPropertyNames())
        this->setProperty(property.constData(),
                          other.property(property.constData()));

    return *this;
}

bool QbCaps::isCompatible(const QbCaps &other) const
{
    if (this->mimeType() != other.mimeType())
        return false;

    bool compatible = true;

    foreach (QByteArray property, other.dynamicPropertyNames())
        if (!this->dynamicPropertyNames().contains(property) ||
            this->property(property.constData()) != other.property(property.constData()))
        {
            compatible = false;

            break;
        }

    return compatible;
}

void QbCaps::setMimeType(QString mimeType)
{
    this->m_isValid = QRegExp("\\s*[a-z]+/\\w+(?:(?:-|\\+|\\.)\\w+)*\\s*").exactMatch(mimeType);
    this->m_mimeType = this->m_isValid? mimeType.trimmed(): "";
}

void QbCaps::resetMimeType()
{
    this->setMimeType("");
}
