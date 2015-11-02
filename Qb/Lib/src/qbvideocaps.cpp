/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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

#include "qbvideocaps.h"

class QbVideoCapsPrivate
{
    public:
        bool m_isValid;
        QbVideoCaps::PixelFormat m_format;
        int m_width;
        int m_height;
        QbFrac m_fps;
};

QbVideoCaps::QbVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new QbVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = QbVideoCaps::Format_none;
    this->d->m_width = 0;
    this->d->m_height = 0;
}

QbVideoCaps::QbVideoCaps(const QVariantMap &caps)
{
    this->d = new QbVideoCapsPrivate();
    this->d->m_format = caps["format"].value<PixelFormat>();
    this->d->m_isValid = this->d->m_format == QbVideoCaps::Format_none? false: false;
    this->d->m_width = caps["width"].toInt();
    this->d->m_height = caps["height"].toInt();
    this->d->m_fps = caps["fps"].value<QbFrac>();
}

QbVideoCaps::QbVideoCaps(const QString &caps)
{
    this->d = new QbVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = QbVideoCaps::Format_none;
    this->d->m_width = 0;
    this->d->m_height = 0;
    *this = caps;
}

QbVideoCaps::QbVideoCaps(const QbCaps &caps)
{
    this->d = new QbVideoCapsPrivate();

    if (caps.mimeType() == "video/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->d->m_format = this->pixelFormatFromString(caps.property("format").toString());
        this->d->m_width = caps.property("width").toInt();
        this->d->m_height = caps.property("height").toInt();
        this->d->m_fps = caps.property("fps").toString();
    } else {
        this->d->m_isValid = false;
        this->d->m_format = QbVideoCaps::Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
    }
}

QbVideoCaps::QbVideoCaps(const QbVideoCaps &other):
    QObject()
{
    this->d = new QbVideoCapsPrivate();
    this->d->m_isValid = other.d->m_isValid;
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;
}

QbVideoCaps::~QbVideoCaps()
{
    delete this->d;
}

QbVideoCaps &QbVideoCaps::operator =(const QbVideoCaps &other)
{
    if (this != &other) {
        this->d->m_isValid = other.d->m_isValid;
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;
    }

    return *this;
}

QbVideoCaps &QbVideoCaps::operator =(const QbCaps &caps)
{
    if (caps.mimeType() == "video/x-raw") {
        this->d->m_isValid = caps.isValid();
        this->d->m_format = this->pixelFormatFromString(caps.property("format").toString());
        this->d->m_width = caps.property("width").toInt();
        this->d->m_height = caps.property("height").toInt();
        this->d->m_fps = caps.property("fps").toString();
    } else {
        this->d->m_isValid = false;
        this->d->m_format = QbVideoCaps::Format_none;
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = QbFrac();
    }

    return *this;
}

QbVideoCaps &QbVideoCaps::operator =(const QString &caps)
{
    return this->operator =(QbCaps(caps));
}

bool QbVideoCaps::operator ==(const QbVideoCaps &other) const
{
    if (this->d->m_isValid == other.d->m_isValid
        && this->d->m_format == other.d->m_format
        && this->d->m_width == other.d->m_width
        && this->d->m_height == other.d->m_height
        && this->d->m_fps == other.d->m_fps)
        return true;

    return false;
}

bool QbVideoCaps::operator !=(const QbVideoCaps &other) const
{
    return !(*this == other);
}

QbVideoCaps::operator bool() const
{
    return this->d->m_isValid;
}

bool QbVideoCaps::isValid() const
{
    return this->d->m_isValid;
}

bool &QbVideoCaps::isValid()
{
    return this->d->m_isValid;
}

QbVideoCaps::PixelFormat QbVideoCaps::format() const
{
    return this->d->m_format;
}

QbVideoCaps::PixelFormat &QbVideoCaps::format()
{
    return this->d->m_format;
}

int QbVideoCaps::width() const
{
    return this->d->m_width;
}

int &QbVideoCaps::width()
{
    return this->d->m_width;
}

