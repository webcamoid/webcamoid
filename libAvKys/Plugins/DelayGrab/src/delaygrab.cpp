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

#include <QMutex>
#include <QRandomGenerator>
#include <QSize>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "delaygrab.h"

enum DelayGrabMode
{
    // Random delay with square distribution
    DelayGrabModeRandomSquare,
    // Vertical stripes of increasing delay outward from center
    DelayGrabModeVerticalIncrease,
    // Horizontal stripes of increasing delay outward from center
    DelayGrabModeHorizontalIncrease,
    // Rings of increasing delay outward from center
    DelayGrabModeRingsIncrease
};

class DelayGrabPrivate;

class PropertiesCallbacks: public IAkNumericPropertyCallbacks<qint32>
{
    public:
        PropertiesCallbacks(DelayGrabPrivate *self);
        void valueChanged(qint32 value) override;

    private:
        DelayGrabPrivate *self;
};

class DelayGrabPrivate
{
    public:
        DelayGrab *self {nullptr};
        QString m_description {QObject::tr("Delay Grab")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyIntMenu m_mode {QObject::tr("Mode"),
                                   DelayGrabModeRingsIncrease, {
                                       {"randomSquare"      , QObject::tr("Random square"      ), DelayGrabModeRandomSquare      },
                                       {"verticalIncrease"  , QObject::tr("Vertical increase"  ), DelayGrabModeVerticalIncrease  },
                                       {"horizontalIncrease", QObject::tr("Horizontal increase"), DelayGrabModeHorizontalIncrease},
                                       {"ringsIncrease"     , QObject::tr("Rings increase"     ), DelayGrabModeRingsIncrease     }
                                   }};
        IAkPropertyInt m_blockSize {QObject::tr("Block size"), 2};
        IAkPropertyInt m_nFrames {QObject::tr("Frames"), 71};
        PropertiesCallbacks *m_propertiesCallbacks {nullptr};
        QMutex m_mutex;
        QSize m_frameSize;
        int m_curBlockSize {2};
        int m_curNFrames {71};
        QVector<AkVideoPacket> m_frames;
        AkVideoPacket m_delayMap;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit DelayGrabPrivate(DelayGrab *self);
        ~DelayGrabPrivate();
        void updateDelaymap();
};

DelayGrab::DelayGrab(QObject *parent):
      QObject(parent)
{
    this->d = new DelayGrabPrivate(this);
    this->registerProperty("mode", &this->d->m_mode);
    this->registerProperty("blockSize", &this->d->m_blockSize);
    this->registerProperty("nFrames", &this->d->m_nFrames);

    this->d->m_mode.subscribe(this->d->m_propertiesCallbacks);
    this->d->m_blockSize.subscribe(this->d->m_propertiesCallbacks);
    this->d->m_nFrames.subscribe(this->d->m_propertiesCallbacks);
}

DelayGrab::~DelayGrab()
{
    delete this->d;
}

QString DelayGrab::description() const
{
    return this->d->m_description;
}

AkElementType DelayGrab::type() const
{
    return this->d->m_type;
}

AkElementCategory DelayGrab::category() const
{
    return this->d->m_category;
}

void *DelayGrab::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *DelayGrab::create(const QString &id)
{
    Q_UNUSED(id)

    return new DelayGrab;
}

int DelayGrab::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/DelayGrab",
                            this->d->m_description,
                            pluginPath,
                            QStringList(),
                            this->d->m_type,
                            this->d->m_category,
                            0,
                            this);
    akPluginManager->registerPlugin(pluginInfo);

    return 0;
}

void DelayGrab::deleteThis(void *userData) const
{
    delete reinterpret_cast<DelayGrab *>(userData);
}

QString DelayGrab::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/DelayGrab/share/qml/main.qml");
}

