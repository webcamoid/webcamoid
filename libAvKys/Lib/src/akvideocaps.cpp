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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QMetaEnum>

#include "akvideocaps.h"

class AkVideoCapsPrivate
{
    public:
        bool m_isValid;
        AkVideoCaps::PixelFormat m_format;
        int m_width;
        int m_height;
        AkFrac m_fps;
};

AkVideoCaps::AkVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_width = 0;
    this->d->m_height = 0;
}

AkVideoCaps::AkVideoCaps(const QVariantMap &caps)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = true;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_width = 0;
    this->d->m_height = 0;

    this->fromMap(caps);
}

AkVideoCaps::AkVideoCaps(const QString &caps)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_width = 0;
    this->d->m_height = 0;

    this->fromString(caps);
}

AkVideoCaps::AkVideoCaps(const AkCaps &caps)
{
    this->d = new AkVideoCapsPrivate();

    if (caps.mimeType() == "video/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->update(caps);
    } else {
        this->d->m_isValid = false;
        this->d->m_format = AkVideoCaps::Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
    }
}

AkVideoCaps::AkVideoCaps(const AkVideoCaps &other):
    QObject()
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;

    QList<QByteArray> properties = other.dynamicPropertyNames();

    foreach (QByteArray property, properties)
        this->setProperty(property, other.property(property));
}

AkVideoCaps::~AkVideoCaps()
{
    delete this->d;
}

AkVideoCaps &AkVideoCaps::operator =(const AkVideoCaps &other)
{
    if (this != &other) {
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;

        this->clear();

        QList<QByteArray> properties = other.dynamicPropertyNames();

        foreach (QByteArray property, properties)
            this->setProperty(property, other.property(property));
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const AkCaps &caps)
{
    if (caps.mimeType() == "video/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->update(caps);
    } else {
        this->d->m_isValid = false;
        this->d->m_format = AkVideoCaps::Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = AkFrac();
    }

    return *this;
}

AkVideoCaps &AkVideoCaps::operator =(const QString &caps)
{
    return this->operator =(AkCaps(caps));
}

bool AkVideoCaps::operator ==(const AkVideoCaps &other) const
{
    if (this->toString() == other.toString())
        return true;

    return false;
}

bool AkVideoCaps::operator !=(const AkVideoCaps &other) const
{
    return !(*this == other);
}

AkVideoCaps::operator bool() const
{
    return this->d->m_isValid;
}

bool AkVideoCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &AkVideoCaps::isValid()
{
    return this->d->m_isValid;
}

AkVideoCaps::PixelFormat AkVideoCaps::format() const
{
    return this->d->m_format;
}

AkVideoCaps::PixelFormat &AkVideoCaps::format()
{
    return this->d->m_format;
}

int AkVideoCaps::width() const
{
    return this->d->m_width;
}

int &AkVideoCaps::width()
{
    return this->d->m_width;
}

int AkVideoCaps::height() const
{
    return this->d->m_height;
}

int &AkVideoCaps::height()
{
    return this->d->m_height;
}

AkFrac AkVideoCaps::fps() const
{
    return this->d->m_fps;
}

AkFrac &AkVideoCaps::fps()
{
    return this->d->m_fps;
}

AkVideoCaps &AkVideoCaps::fromMap(const QVariantMap &caps)
{
    QList<QByteArray> properties = this->dynamicPropertyNames();

    foreach (QByteArray property, properties)
        this->setProperty(property, QVariant());

    if (!caps.contains("mimeType")) {
        this->d->m_isValid = false;

        return *this;
    }

    foreach (QString key, caps.keys())
        if (key == "mimeType") {
            this->d->m_isValid = caps[key].toString() == "video/x-raw";

            if (!this->d->m_isValid)
                return *this;
        } else
            this->setProperty(key.trimmed().toStdString().c_str(), caps[key]);

    return *this;
}

AkVideoCaps &AkVideoCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap AkVideoCaps::toMap() const
{
    QVariantMap map;
    map["format"] = this->pixelFormatToString(this->d->m_format);
    map["width"] = this->d->m_width;
    map["height"] = this->d->m_height;
    map["fps"] = QVariant::fromValue(this->d->m_fps);

    foreach (QByteArray property, this->dynamicPropertyNames()) {
        QString key = QString::fromUtf8(property.constData());
        map[key] = this->property(property);
    }

    return map;
}

QString AkVideoCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString format = this->pixelFormatToString(this->d->m_format);

    QString caps = QString("video/x-raw,"
                           "format=%1,"
                           "width=%2,"
                           "height=%3,"
                           "fps=%4").arg(format)
                                    .arg(this->d->m_width)
                                    .arg(this->d->m_height)
                                    .arg(this->d->m_fps.toString());

    QStringList properties;

    foreach (QByteArray property, this->dynamicPropertyNames())
        properties << QString::fromUtf8(property.constData());

    properties.sort();

    foreach (QString property, properties)
        caps.append(QString(",%1=%2").arg(property)
                                     .arg(this->property(property.toStdString().c_str()).toString()));

    return caps;
}

AkVideoCaps &AkVideoCaps::update(const AkCaps &caps)
{
    if (caps.mimeType() != "video/x-raw")
        return *this;

    this->clear();

    QList<QByteArray> properties = caps.dynamicPropertyNames();

    foreach (QByteArray property, properties)
        if (property == "format")
            this->d->m_format = this->pixelFormatFromString(caps.property(property).toString());
        else if (property == "width")
            this->d->m_width = caps.property(property).toInt();
        else if (property == "height")
            this->d->m_height = caps.property(property).toInt();
        else if (property == "fps")
            this->d->m_fps = caps.property("fps").toString();
        else
            this->setProperty(property, caps.property(property));

    return *this;
}

AkCaps AkVideoCaps::toCaps() const
{
    return AkCaps(this->toString());
}

QString AkVideoCaps::pixelFormatToString(AkVideoCaps::PixelFormat pixelFormat)
{
    AkVideoCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(pixelFormat));
    format.remove("Format_");

    return format;
}

AkVideoCaps::PixelFormat AkVideoCaps::pixelFormatFromString(const QString &pixelFormat)
{
    AkVideoCaps caps;
    QString format = "Format_" + pixelFormat;
    int enumIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum enumType = caps.metaObject()->enumerator(enumIndex);
    int enumValue = enumType.keyToValue(format.toStdString().c_str());

    return static_cast<PixelFormat>(enumValue);
}

void AkVideoCaps::setFormat(AkVideoCaps::PixelFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkVideoCaps::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(width);
}

void AkVideoCaps::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(height);
}

void AkVideoCaps::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void AkVideoCaps::resetFormat()
{
    this->setFormat(AkVideoCaps::Format_none);
}

void AkVideoCaps::resetWidth()
{
    this->setWidth(0);
}

void AkVideoCaps::resetHeight()
{
    this->setHeight(0);
}

void AkVideoCaps::resetFps()
{
    this->setFps(AkFrac());
}

void AkVideoCaps::clear()
{
    QList<QByteArray> properties = this->dynamicPropertyNames();

    foreach (QByteArray property, properties)
        this->setProperty(property.constData(), QVariant());
}

QDebug operator <<(QDebug debug, const AkVideoCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkVideoCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkVideoCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}