int QbVideoCaps::height() const
{
    return this->d->m_height;
}

int &QbVideoCaps::height()
{
    return this->d->m_height;
}

QbFrac QbVideoCaps::fps() const
{
    return this->d->m_fps;
}

QbFrac &QbVideoCaps::fps()
{
    return this->d->m_fps;
}

QbVideoCaps &QbVideoCaps::fromMap(const QVariantMap &caps)
{
    this->d->m_format = caps["format"].value<PixelFormat>();
    this->d->m_isValid = this->d->m_format == QbVideoCaps::Format_none? false: false;
    this->d->m_width = caps["width"].toInt();
    this->d->m_height = caps["height"].toInt();
    this->d->m_fps = caps["fps"].value<QbFrac>();

    return *this;
}

QbVideoCaps &QbVideoCaps::fromString(const QString &caps)
{
    return *this = caps;
}

QVariantMap QbVideoCaps::toMap() const
{
    QVariantMap map;
    map["format"] = this->d->m_format;
    map["width"] = this->d->m_width;
    map["height"] = this->d->m_height;
    map["fps"] = QVariant::fromValue(this->d->m_fps);

    return map;
}

QString QbVideoCaps::toString() const
{
    if (!this->d->m_isValid)
        return QString();

    QString format = this->pixelFormatToString(this->d->m_format);

    return QString("video/x-raw,"
                   "format=%1,"
                   "width=%2,"
                   "height=%3,"
                   "fps=%4").arg(format)
                            .arg(this->d->m_width)
                            .arg(this->d->m_height)
                            .arg(this->d->m_fps.toString());
}

QbVideoCaps &QbVideoCaps::update(const QbCaps &caps)
{
    if (caps.mimeType() != "video/x-raw")
        return *this;

    if (caps.contains("format"))
        this->d->m_format = this->pixelFormatFromString(caps.property("format").toString());

    if (caps.contains("width"))
        this->d->m_width = caps.property("width").toInt();

    if (caps.contains("height"))
        this->d->m_height = caps.property("height").toInt();

    if (caps.contains("fps"))
        this->d->m_fps = caps.property("fps").toString();

    return *this;
}

QbCaps QbVideoCaps::toCaps() const
{
    return QbCaps(this->toString());
}

QString QbVideoCaps::pixelFormatToString(QbVideoCaps::PixelFormat pixelFormat)
{
    QbVideoCaps caps;
    int formatIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum formatEnum = caps.metaObject()->enumerator(formatIndex);
    QString format(formatEnum.valueToKey(pixelFormat));
    format.remove("Format_");

    return format;
}

QbVideoCaps::PixelFormat QbVideoCaps::pixelFormatFromString(const QString &pixelFormat)
{
    QbVideoCaps caps;
    QString format = "Format_" + pixelFormat;
    int enumIndex = caps.metaObject()->indexOfEnumerator("PixelFormat");
    QMetaEnum enumType = caps.metaObject()->enumerator(enumIndex);
    int enumValue = enumType.keyToValue(format.toStdString().c_str());

    return static_cast<PixelFormat>(enumValue);
}

void QbVideoCaps::setFormat(QbVideoCaps::PixelFormat format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void QbVideoCaps::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(width);
}

void QbVideoCaps::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(height);
}

void QbVideoCaps::setFps(const QbFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void QbVideoCaps::resetFormat()
{
    this->setFormat(QbVideoCaps::Format_none);
}

void QbVideoCaps::resetWidth()
{
    this->setWidth(0);
}

void QbVideoCaps::resetHeight()
{
    this->setHeight(0);
}

void QbVideoCaps::resetFps()
{
    this->setFps(QbFrac());
}

QDebug operator <<(QDebug debug, const QbVideoCaps &caps)
{
    debug.nospace() << caps.toString();

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, QbVideoCaps &caps)
{
    QString capsStr;
    istream >> capsStr;
    caps.fromString(capsStr);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const QbVideoCaps &caps)
{
    ostream << caps.toString();

    return ostream;
}
