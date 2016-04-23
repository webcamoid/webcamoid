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

#include <QMetaEnum>

#include "akvideocaps.h"

typedef QMap<AkVideoCaps::PixelFormat, int> BitsPerPixelMap;

inline BitsPerPixelMap initBitsPerPixelMap()
{
    BitsPerPixelMap bitsPerPixel;
    bitsPerPixel[AkVideoCaps::Format_yuv420p] = 12;
    bitsPerPixel[AkVideoCaps::Format_yuyv422] = 16;
    bitsPerPixel[AkVideoCaps::Format_rgb24] = 24;
    bitsPerPixel[AkVideoCaps::Format_bgr24] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv422p] = 16;
    bitsPerPixel[AkVideoCaps::Format_yuv444p] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv410p] = 9;
    bitsPerPixel[AkVideoCaps::Format_yuv411p] = 12;
    bitsPerPixel[AkVideoCaps::Format_gray] = 8;
    bitsPerPixel[AkVideoCaps::Format_monow] = 1;
    bitsPerPixel[AkVideoCaps::Format_monob] = 1;
    bitsPerPixel[AkVideoCaps::Format_pal8] = 8;
    bitsPerPixel[AkVideoCaps::Format_yuvj420p] = 12;
    bitsPerPixel[AkVideoCaps::Format_yuvj422p] = 16;
    bitsPerPixel[AkVideoCaps::Format_yuvj444p] = 24;
    bitsPerPixel[AkVideoCaps::Format_uyvy422] = 16;
    bitsPerPixel[AkVideoCaps::Format_uyyvyy411] = 12;
    bitsPerPixel[AkVideoCaps::Format_bgr8] = 8;
    bitsPerPixel[AkVideoCaps::Format_bgr4] = 4;
    bitsPerPixel[AkVideoCaps::Format_bgr4_byte] = 4;
    bitsPerPixel[AkVideoCaps::Format_rgb8] = 8;
    bitsPerPixel[AkVideoCaps::Format_rgb4] = 4;
    bitsPerPixel[AkVideoCaps::Format_rgb4_byte] = 4;
    bitsPerPixel[AkVideoCaps::Format_nv12] = 12;
    bitsPerPixel[AkVideoCaps::Format_nv21] = 12;
    bitsPerPixel[AkVideoCaps::Format_argb] = 32;
    bitsPerPixel[AkVideoCaps::Format_rgba] = 32;
    bitsPerPixel[AkVideoCaps::Format_abgr] = 32;
    bitsPerPixel[AkVideoCaps::Format_bgra] = 32;
    bitsPerPixel[AkVideoCaps::Format_gray16be] = 16;
    bitsPerPixel[AkVideoCaps::Format_gray16le] = 16;
    bitsPerPixel[AkVideoCaps::Format_yuv440p] = 16;
    bitsPerPixel[AkVideoCaps::Format_yuvj440p] = 16;
    bitsPerPixel[AkVideoCaps::Format_yuva420p] = 20;
    bitsPerPixel[AkVideoCaps::Format_rgb48be] = 48;
    bitsPerPixel[AkVideoCaps::Format_rgb48le] = 48;
    bitsPerPixel[AkVideoCaps::Format_rgb565be] = 16;
    bitsPerPixel[AkVideoCaps::Format_rgb565le] = 16;
    bitsPerPixel[AkVideoCaps::Format_rgb555be] = 15;
    bitsPerPixel[AkVideoCaps::Format_rgb555le] = 15;
    bitsPerPixel[AkVideoCaps::Format_bgr565be] = 16;
    bitsPerPixel[AkVideoCaps::Format_bgr565le] = 16;
    bitsPerPixel[AkVideoCaps::Format_bgr555be] = 15;
    bitsPerPixel[AkVideoCaps::Format_bgr555le] = 15;
    bitsPerPixel[AkVideoCaps::Format_yuv420p16le] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv420p16be] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv422p16le] = 32;
    bitsPerPixel[AkVideoCaps::Format_yuv422p16be] = 32;
    bitsPerPixel[AkVideoCaps::Format_yuv444p16le] = 48;
    bitsPerPixel[AkVideoCaps::Format_yuv444p16be] = 48;
    bitsPerPixel[AkVideoCaps::Format_rgb444le] = 12;
    bitsPerPixel[AkVideoCaps::Format_rgb444be] = 12;
    bitsPerPixel[AkVideoCaps::Format_bgr444le] = 12;
    bitsPerPixel[AkVideoCaps::Format_bgr444be] = 12;
    bitsPerPixel[AkVideoCaps::Format_ya8] = 16;
    bitsPerPixel[AkVideoCaps::Format_bgr48be] = 48;
    bitsPerPixel[AkVideoCaps::Format_bgr48le] = 48;
    bitsPerPixel[AkVideoCaps::Format_yuv420p9be] = 13;
    bitsPerPixel[AkVideoCaps::Format_yuv420p9le] = 13;
    bitsPerPixel[AkVideoCaps::Format_yuv420p10be] = 15;
    bitsPerPixel[AkVideoCaps::Format_yuv420p10le] = 15;
    bitsPerPixel[AkVideoCaps::Format_yuv422p10be] = 20;
    bitsPerPixel[AkVideoCaps::Format_yuv422p10le] = 20;
    bitsPerPixel[AkVideoCaps::Format_yuv444p9be] = 27;
    bitsPerPixel[AkVideoCaps::Format_yuv444p9le] = 27;
    bitsPerPixel[AkVideoCaps::Format_yuv444p10be] = 30;
    bitsPerPixel[AkVideoCaps::Format_yuv444p10le] = 30;
    bitsPerPixel[AkVideoCaps::Format_yuv422p9be] = 18;
    bitsPerPixel[AkVideoCaps::Format_yuv422p9le] = 18;
    bitsPerPixel[AkVideoCaps::Format_gbrp] = 24;
    bitsPerPixel[AkVideoCaps::Format_gbrp9be] = 27;
    bitsPerPixel[AkVideoCaps::Format_gbrp9le] = 27;
    bitsPerPixel[AkVideoCaps::Format_gbrp10be] = 30;
    bitsPerPixel[AkVideoCaps::Format_gbrp10le] = 30;
    bitsPerPixel[AkVideoCaps::Format_gbrp16be] = 48;
    bitsPerPixel[AkVideoCaps::Format_gbrp16le] = 48;
    bitsPerPixel[AkVideoCaps::Format_yuva420p9be] = 22;
    bitsPerPixel[AkVideoCaps::Format_yuva420p9le] = 22;
    bitsPerPixel[AkVideoCaps::Format_yuva422p9be] = 27;
    bitsPerPixel[AkVideoCaps::Format_yuva422p9le] = 27;
    bitsPerPixel[AkVideoCaps::Format_yuva444p9be] = 36;
    bitsPerPixel[AkVideoCaps::Format_yuva444p9le] = 36;
    bitsPerPixel[AkVideoCaps::Format_yuva420p10be] = 25;
    bitsPerPixel[AkVideoCaps::Format_yuva420p10le] = 25;
    bitsPerPixel[AkVideoCaps::Format_yuva422p10be] = 30;
    bitsPerPixel[AkVideoCaps::Format_yuva422p10le] = 30;
    bitsPerPixel[AkVideoCaps::Format_yuva444p10be] = 40;
    bitsPerPixel[AkVideoCaps::Format_yuva444p10le] = 40;
    bitsPerPixel[AkVideoCaps::Format_yuva420p16be] = 40;
    bitsPerPixel[AkVideoCaps::Format_yuva420p16le] = 40;
    bitsPerPixel[AkVideoCaps::Format_yuva422p16be] = 48;
    bitsPerPixel[AkVideoCaps::Format_yuva422p16le] = 48;
    bitsPerPixel[AkVideoCaps::Format_yuva444p16be] = 64;
    bitsPerPixel[AkVideoCaps::Format_yuva444p16le] = 64;
    bitsPerPixel[AkVideoCaps::Format_xyz12le] = 36;
    bitsPerPixel[AkVideoCaps::Format_xyz12be] = 36;
    bitsPerPixel[AkVideoCaps::Format_nv16] = 16;
    bitsPerPixel[AkVideoCaps::Format_nv20le] = 20;
    bitsPerPixel[AkVideoCaps::Format_nv20be] = 20;
    bitsPerPixel[AkVideoCaps::Format_yvyu422] = 16;
    bitsPerPixel[AkVideoCaps::Format_ya16be] = 32;
    bitsPerPixel[AkVideoCaps::Format_ya16le] = 32;
    bitsPerPixel[AkVideoCaps::Format_rgba64be] = 64;
    bitsPerPixel[AkVideoCaps::Format_rgba64le] = 64;
    bitsPerPixel[AkVideoCaps::Format_bgra64be] = 64;
    bitsPerPixel[AkVideoCaps::Format_bgra64le] = 64;
    bitsPerPixel[AkVideoCaps::Format_0rgb] = 24;
    bitsPerPixel[AkVideoCaps::Format_rgb0] = 24;
    bitsPerPixel[AkVideoCaps::Format_0bgr] = 24;
    bitsPerPixel[AkVideoCaps::Format_bgr0] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuva444p] = 32;
    bitsPerPixel[AkVideoCaps::Format_yuva422p] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv420p12be] = 18;
    bitsPerPixel[AkVideoCaps::Format_yuv420p12le] = 18;
    bitsPerPixel[AkVideoCaps::Format_yuv420p14be] = 21;
    bitsPerPixel[AkVideoCaps::Format_yuv420p14le] = 21;
    bitsPerPixel[AkVideoCaps::Format_yuv422p12be] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv422p12le] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv422p14be] = 28;
    bitsPerPixel[AkVideoCaps::Format_yuv422p14le] = 28;
    bitsPerPixel[AkVideoCaps::Format_yuv444p12be] = 36;
    bitsPerPixel[AkVideoCaps::Format_yuv444p12le] = 36;
    bitsPerPixel[AkVideoCaps::Format_yuv444p14be] = 42;
    bitsPerPixel[AkVideoCaps::Format_yuv444p14le] = 42;
    bitsPerPixel[AkVideoCaps::Format_gbrp12be] = 36;
    bitsPerPixel[AkVideoCaps::Format_gbrp12le] = 36;
    bitsPerPixel[AkVideoCaps::Format_gbrp14be] = 42;
    bitsPerPixel[AkVideoCaps::Format_gbrp14le] = 42;
    bitsPerPixel[AkVideoCaps::Format_gbrap] = 32;
    bitsPerPixel[AkVideoCaps::Format_gbrap16be] = 64;
    bitsPerPixel[AkVideoCaps::Format_gbrap16le] = 64;
    bitsPerPixel[AkVideoCaps::Format_yuvj411p] = 12;
    bitsPerPixel[AkVideoCaps::Format_bayer_bggr8] = 8;
    bitsPerPixel[AkVideoCaps::Format_bayer_rggb8] = 8;
    bitsPerPixel[AkVideoCaps::Format_bayer_gbrg8] = 8;
    bitsPerPixel[AkVideoCaps::Format_bayer_grbg8] = 8;
    bitsPerPixel[AkVideoCaps::Format_bayer_bggr16le] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_bggr16be] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_rggb16le] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_rggb16be] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_gbrg16le] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_gbrg16be] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_grbg16le] = 16;
    bitsPerPixel[AkVideoCaps::Format_bayer_grbg16be] = 16;
    bitsPerPixel[AkVideoCaps::Format_yuv440p10le] = 20;
    bitsPerPixel[AkVideoCaps::Format_yuv440p10be] = 20;
    bitsPerPixel[AkVideoCaps::Format_yuv440p12le] = 24;
    bitsPerPixel[AkVideoCaps::Format_yuv440p12be] = 24;
    bitsPerPixel[AkVideoCaps::Format_ayuv64le] = 64;
    bitsPerPixel[AkVideoCaps::Format_ayuv64be] = 64;

    return bitsPerPixel;
}

