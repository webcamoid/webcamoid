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
#include <QMetaEnum>
#include <QSize>
#include <QQmlEngine>

#include "akcompressedvideocaps.h"
#include "akfrac.h"
#include "akcaps.h"

class AkCompressedVideoCapsPrivate
{
    public:
        QString m_format;
        int m_width {0};
        int m_height {0};
        AkFrac m_fps;
};

AkCompressedVideoCaps::AkCompressedVideoCaps(QObject *parent):
    QObject(parent)
{
    this->d = new AkCompressedVideoCapsPrivate();
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const QString &format,
                                             int width,
                                             int height,
                                             const AkFrac &fps):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = width;
    this->d->m_height = height;
    this->d->m_fps = fps;
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const QString &format,
                                             const QSize &size,
                                             const AkFrac &fps):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();
    this->d->m_format = format;
    this->d->m_width = size.width();
    this->d->m_height = size.height();
    this->d->m_fps = fps;
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const AkCaps &other):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();

    if (other.type() == AkCaps::CapsVideoCompressed) {
        auto data = reinterpret_cast<AkCompressedVideoCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_width = data->d->m_width;
        this->d->m_height = data->d->m_height;
        this->d->m_fps = data->d->m_fps;
    }
}

AkCompressedVideoCaps::AkCompressedVideoCaps(const AkCompressedVideoCaps &other):
    QObject()
{
    this->d = new AkCompressedVideoCapsPrivate();
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;
}

AkCompressedVideoCaps::~AkCompressedVideoCaps()
{
    delete this->d;
}

AkCompressedVideoCaps &AkCompressedVideoCaps::operator =(const AkCaps &other)
{
    if (other.type() == AkCaps::CapsVideoCompressed) {
        auto data = reinterpret_cast<AkCompressedVideoCaps *>(other.privateData());
        this->d->m_format = data->d->m_format;
        this->d->m_width = data->d->m_width;
        this->d->m_height = data->d->m_height;
        this->d->m_fps = data->d->m_fps;
    } else {
        this->d->m_format = "";
        this->d->m_width = 0;
        this->d->m_height = 0;
        this->d->m_fps = {};
    }

    return *this;
}

AkCompressedVideoCaps &AkCompressedVideoCaps::operator =(const AkCompressedVideoCaps &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;
    }

    return *this;
}

bool AkCompressedVideoCaps::operator ==(const AkCompressedVideoCaps &other) const
{
    return this->d->m_format == other.d->m_format
            && this->d->m_width == other.d->m_width
            && this->d->m_height == other.d->m_height
            && this->d->m_fps == other.d->m_fps;
}

bool AkCompressedVideoCaps::operator !=(const AkCompressedVideoCaps &other) const
{
    return !(*this == other);
}

AkCompressedVideoCaps::operator bool() const
{
    return !this->d->m_format.isEmpty()
           && this->d->m_width > 0
           && this->d->m_height > 0;
}

AkCompressedVideoCaps::operator AkCaps() const
{
    AkCaps caps;
    caps.setType(AkCaps::CapsVideoCompressed);
    caps.setPrivateData(new AkCompressedVideoCaps(*this),
                        [] (void *data) -> void * {
                            return new AkCompressedVideoCaps(*reinterpret_cast<AkCompressedVideoCaps *>(data));
                        },
                        [] (void *data) {
                            delete reinterpret_cast<AkCompressedVideoCaps *>(data);
                        });

    return caps;
}

QObject *AkCompressedVideoCaps::create()
{
    return new AkCompressedVideoCaps();
}

QObject *AkCompressedVideoCaps::create(const AkCaps &caps)
{
    return new AkCompressedVideoCaps(caps);
}

QObject *AkCompressedVideoCaps::create(const AkCompressedVideoCaps &caps)
{
    return new AkCompressedVideoCaps(caps);
}

