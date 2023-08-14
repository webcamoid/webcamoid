/* Webcamoid, webcam capture application.
 * Copyright (C) 2022  Gonzalo Exequiel Pedone
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

#include <QQmlContext>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "otsuelement.h"

using HistogramType = quint64;

class OtsuElementPrivate
{
    public:
        int m_levels {2};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_graya8pack, 0, 0, {}}};

        QVector<HistogramType> histogram(const AkVideoPacket &src) const;
        QVector<qreal> buildTables(const QVector<HistogramType> &histogram) const;
        void forLoop(qreal *maxSum,
                     QVector<int> *thresholds,
                     const QVector<qreal> &H,
                     int u,
                     int vmax,
                     int level,
                     int levels,
                     QVector<int> *index) const;
        QVector<int> otsu(const QVector<HistogramType> &histogram,
                          int classes) const;
        AkVideoPacket threshold(const AkVideoPacket &src,
                                const QVector<int> &thresholds,
                                int levels) const;
};

OtsuElement::OtsuElement():
    AkElement()
{
    this->d = new OtsuElementPrivate;
}

OtsuElement::~OtsuElement()
{
    delete this->d;
}

int OtsuElement::levels() const
{
    return this->d->m_levels;
}

QString OtsuElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Otsu/share/qml/main.qml");
}

void OtsuElement::controlInterfaceConfigure(QQmlContext *context,
                                            const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Otsu", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket OtsuElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    int levels = qMax(this->d->m_levels, 2);
    auto hist = this->d->histogram(src);
    auto thresholds = this->d->otsu(hist, levels);
    auto dst = this->d->threshold(src, thresholds, levels);

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void OtsuElement::setLevels(int levels)
{
    if (this->d->m_levels == levels)
        return;

    this->d->m_levels = levels;
    emit this->levelsChanged(levels);
}

void OtsuElement::resetLevels()
{
    this->setLevels(5);
}

QVector<HistogramType> OtsuElementPrivate::histogram(const AkVideoPacket &src) const
{
    QVector<HistogramType> histogram(256, 0);

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));

        for (int x = 0; x < src.caps().width(); x++)
            histogram[srcLine[x] >> 8]++;
    }

    return histogram;
}

QVector<qreal> OtsuElementPrivate::buildTables(const QVector<HistogramType> &histogram) const
{
    // Create cumulative sum tables.
    QVector<quint64> P(histogram.size() + 1);
    QVector<quint64> S(histogram.size() + 1);
    P[0] = 0;
    S[0] = 0;

    quint64 sumP = 0;
    quint64 sumS = 0;

    for (int i = 0; i < histogram.size(); i++) {
        sumP += quint64(histogram[i]);
        sumS += quint64(i * histogram[i]);
        P[i + 1] = sumP;
        S[i + 1] = sumS;
    }

    // Calculate the between-class variance for the interval u-v
    QVector<qreal> H(histogram.size() * histogram.size(), 0.);

    for (int u = 0; u < histogram.size(); u++) {
        auto hLine = H.data() + u * histogram.size();

        for (int v = u + 1; v < histogram.size(); v++)
            if (P[v] == P[u]) {
                hLine[v] = 0;
            } else {
                auto sDiff = S[v] - S[u];
                hLine[v] = sDiff * sDiff / (P[v] - P[u]);
            }
    }

    return H;
}

void OtsuElementPrivate::forLoop(qreal *maxSum,
                                 QVector<int> *thresholds,
                                 const QVector<qreal> &H,
                                 int u,
                                 int vmax,
                                 int level,
                                 int levels,
                                 QVector<int> *index) const
{
    int classes = index->size() - 1;

    for (int i = u; i < vmax; i++) {
        (*index)[level] = i;

        if (level + 1 >= classes) {
            // Reached the end of the for loop.

            // Calculate the quadratic sum of all intervals.
            qreal sum = 0.;

            for (int c = 0; c < classes; c++) {
                int u = index->at(c);
                int v = index->at(c + 1);
                sum += H[v + u * levels];
            }

            if (*maxSum < sum) {
                // Return calculated threshold.
                *thresholds = index->mid(1, thresholds->size());
                *maxSum = sum;
            }
        } else
            // Start a new for loop level, one position after current one.
            this->forLoop(maxSum,
                          thresholds,
                          H,
                          i + 1,
                          vmax + 1,
                          level + 1,
                          levels,
                          index);
    }
}

QVector<int> OtsuElementPrivate::otsu(const QVector<HistogramType> &histogram,
                                      int classes) const
{
    qreal maxSum = 0.;
    QVector<int> thresholds(classes - 1, 0);
    auto H = this->buildTables(histogram);
    QVector<int> index(classes + 1);
    index[0] = 0;
    index[index.size() - 1] = histogram.size() - 1;

    this->forLoop(&maxSum,
                  &thresholds,
                  H,
                  1,
                  histogram.size() - classes + 1,
                  1,
                  histogram.size(), &index);

    return thresholds;
}

AkVideoPacket OtsuElementPrivate::threshold(const AkVideoPacket &src,
                                            const QVector<int> &thresholds,
                                            int levels) const
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);
    quint8 colorTable[256];
    int j = 0;

    for (int i = 0; i < 256; i++) {
        if (j < levels - 1 && i >= thresholds[j])
            j++;

        colorTable[i] = 255 * j / (levels - 1);
    }

    for (int y = 0; y < src.caps().height(); y++) {
        auto srcLine = reinterpret_cast<const quint16 *>(src.constLine(0, y));
        auto dstLine = reinterpret_cast<quint16 *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            auto &pixel = srcLine[x];
            dstLine[x] = (colorTable[pixel >> 8] << 8) | (pixel & 0xff);
        }
    }

    return dst;
}

#include "moc_otsuelement.cpp"
