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
#include <QQmlContext>
#include <QRandomGenerator>
#include <QtMath>
#include <qrgb.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "rippleelement.h"

class RippleElementPrivate
{
    public:
        RippleElement::RippleMode m_mode {RippleElement::RippleModeMotionDetect};
        int m_amplitude {256};
        int m_decay {8};
        int m_threshold {15};
        int m_lumaThreshold {15};
        int m_minDropSize {3};
        int m_maxDropSize {127};
        qreal m_dropSigma {1.0};
        qreal m_dropProbability {1.0};
        AkCaps m_caps;
        AkVideoPacket m_prevFrame;
        AkVideoPacket m_rippleBuffer[2];
        int m_curRippleBuffer {0};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        AkVideoPacket imageDiff(const AkVideoPacket &img1,
                                const AkVideoPacket &img2,
                                int threshold,
                                int lumaThreshold, int strength);
        void addDrop(AkVideoPacket &buffer, const AkVideoPacket &drop) const;
        void ripple(const AkVideoPacket &topBuffer,
                    AkVideoPacket &bottomBuffer,
                    int decay) const;
        AkVideoPacket applyWater(const AkVideoPacket &src,
                                 const AkVideoPacket &water) const;
        AkVideoPacket drop(int bufferWidth,
                           int bufferHeight,
                           int dropWidth,
                           int dropHeight,
                           int amplitude,
                           qreal sigma) const;
};

RippleElement::RippleElement(): AkElement()
{
    this->d = new RippleElementPrivate;
}

RippleElement::~RippleElement()
{
    delete this->d;
}

RippleElement::RippleMode RippleElement::mode() const
{
    return this->d->m_mode;
}

int RippleElement::amplitude() const
{
    return this->d->m_amplitude;
}

int RippleElement::decay() const
{
    return this->d->m_decay;
}

int RippleElement::threshold() const
{
    return this->d->m_threshold;
}

int RippleElement::lumaThreshold() const
{
    return this->d->m_lumaThreshold;
}

int RippleElement::minDropSize() const
{
    return this->d->m_minDropSize;
}

int RippleElement::maxDropSize() const
{
    return this->d->m_maxDropSize;
}

qreal RippleElement::dropSigma() const
{
    return this->d->m_dropSigma;
}

qreal RippleElement::dropProbability() const
{
    return this->d->m_dropProbability;
}

QString RippleElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Ripple/share/qml/main.qml");
}