QObject *AkCompressedVideoCaps::create(const QString &format,
                                       int width,
                                       int height,
                                       const AkFrac &fps)
{
    return new AkCompressedVideoCaps(format, width, height, fps);
}

QObject *AkCompressedVideoCaps::create(const QString &format,
                             const QSize &size,
                             const AkFrac &fps)
{
    return new AkCompressedVideoCaps(format, size, fps);
}

QVariant AkCompressedVideoCaps::toVariant() const
{
    return QVariant::fromValue(*this);
}

QString AkCompressedVideoCaps::format() const
{
    return this->d->m_format;
}

int AkCompressedVideoCaps::width() const
{
    return this->d->m_width;
}

int AkCompressedVideoCaps::height() const
{
    return this->d->m_height;
}

AkFrac AkCompressedVideoCaps::fps() const
{
    return this->d->m_fps;
}

AkFrac &AkCompressedVideoCaps::fps()
{
    return this->d->m_fps;
}

void AkCompressedVideoCaps::setFormat(const QString &format)
{
    if (this->d->m_format == format)
        return;

    this->d->m_format = format;
    emit this->formatChanged(format);
}

void AkCompressedVideoCaps::setSize(const QSize &size)
{
    QSize curSize(this->d->m_width, this->d->m_height);

    if (curSize == size)
        return;

    this->d->m_width = size.width();
    this->d->m_height = size.height();
    emit this->widthChanged(size.width());
    emit this->heightChanged(size.height());
    emit sizeChanged(size);
}

void AkCompressedVideoCaps::setWidth(int width)
{
    if (this->d->m_width == width)
        return;

    this->d->m_width = width;
    emit this->widthChanged(width);
}

void AkCompressedVideoCaps::setHeight(int height)
{
    if (this->d->m_height == height)
        return;

    this->d->m_height = height;
    emit this->heightChanged(height);
}

void AkCompressedVideoCaps::setFps(const AkFrac &fps)
{
    if (this->d->m_fps == fps)
        return;

    this->d->m_fps = fps;
    emit this->fpsChanged(fps);
}

void AkCompressedVideoCaps::resetFormat()
{
    this->setFormat({});
}

void AkCompressedVideoCaps::resetSize()
{
    this->setSize({});
}

void AkCompressedVideoCaps::resetWidth()
{
    this->setWidth(0);
}

void AkCompressedVideoCaps::resetHeight()
{
    this->setHeight(0);
}

void AkCompressedVideoCaps::resetFps()
{
    this->setFps(AkFrac());
}

void AkCompressedVideoCaps::registerTypes()
{
    qRegisterMetaType<AkCompressedVideoCaps>("AkCompressedVideoCaps");
    qmlRegisterSingletonType<AkCompressedVideoCaps>("Ak", 1, 0, "AkCompressedVideoCaps",
                                          [] (QQmlEngine *qmlEngine,
                                              QJSEngine *jsEngine) -> QObject * {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(jsEngine)

        return new AkCompressedVideoCaps();
    });
}

QDebug operator <<(QDebug debug, const AkCompressedVideoCaps &caps)
{
    debug.nospace() << "AkCompressedVideoCaps("
                    << "format="
                    << caps.format()
                    << ",width="
                    << caps.width()
                    << ",height="
                    << caps.height()
                    << ",fps="
                    << caps.fps()
                    << ")";

    return debug.space();
}

QDataStream &operator >>(QDataStream &istream, AkCompressedVideoCaps &caps)
{
    QString format;
    istream >> format;
    caps.setFormat(format);
    int width = 0;
    istream >> width;
    caps.setWidth(width);
    int height = 0;
    istream >> height;
    caps.setHeight(height);
    AkFrac fps;
    istream >> fps;
    caps.setFps(fps);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, const AkCompressedVideoCaps &caps)
{
    ostream << caps.format();
    ostream << caps.width();
    ostream << caps.height();
    ostream << caps.fps();

    return ostream;
}

#include "moc_akcompressedvideocaps.cpp"