void DelayGrab::controlInterfaceConfigure(QQmlContext *context,
                                                 const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("DelayGrab", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket DelayGrab::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_frames.clear();
        this->d->m_frameSize = frameSize;
        this->d->updateDelaymap();
    }

    this->d->m_mutex.lock();

    this->d->m_frames << src;
    int diff = this->d->m_frames.size() - this->d->m_curNFrames;

    for (int i = 0; i < diff; i++)
        this->d->m_frames.removeFirst();

    if (this->d->m_frames.isEmpty()) {
        this->d->m_mutex.unlock();

        if (packet)
            this->oStream(packet);

        return packet;
    }

    if (!this->d->m_delayMap) {
        this->d->m_mutex.unlock();

        if (packet)
            this->oStream(packet);

        return packet;
    }

    int delayMapWidth = src.caps().width() / this->d->m_curBlockSize;
    int delayMapHeight = src.caps().height() / this->d->m_curBlockSize;

    AkVideoPacket dst(src.caps(), true);
    dst.copyMetadata(src);
    size_t oLineSize = dst.lineSize(0);

    // Copy image blockwise to frame buffer
    for (int y = 0; y < delayMapHeight; y++) {
        auto yb = this->d->m_curBlockSize * y;
        auto dstLine = dst.line(0, yb);
        auto delayLine =
            reinterpret_cast<quint16 *>(this->d->m_delayMap.line(0, y));

        for (int x = 0; x < delayMapWidth; x++) {
            int curFrame = qAbs(this->d->m_frames.size() - delayLine[x] - 1)
                           % this->d->m_frames.size();
            auto &frame = this->d->m_frames[curFrame];
            size_t iLineSize = frame.lineSize(0);
            size_t xoffset = this->d->m_curBlockSize * x * sizeof(QRgb);
            auto srcLineX = frame.constLine(0, yb) + xoffset;
            auto dstLineX = dstLine + xoffset;

            // copy block
            for (int j = 0; j < this->d->m_curBlockSize; j++) {
                memcpy(dstLineX, srcLineX, this->d->m_curBlockSize * sizeof(QRgb));
                srcLineX += iLineSize;
                dstLineX += oLineSize;
            }
        }
    }

    this->d->m_mutex.unlock();

    if (dst)
        this->oStream(dst);

    return dst;
}

DelayGrabPrivate::DelayGrabPrivate(DelayGrab *self):
      self(self)
{
    this->m_propertiesCallbacks = new PropertiesCallbacks(this);
}

DelayGrabPrivate::~DelayGrabPrivate()
{
    delete this->m_propertiesCallbacks;
}

void DelayGrabPrivate::updateDelaymap()
{
    QMutexLocker locker(&this->m_mutex);

    if (this->m_frameSize.isEmpty())
        return;

    this->m_curNFrames = qMax(this->m_nFrames.value(), 1);
    this->m_curBlockSize = qMax(this->m_blockSize.value(), 1);
    int delayMapWidth = this->m_frameSize.width() / this->m_curBlockSize;
    int delayMapHeight = this->m_frameSize.height() / this->m_curBlockSize;

    this->m_delayMap = AkVideoPacket({AkVideoCaps::Format_y16,
                                      delayMapWidth,
                                      delayMapHeight,
                                      {}});

    int minX = -(delayMapWidth >> 1);
    int maxX = delayMapWidth >> 1;
    int minY = -(delayMapHeight >> 1);
    int maxY = delayMapHeight >> 1;

    auto mode = this->m_mode.value();

    for (int y = minY; y < maxY; y++) {
        auto delayLine =
            reinterpret_cast<quint16 *>(this->m_delayMap.line(0, y - minY));

        for (int x = minX; x < maxX; x++) {
            int value = 0;

            switch (mode) {
            case DelayGrabModeRandomSquare: {
                // Random delay with square distribution
                auto d = QRandomGenerator::global()->bounded(1.0);
                value = qRound(16.0 * d * d);

                break;
            }
            case DelayGrabModeVerticalIncrease: {
                value = qAbs(x) >> 1;

                break;
            }
            case DelayGrabModeHorizontalIncrease: {
                value = qAbs(y) >> 1;

                break;
            }
            default: {
                // Rings of increasing delay outward from center
                value = qRound(sqrt(x * x + y * y) / 2.0);

                break;
            }
            }

            // Clip values
            delayLine[x - minX] = value % this->m_curNFrames;
        }
    }
}

PropertiesCallbacks::PropertiesCallbacks(DelayGrabPrivate *self):
    self(self)
{

}

void PropertiesCallbacks::valueChanged(qint32 value)
{
    Q_UNUSED(value)

    self->updateDelaymap();
}

#include "moc_delaygrab.cpp"
