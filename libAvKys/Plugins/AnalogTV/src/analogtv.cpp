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

#include <QRandomGenerator>
#include <QSize>
#include <QtMath>
#include <qrgb.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "analogtv.h"

class AnalogTVPrivate
{
    public:
        AnalogTV *self {nullptr};
        QString m_description {QObject::tr("Analog TV")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        IAkPropertyDouble m_vsync {QObject::tr("Vertical synchonization"), 0.02};
        IAkPropertyInt m_xOffset {QObject::tr("X offset"), 0};
        IAkPropertyDouble m_hsyncFactor {QObject::tr("Horizontal synchonization factor"), 5.0};
        IAkPropertyInt m_hsyncSmoothness {QObject::tr("Horizontal synchonization smoothness"), 20};
        IAkPropertyDouble m_hueFactor {QObject::tr("Hue factor"), 1.0};
        IAkPropertyDouble m_noise {QObject::tr("Noise"), 0.1};
        qreal m_yOffset {0.0};
        qreal m_sign {1.0};
        QSize m_curSize;
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};
        qint64 *m_aiMultTable {nullptr};
        qint64 *m_aoMultTable {nullptr};
        qint64 *m_alphaDivTable {nullptr};

        explicit AnalogTVPrivate(AnalogTV *self);
        void createLumaOffset(const AkVideoPacket &src,
                              qreal factor,
                              int *lumaOffset) const;
        void smoothLumaOffset(int *lumaOffset,
                              int height,
                              int smoothness) const;
        AkVideoPacket applyVSync(const AkVideoPacket &src);
        AkVideoPacket applyHSync(const AkVideoPacket &src,
                                 const int *lumaOffset,
                                 int xOffset);
        inline void rotateHue(QRgb &pixel, int degrees) const;
        void applyChromaDephasing(AkVideoPacket &src,
                                  const int *lumaOffset,
                                  qreal hueFactor) const;
        void applyNoise(AkVideoPacket &src, qreal persent) const;

        template<typename T>
        inline T mod(T value, T mod) const
        {
            return (value % mod + mod) % mod;
        }
};

AnalogTV::AnalogTV(QObject *parent):
      QObject(parent)
{
    this->d = new AnalogTVPrivate(this);

    this->registerProperty("vsync", &this->d->m_vsync);
    this->registerProperty("xOffset", &this->d->m_xOffset);
    this->registerProperty("hsyncFactor", &this->d->m_hsyncFactor);
    this->registerProperty("hsyncSmoothness", &this->d->m_hsyncSmoothness);
    this->registerProperty("hueFactor", &this->d->m_hueFactor);
    this->registerProperty("noise", &this->d->m_noise);

    constexpr qint64 maxAi = 255;
    qint64 maxAi2 = maxAi * maxAi;
    constexpr qint64 alphaMult = 1 << 16;
    this->d->m_aiMultTable = new qint64 [alphaMult];
    this->d->m_aoMultTable = new qint64 [alphaMult];
    this->d->m_alphaDivTable = new qint64 [alphaMult];

    for (qint64 ai = 0; ai < 256; ai++)
        for (qint64 ao = 0; ao < 256; ao++) {
            auto alphaMask = (ai << 8) | ao;
            auto a = maxAi2 - (maxAi - ai) * (maxAi - ao);
            this->d->m_aiMultTable[alphaMask] = a? alphaMult * ai * maxAi / a: 0;
            this->d->m_aoMultTable[alphaMask] = a? alphaMult * ao * (maxAi - ai) / a: 0;
            this->d->m_alphaDivTable[alphaMask] = a / maxAi;
        }
}

AnalogTV::~AnalogTV()
{
    delete [] this->d->m_aiMultTable;
    delete [] this->d->m_aoMultTable;
    delete [] this->d->m_alphaDivTable;
    delete this->d;
}

QString AnalogTV::description() const
{
    return this->d->m_description;
}

AkElementType AnalogTV::type() const
{
    return this->d->m_type;
}

AkElementCategory AnalogTV::category() const
{
    return this->d->m_category;
}

void *AnalogTV::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *AnalogTV::create(const QString &id)
{
    Q_UNUSED(id)

    return new AnalogTV;
}

int AnalogTV::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/AnalogTV",
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

void AnalogTV::deleteThis(void *userData) const
{
    delete reinterpret_cast<AnalogTV *>(userData);
}

QString AnalogTV::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/AnalogTV/share/qml/main.qml");
}