Q_GLOBAL_STATIC_WITH_ARGS(BitsPerPixelMap, toBitsPerPixel, (initBitsPerPixelMap()))

class AkVideoCapsPrivate
{
    public:
        bool m_isValid;
        AkVideoCaps::PixelFormat m_format;
        int m_bpp;
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
    this->d->m_bpp = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;
}

AkVideoCaps::AkVideoCaps(const QVariantMap &caps)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = true;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_bpp = 0;
    this->d->m_width = 0;
    this->d->m_height = 0;

    this->fromMap(caps);
}

AkVideoCaps::AkVideoCaps(const QString &caps)
{
    this->d = new AkVideoCapsPrivate();
    this->d->m_isValid = false;
    this->d->m_format = AkVideoCaps::Format_none;
    this->d->m_bpp = 0;
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
        this->d->m_bpp = 0;
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
    this->d->m_bpp = other.d->m_bpp;
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
        this->d->m_bpp = other.d->m_bpp;
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
        this->d->m_bpp = 0;
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

AkVideoCaps::operator AkCaps() const
{
    return this->toCaps();
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

int AkVideoCaps::bpp() const
{
    return this->d->m_bpp;
}

int &AkVideoCaps::bpp()
{
    return this->d->m_bpp;
}

QSize AkVideoCaps::size() const
{
    return QSize(this->d->m_width, this->d->m_height);
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

int AkVideoCaps::pictureSize() const
{
    return this->d->m_bpp * this->d->m_width * this->d->m_height / 8;
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
    map["bpp"] = this->d->m_bpp;
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
                           "bpp=%2,"
                           "width=%3,"
                           "height=%4,"
                           "fps=%5").arg(format)
                                    .arg(this->d->m_bpp)
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
        else if (property == "bpp")
            this->d->m_bpp = caps.property(property).toInt();
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

int AkVideoCaps::bitsPerPixel(AkVideoCaps::PixelFormat pixelFormat)
{
    return toBitsPerPixel->value(pixelFormat, 0);
}

int AkVideoCaps::bitsPerPixel(const QString &pixelFormat)
{
    return AkVideoCaps::bitsPerPixel(AkVideoCaps::pixelFormatFromString(pixelFormat));
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

void AkVideoCaps::setBpp(int bpp)
{
    if (this->d->m_bpp == bpp)
        return;

    this->d->m_bpp = bpp;
    emit this->bppChanged(bpp);
}

void AkVideoCaps::setSize(const QSize &size)
{
    QSize curSize(this->d->m_width, this->d->m_height);

    if (curSize == size)
        return;

    this->setWidth(size.width());
    this->setHeight(size.height());
    emit sizeChanged(size);
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

void AkVideoCaps::resetBpp()
{
    this->setBpp(0);
}

void AkVideoCaps::resetSize()
{
    this->setSize(QSize());
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