void RippleElement::controlInterfaceConfigure(QQmlContext *context,
                                              const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Ripple", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket RippleElement::iVideoStream(const AkVideoPacket &packet)
{
    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst;

    if (packet.caps() != this->d->m_caps) {
        this->d->m_prevFrame = AkVideoPacket();
        this->d->m_caps = packet.caps();
    }

    if (!this->d->m_prevFrame) {
        dst = src;
        this->d->m_rippleBuffer[0] = AkVideoPacket(src.caps(), true);
        this->d->m_rippleBuffer[1] = AkVideoPacket(src.caps(), true);
        this->d->m_curRippleBuffer = 0;
    } else {
        AkVideoPacket drop;

        if (this->d->m_mode == RippleModeMotionDetect) {
            // Compute the difference between previous and current frame,
            // and save it to the buffer.
            drop = this->d->imageDiff(this->d->m_prevFrame,
                                      src,
                                      this->d->m_threshold,
                                      this->d->m_lumaThreshold,
                                      this->d->m_amplitude);
        } else {
            auto dropProbability = qBound(0.0, this->d->m_dropProbability, 1.0);
            static std::default_random_engine generator;
            std::bernoulli_distribution distribution(dropProbability);

            if (distribution(generator)) {
                auto minDropSize = qMax(this->d->m_minDropSize, 0);
                auto maxDropSize = qMax(this->d->m_maxDropSize, 0);

                if (minDropSize > maxDropSize)
                    std::swap(minDropSize, maxDropSize);

                auto dropSize =
                        QRandomGenerator::global()->bounded(minDropSize,
                                                            maxDropSize);
                auto amplitude =
                        QRandomGenerator::global()->bounded(-this->d->m_amplitude,
                                                            this->d->m_amplitude);
                drop = this->d->drop(src.caps().width(),
                                     src.caps().height(),
                                     dropSize,
                                     dropSize,
                                     amplitude,
                                     this->d->m_dropSigma);
            }
        }

        auto &topBuffer = this->d->m_rippleBuffer[this->d->m_curRippleBuffer];
        auto &bottomBuffer = this->d->m_rippleBuffer[1 - this->d->m_curRippleBuffer];
        this->d->addDrop(topBuffer, drop);
        this->d->addDrop(bottomBuffer, drop);
        this->d->ripple(topBuffer, bottomBuffer, this->d->m_decay);
        this->d->m_curRippleBuffer = 1 - this->d->m_curRippleBuffer;

        // Apply buffer.
        dst = this->d->applyWater(src, topBuffer);
    }

    this->d->m_prevFrame = src;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void RippleElement::setMode(RippleMode mode)
{
    if (this->d->m_mode == mode)
        return;

    this->d->m_mode = mode;
    emit this->modeChanged(mode);
}

void RippleElement::setAmplitude(int amplitude)
{
    if (this->d->m_amplitude == amplitude)
        return;

    this->d->m_amplitude = amplitude;
    emit this->amplitudeChanged(amplitude);
}

void RippleElement::setDecay(int decay)
{
    if (this->d->m_decay == decay)
        return;

    this->d->m_decay = decay;
    emit this->decayChanged(decay);
}

void RippleElement::setThreshold(int threshold)
{
    if (this->d->m_threshold == threshold)
        return;

    this->d->m_threshold = threshold;
    emit this->thresholdChanged(threshold);
}

void RippleElement::setLumaThreshold(int lumaThreshold)
{
    if (this->d->m_lumaThreshold == lumaThreshold)
        return;

    this->d->m_lumaThreshold = lumaThreshold;
    emit this->lumaThresholdChanged(lumaThreshold);
}

void RippleElement::setMinDropSize(int minDropSize)
{
    if (this->d->m_minDropSize == minDropSize)
        return;

    this->d->m_minDropSize = minDropSize;
    emit this->minDropSizeChanged(minDropSize);
}

void RippleElement::setMaxDropSize(int maxDropSize)
{
    if (this->d->m_maxDropSize == maxDropSize)
        return;

    this->d->m_maxDropSize = maxDropSize;
    emit this->maxDropSizeChanged(maxDropSize);
}

void RippleElement::setDropSigma(qreal dropSigma)
{
    if (qFuzzyCompare(this->d->m_dropSigma, dropSigma))
        return;

    this->d->m_dropSigma = dropSigma;
    emit this->dropSigmaChanged(dropSigma);
}

void RippleElement::setDropProbability(qreal dropProbability)
{
    if (qFuzzyCompare(this->d->m_dropProbability, dropProbability))
        return;

    this->d->m_dropProbability = dropProbability;
    emit this->dropProbabilityChanged(dropProbability);
}

void RippleElement::resetMode()
{
    this->setMode(RippleModeMotionDetect);
}

void RippleElement::resetAmplitude()
{
    this->setAmplitude(256);
}

void RippleElement::resetDecay()
{
    this->setDecay(8);
}

void RippleElement::resetThreshold()
{
    this->setThreshold(15);
}

void RippleElement::resetLumaThreshold()
{
    this->setLumaThreshold(15);
}

void RippleElement::resetMinDropSize()
{
    this->setMinDropSize(3);
}

void RippleElement::resetMaxDropSize()
{
    this->setMaxDropSize(127);
}

void RippleElement::resetDropSigma()
{
    this->setDropSigma(1.0);
}

void RippleElement::resetDropProbability()
{
    this->setDropProbability(1.0);
}

QDataStream &operator >>(QDataStream &istream, RippleElement::RippleMode &mode)
{
    int modeInt;
    istream >> modeInt;
    mode = static_cast<RippleElement::RippleMode>(modeInt);

    return istream;
}

QDataStream &operator <<(QDataStream &ostream, RippleElement::RippleMode mode)
{
    ostream << static_cast<int>(mode);

    return ostream;
}

AkVideoPacket RippleElementPrivate::imageDiff(const AkVideoPacket &img1,
                                              const AkVideoPacket &img2,
                                              int threshold,
                                              int lumaThreshold,
                                              int strength)
{
    int width = qMin(img1.caps().width(), img2.caps().width());
    int height = qMin(img1.caps().height(), img2.caps().height());
    auto ocaps = img1.caps();
    ocaps.setFormat(AkVideoCaps::Format_gray32);
    AkVideoPacket diff(ocaps);
    diff.copyMetadata(img1);

    for (int y = 0; y < height; y++) {
        auto img1Line = reinterpret_cast<const QRgb *>(img1.constLine(0, y));
        auto img2Line = reinterpret_cast<const QRgb *>(img2.constLine(0, y));
        auto diffLine = reinterpret_cast<qint32 *>(diff.line(0, y));

        for (int x = 0; x < width; x++) {
            auto &pixel1 = img1Line[x];
            int r1 = qRed(pixel1);
            int g1 = qGreen(pixel1);
            int b1 = qBlue(pixel1);

            auto &pixel2 = img2Line[x];
            int r2 = qRed(pixel2);
            int g2 = qGreen(pixel2);
            int b2 = qBlue(pixel2);

            int dr = r1 - r2;
            int dg = g1 - g2;
            int db = b1 - b2;

            int s = dr * dr + dg * dg + db * db;
            s = qRound(qSqrt(s / 3.0));
            s = s < threshold? 0: s;

            int gray = qGray(pixel2);
            s = gray < lumaThreshold? 0: s;

            diffLine[x] = (strength * s) >> 8;
        }
    }

    return diff;
}

void RippleElementPrivate::addDrop(AkVideoPacket &buffer,
                                   const AkVideoPacket &drop) const
{
    if (!buffer || !drop)
        return;

    for (int y = 0; y < buffer.caps().height(); y++) {
        auto dropsLine = reinterpret_cast<const qint32 *>(drop.constLine(0, y));
        auto bufferLine = reinterpret_cast<qint32 *>(buffer.line(0, y));

        for (int x = 0; x < buffer.caps().width(); x++)
            bufferLine[x] += dropsLine[x];
    }
}

void RippleElementPrivate::ripple(const AkVideoPacket &topBuffer,
                                  AkVideoPacket &bottomBuffer,
                                  int decay) const
{
    AkVideoPacket combinedBuffer(topBuffer.caps(), true);

    int widthM1 = topBuffer.caps().width() - 1;
    int heightM1 = topBuffer.caps().height() - 1;

    auto lineTSize = topBuffer.lineSize(0);
    auto lineBSize = bottomBuffer.lineSize(0);
    auto lineCSize = combinedBuffer.lineSize(0);

    memset(bottomBuffer.data(), 0, lineBSize);
    memset(bottomBuffer.line(0, heightM1), 0, lineBSize);
    auto lineBottom = reinterpret_cast<qint32 *>(bottomBuffer.line(0, 1));

    for (int y = 1; y < heightM1; y++) {
        lineBottom[0] = lineBottom[widthM1] = 0;
        lineBottom = reinterpret_cast<qint32 *>(reinterpret_cast<qint8 *>(lineBottom) + lineBSize);
    }

    // Wave simulation.

    static const int wavesKernel[9] {
        1,  1, 1,
        1, -9, 1,
        1,  1, 1,
    };
    const int wavesDivl2 = 3;

    auto lineTop = reinterpret_cast<const qint32 *>(topBuffer.constLine(0, 1));
    auto lineTopM1 = reinterpret_cast<const qint32 *>(topBuffer.constLine(0, 0));
    auto lineBottom2 = reinterpret_cast<const qint32 *>(bottomBuffer.constLine(0, 1));
    auto lineCombined = reinterpret_cast<qint32 *>(combinedBuffer.line(0, 1));

    for (int y = 1; y < heightM1; y++) {
        for (int x = 1; x < widthM1; x++) {
            auto lineT = lineTopM1;
            auto lineKernel = wavesKernel;
            qint64 h = 0;

            for (int j = 0; j < 3; j++) {
                for (int i = 0; i < 3; i++)
                    h += qint64(lineKernel[i]) * qint64(lineT[x + i - 1]);

                lineT = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineT) + lineTSize);
                lineKernel += 3;
            }

            h >>= wavesDivl2;
            qint32 v = lineTop[x] - lineBottom2[x];
            v += qint32(h) - (v >> decay);
            lineCombined[x] = v + lineTop[x];
        }

        lineTop = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineTop) + lineTSize);
        lineTopM1 = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineTopM1) + lineTSize);
        lineBottom2 = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineBottom2) + lineBSize);
        lineCombined = reinterpret_cast<qint32 *>(reinterpret_cast<qint8 *>(lineCombined) + lineCSize);
    }

    // Low pass filter.

    static const int denoiseKernel[9] {
        0,  1, 0,
        1, 60, 1,
        0,  1, 0,
    };
    const int denoiseDivl2 = 6;

    auto lineCombined2 = reinterpret_cast<const qint32 *>(combinedBuffer.constLine(0, 1));
    auto lineCombined2M1 = reinterpret_cast<const qint32 *>(combinedBuffer.constLine(0, 0));
    auto lineBottom3 = reinterpret_cast<qint32 *>(bottomBuffer.line(0, 1));

    for (int y = 1; y < heightM1; y++) {
        for (int x = 1; x < widthM1; x++) {
            auto lineC = lineCombined2M1;
            auto lineKernel = denoiseKernel;
            qint64 h = 0;

            for (int j = 0; j < 3; j++) {
                for (int i = 0; i < 3; i++)
                    h += qint64(lineKernel[i]) * qint64(lineC[x + i - 1]);

                lineC = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineC) + lineCSize);
                lineKernel += 3;
            }

            lineBottom3[x] = qint32(h) >> denoiseDivl2;
        }

        lineCombined2 = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineCombined2) + lineCSize);
        lineCombined2M1 = reinterpret_cast<const qint32 *>(reinterpret_cast<const qint8 *>(lineCombined2M1) + lineCSize);
        lineBottom3 = reinterpret_cast<qint32 *>(reinterpret_cast<qint8 *>(lineBottom3) + lineBSize);
    }
}