void AnalogTV::controlInterfaceConfigure(QQmlContext *context,
                                         const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("AnalogTV", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket AnalogTV::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    QSize frameSize(src.caps().width(), src.caps().height());

    if (frameSize != this->d->m_curSize) {
        this->d->m_yOffset = 0.0;
        this->d->m_curSize = frameSize;
    }

    auto lumaOffset = new int [src.caps().height()];
    this->d->createLumaOffset(src, this->d->m_hsyncFactor, lumaOffset);
    this->d->smoothLumaOffset(lumaOffset,
                              src.caps().height(),
                              this->d->m_hsyncSmoothness);
    auto dst = this->d->applyHSync(src, lumaOffset, this->d->m_xOffset);
    this->d->applyChromaDephasing(dst, lumaOffset, this->d->m_hueFactor);
    delete [] lumaOffset;
    this->d->applyNoise(dst, this->d->m_noise);
    dst = this->d->applyVSync(dst);

    if (dst)
        this->oStream(dst);

    return dst;
}

void AnalogTVPrivate::createLumaOffset(const AkVideoPacket &src,
                                              qreal factor,
                                              int *lumaOffset) const
{
    auto heightLuma = new quint8 [src.caps().height()];
    quint64 avgLuma = 0;

    for (int y = 0; y < src.caps().height(); y++) {
        auto line = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        quint64 avgLineLuma = 0;

        for (int x = 0; x < src.caps().height(); x++) {
            auto luma = qGray(line[x]);
            avgLuma += luma;
            avgLineLuma += luma;
        }

        heightLuma[y] = quint8(avgLineLuma / src.caps().height());
    }

    avgLuma /= size_t(src.caps().width()) * size_t(src.caps().height());

    for (int y = 0; y < src.caps().height(); y++)
        lumaOffset[y] = qRound(factor * (int(avgLuma) - int(heightLuma[y])));

    delete [] heightLuma;
}

void AnalogTVPrivate::smoothLumaOffset(int *lumaOffset,
                                              int height,
                                              int smoothness) const
{
    auto sumLumaOffset = new qint64 [height + 1];
    sumLumaOffset[0] = 0;

    for (int y = 0; y < height; y++)
        sumLumaOffset[y + 1] = lumaOffset[y] + sumLumaOffset[y];

    smoothness = qMax(smoothness, 0);

    for (int y = 0; y < height; y++) {
        int y2 = y << 1;
        int minY = (y2 - smoothness) >> 1;
        int maxY = ((y2 + smoothness) >> 1) + 1;
        minY = qMax(minY, 0);
        maxY = qMin(maxY, height);
        int n = maxY - minY;

        if (n != 0)
            lumaOffset[y] = (sumLumaOffset[maxY] - sumLumaOffset[minY]) / n;
    }

    delete [] sumLumaOffset;
}

AkVideoPacket AnalogTVPrivate::applyVSync(const AkVideoPacket &src)
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int offset = int(this->m_yOffset);

    memcpy(dst.line(0, 0),
           src.constLine(0, src.caps().height() - offset - 1),
           size_t(src.lineSize(0) * offset));
    memcpy(dst.line(0, offset),
           src.constLine(0, 0),
           size_t(src.lineSize(0) * (src.caps().height() - offset)));

    qreal vsync = this->m_vsync;

    if (!qFuzzyCompare(this->m_yOffset, 0.0) && qFuzzyCompare(vsync, 0.0)) {
        auto yOffset = this->m_sign > 0.0?
                           this->m_yOffset:
                           src.caps().height() - this->m_yOffset;
        vsync = 0.1 * this->m_sign * yOffset / src.caps().height();
    }

    this->m_yOffset += vsync * dst.caps().height();
    this->m_sign = vsync < 0.0? -1.0: 1.0;

    if (int(this->m_yOffset) == 0 && qFuzzyCompare(this->m_vsync, 0.0))
        this->m_yOffset = 0.0;

    if (this->m_yOffset >= qreal(src.caps().height()))
        this->m_yOffset = 0.0;
    else if (this->m_yOffset < 0.0)
        this->m_yOffset = src.caps().height();

    return dst;
}

