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
#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "hypnotic.h"

class HypnoticPrivate
{
    public:
        Hypnotic *self {nullptr};
        QString m_description {QObject::tr("Hypnotic")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        Hypnotic::OpticMode m_mode {Hypnotic::OpticModeSpiral1};
        Hypnotic::OpticMode m_currentMode {Hypnotic::OpticModeSpiral1};
        int m_speedInc {0};
        int m_threshold {127};
        QSize m_frameSize;
        QRgb m_palette[256];
        AkVideoPacket m_opticalMap;
        quint8 m_speed {16};
        quint8 m_phase {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit HypnoticPrivate(Hypnotic *self);
        inline void createPalette();
        inline AkVideoPacket createOpticalMap(const QSize &size,
                                              Hypnotic::OpticMode mode) const;
        inline AkVideoPacket imageThreshold(const AkVideoPacket &src,
                                            int threshold) const;
};

Hypnotic::Hypnotic(QObject *parent):
      QObject(parent)
{
    this->d = new HypnoticPrivate(this);
    this->d->createPalette();
}

Hypnotic::~Hypnotic()
{
    delete this->d;
}

QString Hypnotic::description() const
{
    return this->d->m_description;
}

AkElementType Hypnotic::type() const
{
    return this->d->m_type;
}

AkElementCategory Hypnotic::category() const
{
    return this->d->m_category;
}

void *Hypnotic::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Hypnotic::create(const QString &id)
{
    Q_UNUSED(id)

    return new Hypnotic;
}

int Hypnotic::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Hypnotic",
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

Hypnotic::OpticMode Hypnotic::mode() const
{
    return this->d->m_mode;
}

int Hypnotic::speedInc() const
{
    return this->d->m_speedInc;
}

int Hypnotic::threshold() const
{
    return this->d->m_threshold;
}

void Hypnotic::deleteThis(void *userData) const
{
    delete reinterpret_cast<Hypnotic *>(userData);
}

QString Hypnotic::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Hypnotic/share/qml/main.qml");
}

void Hypnotic::controlInterfaceConfigure(QQmlContext *context,
                                                const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Hypnotic", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Hypnotic::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize
        || this->d->m_currentMode != this->d->m_mode) {
        this->d->m_speed = 16;
        this->d->m_phase = 0;
        this->d->m_opticalMap = this->d->createOpticalMap(frameSize, this->d->m_mode);
        this->d->m_frameSize = frameSize;
        this->d->m_currentMode = this->d->m_mode;
    }

    this->d->m_speed += this->d->m_speedInc;
    this->d->m_phase -= this->d->m_speed;
    auto diff = this->d->imageThreshold(src, this->d->m_threshold);

    for (int i = 0, y = 0; y < src.caps().height(); y++) {
        auto optLine = this->d->m_opticalMap.constLine(0, y);
        auto diffLine = diff.constLine(0, y);
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); i++, x++)
            dstLine[x] = this->d->m_palette[(quint8(int(optLine[x]) + int(this->d->m_phase)) ^ diffLine[x]) & 255];
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

void Hypnotic::setMode(OpticMode mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mode = mode;
    emit this->modeChanged(mode);
}

void Hypnotic::setSpeedInc(int speedInc)
{
    if (this->d->m_speedInc == speedInc)
        return;

    this->d->m_speedInc = speedInc;
    emit this->speedIncChanged(speedInc);
}

void Hypnotic::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void Hypnotic::resetMode()
{
    this->setMode(Hypnotic::OpticModeSpiral1);
}

void Hypnotic::resetSpeedInc()
{
    this->setSpeedInc(0);
}

void Hypnotic::resetThreshold()
{
    this->setThreshold(127);
}

QDataStream &operator >>(QDataStream &istream, Hypnotic::OpticMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<Hypnotic::OpticMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, Hypnotic::OpticMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

HypnoticPrivate::HypnoticPrivate(Hypnotic *self):
      self(self)
{

}

void HypnoticPrivate::createPalette()
{
    for (int i = 0; i < 112; i++) {
        this->m_palette[i] = qRgb(0, 0, 0);
        this->m_palette[i + 128] = qRgb(255, 255, 255);
    }

    for (int i = 0; i < 16; i++) {
        auto color = QRgb(16 * (i + 1) - 1);
        this->m_palette[i + 112] = qRgb(qRed(color), qGreen(color), qBlue(color));
        color = 255 - color;
        this->m_palette[i + 240] = qRgb(qRed(color), qGreen(color), qBlue(color));
    }
}

AkVideoPacket HypnoticPrivate::createOpticalMap(const QSize &size,
                                                Hypnotic::OpticMode mode) const
{
    AkVideoPacket opticalMap({AkVideoCaps::Format_y8,
                              size.width(),
                              size.height(),
                              {}});

    for (int y = 0; y < size.height(); y++) {
        auto line = opticalMap.line(0, y);
        auto yy = qreal(2 * y - size.height()) / (2 * size.width());

        for (int x = 0; x < size.width(); x++) {
            auto xx = qreal(2 * x - size.width()) / (2 * size.width());
            qreal r = sqrt(xx * xx + yy * yy);
            qreal at = atan2(xx, yy);

            switch (mode) {
            case Hypnotic::OpticModeSpiral1:
                line[x] = quint8(256.0 * at / M_PI + 4000.0 * r);
                break;
            case Hypnotic::OpticModeSpiral2: {
                int j = int(300.0 * r / 32.0);
                qreal rr = 300.0 * r - 32.0 * j;

                j *= 64;
                j += rr > 28.0? qRound(16.0 * (rr - 28.0)): 0;

                line[x] = quint8(4096.0 * at / M_PI + 1600.0 * r - j);

                break;
            }
            case Hypnotic::OpticModeParabola:
                line[x] = quint8(400.0 * yy / (0.3 * xx * xx + 0.1));
                break;
            case Hypnotic::OpticModeHorizontalStripe:
                line[x] = quint8(5120 * x / size.width());
                break;
            }
        }
    }

    return opticalMap;
}

AkVideoPacket HypnoticPrivate::imageThreshold(const AkVideoPacket &src,
                                              int threshold) const
{
    auto ocaps = src.caps();
    ocaps.setFormat(AkVideoCaps::Format_y8);
    AkVideoPacket diff(ocaps);
    diff.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcBits = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto diffBits = diff.line(0, y);

        for (int x = 0; x < src.caps().width(); x++)
            diffBits[x] = qGray(srcBits[x]) >= threshold? 255: 0;
    }

    return diff;
}

#include "moc_hypnotic.cpp"
