/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QMutex>
#include <QQmlContext>
#include <QSettings>
#include <QThread>
#include <QTime>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "colorsrcelement.h"

#define COLORSRC_PREFIX "color://"
// The compositor scales the frame to the output size, so a tiny fixed
// resolution is enough for a solid color.
#define COLORSRC_WIDTH  32
#define COLORSRC_HEIGHT 32

class ColorSrcElementPrivate
{
    public:
        ColorSrcElement *self;
        AkFrac m_fps {30000, 1001};
        QRgb m_color {qRgba(0, 0, 0, 255)};
        QString m_media;
        qint64 m_id {-1};
        QThreadPool m_threadPool;
        QFuture<void> m_framesThreadStatus;
        AkVideoPacket m_cachedPacket;
        mutable QMutex m_mutex;
        bool m_run {false};

        explicit ColorSrcElementPrivate(ColorSrcElement *self);
        QString settingsId() const;
        void loadSettings();
        void saveSettings() const;
        QString summary() const;
        AkVideoPacket buildPacket() const;
        void readFrame();
};

ColorSrcElement::ColorSrcElement():
    AkMultimediaSourceElement()
{
    this->d = new ColorSrcElementPrivate(this);
}

ColorSrcElement::~ColorSrcElement()
{
    this->setState(AkElement::ElementStateNull);
    delete this->d;
}

QStringList ColorSrcElement::medias()
{
    QMutexLocker locker(&this->d->m_mutex);

    if (this->d->m_media.isEmpty())
        return {};

    return {this->d->m_media};
}

QString ColorSrcElement::media() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_media;
}

QList<int> ColorSrcElement::streams()
{
    QMutexLocker locker(&this->d->m_mutex);

    if (this->d->m_media.isEmpty())
        return {};

    return {0};
}

int ColorSrcElement::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString ColorSrcElement::description(const QString &media)
{
    QMutexLocker locker(&this->d->m_mutex);

    if (media.isEmpty() || media != this->d->m_media)
        return {};

    return this->d->summary();
}

AkCaps ColorSrcElement::caps(int stream)
{
    if (stream != 0)
        return {};

    QMutexLocker locker(&this->d->m_mutex);

    if (this->d->m_media.isEmpty())
        return {};

    return AkVideoCaps(AkVideoCaps::Format_argbpack,
                       COLORSRC_WIDTH,
                       COLORSRC_HEIGHT,
                       this->d->m_fps);
}

QRgb ColorSrcElement::color() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_color;
}

AkFrac ColorSrcElement::fps() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_fps;
}

QString ColorSrcElement::summary() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->summary();
}

QString ColorSrcElement::devicePrefix()
{
    return QStringLiteral(COLORSRC_PREFIX);
}

QString ColorSrcElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/ColorSrc/share/qml/main.qml");
}

void ColorSrcElement::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("ColorSrc", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void ColorSrcElement::setColor(QRgb color)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_color == color)
            return;

        this->d->m_color = color;
        this->d->m_cachedPacket = this->d->buildPacket();
        this->d->saveSettings();
    }

    emit this->colorChanged(color);
    emit this->summaryChanged(this->summary());
}

void ColorSrcElement::setFps(const AkFrac &fps)
{
    if (!fps.isValid() || fps.value() <= 0.0)
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_fps == fps)
            return;

        this->d->m_fps = fps;
        this->d->m_cachedPacket = this->d->buildPacket();
        this->d->saveSettings();
    }

    emit this->fpsChanged(fps);
}

void ColorSrcElement::resetColor()
{
    this->setColor(qRgba(0, 0, 0, 255));
}

void ColorSrcElement::resetFps()
{
    this->setFps({30000, 1001});
}

void ColorSrcElement::removeSettings()
{
    QMutexLocker locker(&this->d->m_mutex);

    auto id = this->d->settingsId();

    if (id.isEmpty())
        return;

    QSettings settings;
    settings.beginGroup("ColorSrc");
    settings.remove(id);
    settings.endGroup();
}

void ColorSrcElement::setMedia(const QString &media)
{
    // Every media is an instance of a color: "color://N".
    if (!media.isEmpty() && !media.startsWith(QString(COLORSRC_PREFIX)))
        return;

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_media == media)
            return;
    }

    auto state = this->state();
    this->setState(AkElement::ElementStateNull);

    {
        QMutexLocker locker(&this->d->m_mutex);
        this->d->m_media = media;
        this->d->loadSettings();
        this->d->m_cachedPacket = this->d->buildPacket();
    }

    if (!media.isEmpty())
        this->setState(state);

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
    emit this->streamsChanged(this->streams());
    emit this->colorChanged(this->color());
    emit this->fpsChanged(this->fps());
    emit this->summaryChanged(this->summary());
}