AkVideoPacket AnalogTVPrivate::applyHSync(const AkVideoPacket &src,
                                                 const int *lumaOffset,
                                                 int xOffset)
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); y++) {
        int offset = (lumaOffset[y] + xOffset) % src.caps().width();

        if (offset < 0) {
            memcpy(dst.line(0, y),
                   src.constLine(0, y) - offset * sizeof(QRgb),
                   (src.caps().width() + offset) * sizeof(QRgb));
            memcpy(dst.line(0, y) + (src.caps().width() + offset) * sizeof(QRgb),
                   src.constLine(0, y),
                   -offset * sizeof(QRgb));
        } else {
            memcpy(dst.line(0, y),
                   src.constLine(0, y) + (src.caps().width() - offset) * sizeof(QRgb),
                   offset * sizeof(QRgb));
            memcpy(dst.line(0, y) + offset * sizeof(QRgb),
                   src.constLine(0, y),
                   (src.caps().width() - offset) * sizeof(QRgb));
        }
    }

    return dst;
}

void AnalogTVPrivate::rotateHue(QRgb &pixel, int degrees) const
{
    if (degrees == 0)
        return;

    int r = qRed(pixel);
    int g = qGreen(pixel);
    int b = qBlue(pixel);
    int a = qAlpha(pixel);

    int max = qMax(r, qMax(g, b));
    int min = qMin(r, qMin(g, b));

    if (min == max) {
        pixel = qRgba(min, min, min, a);
    } else {
        // Ccalculate hue

        int c = max - min;
        int h = 0;

        if (max == r)
            h = this->mod(g - b, 6 * c);
        else if (max == g)
            h = b - r + 2 * c;
        else
            h = r - g + 4 * c;

        h = 60 * h / c;

               // Dephase hue

        h = qAbs((h + degrees) % 360);

               // Convert HSL to RGB

        int cc = c * (60 - qAbs((h % 120) - 60)) / 60;

        if (h >= 0 && h < 60) {
            r = c;
            g = cc;
            b = 0;
        } else if (h >= 60 && h < 120) {
            r = cc;
            g = c;
            b = 0;
        } else if (h >= 120 && h < 180) {
            r = 0;
            g = c;
            b = cc;
        } else if (h >= 180 && h < 240) {
            r = 0;
            g = cc;
            b = c;
        } else if (h >= 240 && h < 300) {
            r = cc;
            g = 0;
            b = c;
        } else if (h >= 300 && h < 360) {
            r = c;
            g = 0;
            b = cc;
        } else {
            r = 0;
            g = 0;
            b = 0;
        }

        r = r + min;
        g = g + min;
        b = b + min;

               // Write pixel

        pixel = qRgba(r, g, b, a);
    }
}

void AnalogTVPrivate::applyChromaDephasing(AkVideoPacket &src,
                                                  const int *lumaOffset,
                                                  qreal hueFactor) const
{
    for (int y = 0; y < src.caps().height(); y++) {
        auto line = reinterpret_cast<QRgb *>(src.line(0, y));
        auto hueOffset = qRound(hueFactor * lumaOffset[y]);

        for (int x = 0; x < src.caps().width(); x++)
            this->rotateHue(line[x], hueOffset);
    }
}

void AnalogTVPrivate::applyNoise(AkVideoPacket &src, qreal persent) const
{
    auto peper = qRound64(persent * src.caps().width() * src.caps().height());

    for (size_t i = 0; i < peper; i++) {
        int gray = QRandomGenerator::global()->bounded(256);
        int alpha = QRandomGenerator::global()->bounded(256);
        int x = QRandomGenerator::global()->bounded(src.caps().width());
        int y = QRandomGenerator::global()->bounded(src.caps().height());
        auto pixel = src.pixel<QRgb>(0, x, y);

        qint64 ro = qRed(pixel);
        qint64 go = qGreen(pixel);
        qint64 bo = qBlue(pixel);
        qint64 ao = qAlpha(pixel);

        auto alphaMask = (alpha << 8) | ao;
        auto graym = gray * this->m_aiMultTable[alphaMask];
        qint64 rt = (graym + ro * this->m_aoMultTable[alphaMask]) >> 16;
        qint64 gt = (graym + go * this->m_aoMultTable[alphaMask]) >> 16;
        qint64 bt = (graym + bo * this->m_aoMultTable[alphaMask]) >> 16;
        qint64 &at = this->m_alphaDivTable[alphaMask];

        src.setPixel(0, x, y, qRgba(int(rt), int(gt), int(bt), int(at)));
    }
}

AnalogTVPrivate::AnalogTVPrivate(AnalogTV *self):
      self(self)
{

}

#include "moc_analogtv.cpp"
