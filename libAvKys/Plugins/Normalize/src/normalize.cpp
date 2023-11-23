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

#include <akfrac.h>
#include <akpacket.h>
#include <akplugininfo.h>
#include <akpluginmanager.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "normalize.h"

#if 0
#define USE_FULLSWING
#endif

#ifdef USE_FULLSWING
    #define MIN_Y 0
    #define MAX_Y 255
#else
    #define MIN_Y 16
    #define MAX_Y 235
#endif

using HistogramType = quint64;

class NormalizePrivate
{
    public:
        Normalize *self {nullptr};
        QString m_description {QObject::tr("Normalize")};
        AkElementType m_type {AkElementType_VideoFilter};
        AkElementCategory m_category {AkElementCategory_VideoFilter};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_ayuvpack, 0, 0, {}}};

        explicit NormalizePrivate(Normalize *self);
        static void histogram(const AkVideoPacket &src, HistogramType *table);
        static void limits(const AkVideoPacket &src,
                           const HistogramType *histogram,
                           int &low, int &high);
        static void normalizationTable(const AkVideoPacket &src, quint8 *table);
};

Normalize::Normalize(QObject *parent):
      QObject(parent)
{
    this->d = new NormalizePrivate(this);

#ifdef USE_FULLSWING
    this->d->m_videoConverter.setYuvColorSpaceType(AkVideoConverter::YuvColorSpaceType_FullSwing);
#endif
}

Normalize::~Normalize()
{
    delete this->d;
}

QString Normalize::description() const
{
    return this->d->m_description;
}

AkElementType Normalize::type() const
{
    return this->d->m_type;
}

AkElementCategory Normalize::category() const
{
    return this->d->m_category;
}

void *Normalize::queryInterface(const QString &interfaceId)
{
    if (interfaceId == IAK_VIDEO_FILTER
        || interfaceId == IAK_UI_QML)
        return this;

    return IAkPlugin::queryInterface(interfaceId);
}

IAkElement *Normalize::create(const QString &id)
{
    Q_UNUSED(id)

    return new Normalize;
}

int Normalize::registerElements(const QStringList &args)
{
    QString pluginPath;
    auto keyMax = 2 * ((args.size() >> 1) - 1);

    for (int i = keyMax; i >= 0; i -= 2)
        if (args[i] == "pluginPath") {
            pluginPath = args.value(i + 1);

            break;
        }

    AkPluginInfo pluginInfo("VideoFilter/Normalize",
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

void Normalize::deleteThis(void *userData) const
{
    delete reinterpret_cast<Normalize *>(userData);
}

AkPacket Normalize::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    quint8 normTable[256];
    NormalizePrivate::normalizationTable(src, normTable);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const AkYuv *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<AkYuv *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto y = qBound<int>(MIN_Y, akCompY(pixel), MAX_Y);
            dstLine[x] = akYuv(normTable[y],
                               akCompU(pixel),
                               akCompV(pixel),
                               akCompA(pixel));
        }
    }

    if (dst)
        this->oStream(dst);

    return dst;
}

NormalizePrivate::NormalizePrivate(Normalize *self):
      self(self)
{

}

void NormalizePrivate::histogram(const AkVideoPacket &src, HistogramType *table)
{
    memset(table, 0, 256 * sizeof(HistogramType));

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const AkYuv *>(src.constLine(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            auto y = qBound<int>(MIN_Y, akCompY(pixel), MAX_Y);
            table[y]++;
        }
    }
}

void NormalizePrivate::limits(const AkVideoPacket &src,
                              const HistogramType *histogram,
                              int &low, int &high)
{
    // The lowest and highest levels must occupy at least 0.1 % of the image.
    auto thresholdIntensity =
        size_t(src.caps().width()) * size_t(src.caps().height()) / 1000;
    int intensity = 0;

    for (low = 0; low < 256; low++) {
        intensity += histogram[low];

        if (intensity > thresholdIntensity)
            break;
    }

    intensity = 0;

    for (high = 255; high > 0; high--) {
        intensity += histogram[high];

        if (intensity > thresholdIntensity)
            break;
    }
}

void NormalizePrivate::normalizationTable(const AkVideoPacket &src,
                                          quint8 *table)
{
    HistogramType histogram[256];
    NormalizePrivate::histogram(src, histogram);
    int low = 0;
    int high = 0;
    NormalizePrivate::limits(src, histogram, low, high);

    if (low == high) {
        for (int i = 0; i < 256; i++)
            table[i] = i;
    } else {
        auto yDiff = MAX_Y - MIN_Y;
        auto q = high - low;

        for (int i = 0; i < 256; i++) {
            auto value = (yDiff * (i - low) + q * MIN_Y) / q;
            table[i] = quint8(qBound<int>(MIN_Y, value, MAX_Y));
        }
    }
}

#include "moc_normalize.cpp"
