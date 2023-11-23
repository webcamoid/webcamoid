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

#include <QSize>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "life.h"

class LifePrivate
{
    public:
        Life *self {nullptr};
        QString m_description {QObject::tr("Life Game")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        QSize m_frameSize;
        AkVideoPacket m_prevFrame;
        AkVideoPacket m_lifeBuffer;
        QRgb m_lifeColor {qRgb(255, 255, 255)};
        int m_threshold {15};
        int m_lumaThreshold {15};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        explicit LifePrivate(Life *self);
        inline AkVideoPacket imageDiff(const AkVideoPacket &img1,
                                       const AkVideoPacket &img2,
                                       int threshold,
                                       int lumaThreshold) const;
        inline void updateLife();
};

Life::Life(QObject *parent):
    QObject(parent)
{
    this->d = new LifePrivate(this);
}

Life::~Life()
{
    delete this->d;
}

QString Life::description() const
{
    return this->d->m_description;
}

AkElementType Life::type() const
{
    return this->d->m_type;
}

AkElementCategory Life::category() const
{
    return this->d->m_category;
}

void *Life::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Life::create(const QString &id)
{
    Q_UNUSED(id)

    return new Life;
}

int Life::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Life",
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

QRgb Life::lifeColor() const
{
    return this->d->m_lifeColor;
}

int Life::threshold() const
{
    return this->d->m_threshold;
}

int Life::lumaThreshold() const
{
    return this->d->m_lumaThreshold;
}

void Life::deleteThis(void *userData) const
{
    delete reinterpret_cast<Life *>(userData);
}

QString Life::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Life/share/qml/main.qml");
}

void Life::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Life", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket Life::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    auto dst = src;
    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_frameSize) {
        this->d->m_lifeBuffer = AkVideoPacket();
        this->d->m_prevFrame = AkVideoPacket();
        this->d->m_frameSize = frameSize;
    }

    if (!this->d->m_prevFrame) {
        this->d->m_lifeBuffer = AkVideoPacket({AkVideoCaps::Format_y8,
                                               src.caps().width(),
                                               src.caps().height(),
                                               {}}, true);
    }
    else {
        // Compute the difference between previous and current frame,
        // and save it to the buffer.
        auto diff = this->d->imageDiff(this->d->m_prevFrame,
                                       src,
                                       this->d->m_threshold,
                                       this->d->m_lumaThreshold);

        for (int y = 0; y < this->d->m_lifeBuffer.caps().height(); y++) {
            auto diffLine = diff.constLine(0, y);
            auto lifeBufferLine = this->d->m_lifeBuffer.line(0, y);

            for (int x = 0; x < this->d->m_lifeBuffer.caps().width(); x++)
                lifeBufferLine[x] |= diffLine[x];
        }

        this->d->updateLife();

        auto lifeColor = qRgba(qRed(this->d->m_lifeColor),
                               qGreen(this->d->m_lifeColor),
                               qBlue(this->d->m_lifeColor),
                               255);

        for (int y = 0; y < src.caps().height(); y++) {
            auto iLine = this->d->m_lifeBuffer.constLine(0, y);
            auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));

            for (int x = 0; x < src.caps().width(); x++) {
                if (iLine[x])
                    oLine[x] = lifeColor;
            }
        }
    }

    this->d->m_prevFrame = src;

    if (dst)
        this->oStream(dst);

    return dst;
}

void Life::setLifeColor(QRgb lifeColor)
{
    if (this->d->m_lifeColor == lifeColor)
        return;

    this->d->m_lifeColor = lifeColor;
    emit this->lifeColorChanged(lifeColor);
}

void Life::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void Life::setLumaThreshold(int lumaThreshold)
{
    if (this->d->m_lumaThreshold == lumaThreshold)
        return;

    this->d->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void Life::resetLifeColor()
{
    this->setLifeColor(qRgb(255, 255, 255));
}

void Life::resetThreshold()
{
    this->setThreshold(15);
}

void Life::resetLumaThreshold()
{
    this->setLumaThreshold(15);
}

LifePrivate::LifePrivate(Life *self):
      self(self)
{

}

AkVideoPacket LifePrivate::imageDiff(const AkVideoPacket &img1,
                                     const AkVideoPacket &img2,
                                     int threshold,
                                     int lumaThreshold) const
{
    int width = qMin(img1.caps().width(), img2.caps().width());
    int height = qMin(img1.caps().height(), img2.caps().height());
    AkVideoPacket diff({AkVideoCaps::Format_y8, width, height, {}});

    for (int y = 0; y < height; y++) {
        auto line1 = reinterpret_cast<const QRgb *>(img1.constLine(0, y));
        auto line2 = reinterpret_cast<const QRgb *>(img2.constLine(0, y));
        auto lineDiff = diff.line(0, y);

        for (int x = 0; x < width; x++) {
            int r1 = qRed(line1[x]);
            int g1 = qGreen(line1[x]);
            int b1 = qBlue(line1[x]);

            int r2 = qRed(line2[x]);
            int g2 = qGreen(line2[x]);
            int b2 = qBlue(line2[x]);

            int dr = r1 - r2;
            int dg = g1 - g2;
            int db = b1 - b2;

            int colorDiff = dr * dr + dg * dg + db * db;

            lineDiff[x] = qSqrt(colorDiff / 3.0) >= threshold
                                  && qGray(line2[x]) >= lumaThreshold? 1: 0;
        }
    }

    return diff;
}

void LifePrivate::updateLife()
{
    AkVideoPacket lifeBuffer(this->m_lifeBuffer.caps(), true);

    for (int y = 1; y < lifeBuffer.caps().height() - 1; y++) {
        auto iLine = this->m_lifeBuffer.constLine(0, y);
        auto oLine = lifeBuffer.line(0, y);

        for (int x = 1; x < lifeBuffer.caps().width() - 1; x++) {
            int count = 0;

            for (int j = -1; j < 2; j++) {
                auto line = this->m_lifeBuffer.constLine(0, y + j);

                for (int i = -1; i < 2; i++)
                    count += line[x + i];
            }

            auto &ipixel = iLine[x];
            count -= ipixel;

            if ((ipixel && count == 2) || count == 3)
                oLine[x] = 1;
        }
    }

    this->m_lifeBuffer = lifeBuffer;
}

#include "moc_life.cpp"