AkVideoPacket RippleElementPrivate::applyWater(const AkVideoPacket &src,
                                               const AkVideoPacket &water) const
{
    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    for (int y = 0; y < src.caps().height(); y++) {
        int ym1 = qMax(y - 1, 0);
        int yp1 = qMin(y + 1, src.caps().height() - 1);
        auto bufLine = reinterpret_cast<const qint32 *>(water.constLine(0, y));
        auto bufLineM1 = reinterpret_cast<const qint32 *>(water.constLine(0, ym1));
        auto bufLineP1 = reinterpret_cast<const qint32 *>(water.constLine(0, yp1));
        auto dstLine = reinterpret_cast<QRgb *>(dst.line(0, y));

        for (int x = 0; x < src.caps().width(); x++) {
            int xm1 = qMax(x - 1, 0);
            int xp1 = qMin(x + 1, src.caps().width() - 1);
            int xOff = bufLine[xm1] - bufLine[xp1];
            int yOff = bufLineM1[x] - bufLineP1[x];

            // Shading
            int xq = qBound(0, x + xOff, src.caps().width() - 1);
            int yq = qBound(0, y + yOff, src.caps().height() - 1);
            auto srcPixel = src.pixel<QRgb>(0, xq, yq);
            dstLine[x] = qRgba(qBound(0, qRed(srcPixel) + xOff, 255),
                               qBound(0, qGreen(srcPixel) + xOff, 255),
                               qBound(0, qBlue(srcPixel) + xOff, 255),
                               qAlpha(srcPixel));
        }
    }

    return dst;
}