void ColorSrcElement::resetMedia()
{
    this->setMedia({});
}

bool ColorSrcElement::setState(AkElement::ElementState state)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_media.isEmpty())
            return false;
    }

    auto curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->d->m_id = Ak::id();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_id = Ak::id();
            this->d->m_run = true;
            this->d->m_framesThreadStatus =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      &ColorSrcElementPrivate::readFrame,
                                      this->d);

            return AkElement::setState(state);
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            this->d->m_run = true;
            this->d->m_framesThreadStatus =
                    QtConcurrent::run(&this->d->m_threadPool,
                                      &ColorSrcElementPrivate::readFrame,
                                      this->d);

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
        case AkElement::ElementStatePaused:
            this->d->m_run = false;
            this->d->m_framesThreadStatus.waitForFinished();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

ColorSrcElementPrivate::ColorSrcElementPrivate(ColorSrcElement *self):
    self(self)
{
    this->m_threadPool.setMaxThreadCount(1);
}

// Must be called with m_mutex locked.
QString ColorSrcElementPrivate::settingsId() const
{
    if (!this->m_media.startsWith(QString(COLORSRC_PREFIX)))
        return {};

    auto id = this->m_media.mid(QString(COLORSRC_PREFIX).size());

    for (auto &c: id)
        if (!c.isLetterOrNumber() && c != '_' && c != '-')
            c = '_';

    return id;
}

// Must be called with m_mutex locked.
QString ColorSrcElementPrivate::summary() const
{
    auto format = [] (QRgb color, int digits) {
        return QString("#%1")
                .arg(color & (0xffffffffu >> (32 - digits * 4)),
                     digits,
                     16,
                     QChar('0'));
    };

    return qAlpha(this->m_color) == 255?
                format(this->m_color, 6):
                format(this->m_color, 8);
}

// Must be called with m_mutex locked.
void ColorSrcElementPrivate::loadSettings()
{
    this->m_color = qRgba(0, 0, 0, 255);
    this->m_fps = AkFrac(30000, 1001);

    auto id = this->settingsId();

    if (id.isEmpty())
        return;

    QSettings settings;
    settings.beginGroup("ColorSrc");
    settings.beginGroup(id);

    this->m_color = settings.value("color", qRgba(0, 0, 0, 255)).toUInt();

    AkFrac fps(settings.value("fps", "30000/1001").toString());

    if (fps.isValid() && fps.value() > 0.0)
        this->m_fps = fps;

    settings.endGroup();
    settings.endGroup();
}

// Must be called with m_mutex locked.
void ColorSrcElementPrivate::saveSettings() const
{
    auto id = this->settingsId();

    if (id.isEmpty())
        return;

    QSettings settings;
    settings.beginGroup("ColorSrc");
    settings.beginGroup(id);
    settings.setValue("color", uint(this->m_color));
    settings.setValue("fps", this->m_fps.toString());
    settings.endGroup();
    settings.endGroup();
}

// Must be called with m_mutex locked.
AkVideoPacket ColorSrcElementPrivate::buildPacket() const
{
    AkVideoCaps caps(AkVideoCaps::Format_argbpack,
                     COLORSRC_WIDTH,
                     COLORSRC_HEIGHT,
                     this->m_fps);
    AkVideoPacket packet(caps);
    packet.fillRgb(this->m_color);

    return packet;
}

void ColorSrcElementPrivate::readFrame()
{
    qreal delayDiff = 0.0;

    while (this->m_run) {
        AkFrac fps;
        AkVideoPacket packet;

        {
            QMutexLocker locker(&this->m_mutex);
            fps = this->m_fps;
            packet = this->m_cachedPacket;
        }

        if (packet) {
            auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                                * fps.value() / 1e3);
            packet.setPts(pts);
            packet.setDuration(1);
            packet.setTimeBase(fps.invert());
            packet.setIndex(0);
            packet.setId(this->m_id);

            emit self->oStream(packet);
        }

        auto delay = (1000 / fps).value() + delayDiff;
        delayDiff = delay - qRound(delay);
        QThread::msleep(qRound(delay));
    }
}

#include "moc_colorsrcelement.cpp"
