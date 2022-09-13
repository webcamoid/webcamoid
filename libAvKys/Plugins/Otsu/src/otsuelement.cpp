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

#include <QImage>
#include <QQmlContext>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "otsuelement.h"

class OtsuElementPrivate
{
    public:
        int m_levels {2};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_gray8, 0, 0, {}}};

        QVector<int> histogram(const QImage &image) const;
        QVector<qreal> buildTables(const QVector<int> &histogram) const;
        void forLoop(qreal *maxSum,
                     QVector<int> *thresholds,
                     const QVector<qreal> &H,
                     int u,
                     int vmax,
                     int level,
                     int levels,
                     QVector<int> *index) const;
        QVector<int> otsu(QVector<int> histogram, int classes) const;
        QImage threshold(const QImage &src,
                         const QVector<int> &thresholds,
                         const QVector<int> &colors) const;
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
    auto src = this->d->m_videoConverter.convertToImage(packet);

    if (src.isNull())
        return AkPacket();

    int levels = this->d->m_levels;

    if (levels < 2)
        levels = 2;

    auto hist = this->d->histogram(src);
    auto thresholds = this->d->otsu(hist, levels);
    QVector<int> colors(levels);

    for (int i = 0; i < levels; i++)
        colors[i] = 255 * i / (levels - 1);

    auto oFrame = this->d->threshold(src, thresholds, colors);
    auto oPacket = this->d->m_videoConverter.convert(oFrame, packet);

    if (oPacket)
        emit this->oStream(oPacket);

    return oPacket;
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

QVector<int> OtsuElementPrivate::histogram(const QImage &image) const
{
    QVector<int> histogram(256, 0);

    for (int y = 0; y < image.height(); y++) {
        auto line = reinterpret_cast<const quint8 *>(image.constScanLine(y));

        for (int x = 0; x < image.width(); x++)
            histogram[line[x]]++;
    }

    // Since we use sum tables add one more to avoid unexistent colors.
    for (int i = 0; i < histogram.size(); i++)
        histogram[i]++;

    return histogram;
}

QVector<qreal> OtsuElementPrivate::buildTables(const QVector<int> &histogram) const
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
            hLine[v] = qPow(S[v] - S[u], 2) / (P[v] - P[u]);
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

QVector<int> OtsuElementPrivate::otsu(QVector<int> histogram,
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

QImage OtsuElementPrivate::threshold(const QImage &src,
                                     const QVector<int> &thresholds,
                                     const QVector<int> &colors) const
{
    QImage dst(src.size(), src.format());
    QVector<quint8> colorTable(256);
    int j = 0;

    for (int i = 0; i < colorTable.size(); i++) {
        if (j < thresholds.size() && i >= thresholds[j])
            j++;

        colorTable[i] = quint8(colors[j]);
    }

    for (int y = 0; y < src.height(); y++) {
        auto srcLine = src.constScanLine(y);
        auto dstLine = dst.scanLine(y);

        for (int x = 0; x < src.width(); x++)
            dstLine[x] = colorTable[srcLine[x]];
    }

    return dst;
}

#include "moc_otsuelement.cpp"