AkVideoPacket RippleElementPrivate::drop(int bufferWidth,
                                         int bufferHeight,
                                         int dropWidth,
                                         int dropHeight,
                                         int amplitude,
                                         qreal sigma) const
{
    AkVideoPacket drop({AkVideoCaps::Format_gray32,
                        bufferWidth,
                        bufferHeight, {}},
                        true);

    if (qFuzzyCompare(sigma, 0.0))
        return drop;

    int x = QRandomGenerator::global()->bounded(0, bufferWidth);
    int y = QRandomGenerator::global()->bounded(0, bufferHeight);

    int minX = -dropWidth / 2;
    int maxX = 1 + dropWidth / 2;
    int minY = -dropHeight / 2;
    int maxY = 1 + dropHeight / 2;

    qreal tsigma2 = -2.0 * sigma * sigma;
    qreal emin = qExp(qreal(minX * minX + minY * minY) / tsigma2);
    qreal oemin = 1.0 - emin;

    for (int j = minY; j < maxY; j++) {
        int yj = y + j;

        if (yj >= 0 && yj < bufferHeight) {
            auto line = reinterpret_cast<qint32 *>(drop.line(0, yj));
            int jj = j * j;

            for (int i = minX; i < maxX; i++) {
                int xi = x + i;

                if (xi >= 0 && xi < bufferWidth) {
                    auto e = qExp(qreal(i * i + jj) / tsigma2);
                    line[xi] = qRound(amplitude * (e - emin) / oemin);
                }
            }
        }
    }

    return drop;
}

#include "moc_rippleelement.cpp"
