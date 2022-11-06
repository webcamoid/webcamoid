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

#include <QQmlContext>
#include <QtConcurrent>
#include <QtMath>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideoconverter.h>
#include <akvideopacket.h>

#include "denoiseelement.h"
#include "params.h"

class DenoiseElementPrivate
{
    public:
        int m_radius {1};
        int m_factor {1024};
        int m_mu {0};
        qreal m_sigma {1.0};
        int *m_weight {nullptr};
        AkVideoConverter m_videoConverter {{AkVideoCaps::Format_argbpack, 0, 0, {}}};

        void makeTable(int factor);
        void integralImage(const AkVideoPacket &packet,
                           int oWidth, int oHeight,
                           PixelU8 *planes,
                           PixelU32 *integral,
                           PixelU64 *integral2);
        static void denoise(const DenoiseStaticParams &staticParams,
                            const DenoiseParams *params);
};

DenoiseElement::DenoiseElement(): AkElement()
{
    this->d = new DenoiseElementPrivate;

    this->d->m_weight = new int[1 << 24];
    this->d->makeTable(this->d->m_factor);
}

DenoiseElement::~DenoiseElement()
{
    delete [] this->d->m_weight;
    delete this->d;
}

int DenoiseElement::radius() const
{
    return this->d->m_radius;
}

int DenoiseElement::factor() const
{
    return this->d->m_factor;
}

int DenoiseElement::mu() const
{
    return this->d->m_mu;
}

qreal DenoiseElement::sigma() const
{
    return this->d->m_sigma;
}

void DenoiseElementPrivate::integralImage(const AkVideoPacket &packet,
                                          int oWidth, int oHeight,
                                          PixelU8 *planes,
                                          PixelU32 *integral,
                                          PixelU64 *integral2)
{
    for (int y = 1; y < oHeight; y++) {
        auto line = reinterpret_cast<const QRgb *>(packet.constLine(0, y - 1));
        PixelU8 *planesLine = planes + (y - 1) * packet.caps().width();

        // Reset current line summation.
        PixelU32 sum;
        PixelU64 sum2;

        for (int x = 1; x < oWidth; x++) {
            QRgb pixel = line[x - 1];

            // Accumulate pixels in current line.
            sum += pixel;
            sum2 += pow2(pixel);

            // Offset to the current line.
            int offset = x + y * oWidth;

            // Offset to the previous line.
            // equivalent to x + (y - 1) * oWidth;
            int offsetPrevious = offset - oWidth;

            planesLine[x - 1] = pixel;

            // Accumulate current line and previous line.
            integral[offset] = sum + integral[offsetPrevious];
            integral2[offset] = sum2 + integral2[offsetPrevious];
        }
    }
}

void DenoiseElementPrivate::denoise(const DenoiseStaticParams &staticParams,
                                    const DenoiseParams *params)
{
    PixelU32 sum = integralSum(staticParams.integral, staticParams.oWidth,
                               params->xp, params->yp, params->kw, params->kh);
    PixelU64 sum2 = integralSum(staticParams.integral2, staticParams.oWidth,
                                params->xp, params->yp, params->kw, params->kh);
    auto ks = quint32(params->kw * params->kh);

    PixelU32 mean = sum / ks;
    PixelU32 dev = sqrt(ks * sum2 - pow2(sum)) / ks;

    mean = bound(0u, mean + staticParams.mu, 255u);
    dev = bound(0., mult(staticParams.sigma, dev), 127.);

    PixelU32 mdMask = (mean << 16) | (dev << 8);

    PixelI32 pixel;
    PixelI32 sumW;

    for (int j = 0; j < params->kh; j++) {
        auto line = staticParams.planes
                    + (params->yp + j) * staticParams.width;

        for (int i = 0; i < params->kw; i++) {
            PixelU8 pix = line[params->xp + i];
            PixelU32 mask = mdMask | pix;
            PixelI32 weight(staticParams.weights[mask.r],
                            staticParams.weights[mask.g],
                            staticParams.weights[mask.b]);
            pixel += weight * pix;
            sumW += weight;
        }
    }

    if (sumW.r < 1)
        pixel.r = params->iPixel.r;
    else
        pixel.r /= sumW.r;

    if (sumW.g < 1)
        pixel.g = params->iPixel.g;
    else
        pixel.g /= sumW.g;

    if (sumW.b < 1)
        pixel.b = params->iPixel.b;
    else
        pixel.b /= sumW.b;

    *params->oPixel = qRgba(pixel.r, pixel.g, pixel.b, params->alpha);
    delete params;
}

void DenoiseElementPrivate::makeTable(int factor)
{
    for (int s = 0; s < 128; s++) {
        int h = -2 * s * s;

        for (int m = 0; m < 256; m++)
            for (int c = 0; c < 256; c++) {
                if (s == 0) {
                    this->m_weight[(m << 16) | (s << 8) | c] = 0;

                    continue;
                }

                int d = c - m;
                d *= d;

                this->m_weight[(m << 16) | (s << 8) | c] = qRound(factor * exp(qreal(d) / h));
            }
    }
}

QString DenoiseElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/Denoise/share/qml/main.qml");
}

void DenoiseElement::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("Denoise", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

AkPacket DenoiseElement::iVideoStream(const AkVideoPacket &packet)
{
    int radius = this->d->m_radius;

    if (radius < 1) {
        if (packet)
            emit this->oStream(packet);

        return packet;
    }

    static int factor = 1024;

    if (this->d->m_factor != factor) {
        this->d->makeTable(factor);
        factor = this->d->m_factor;
    }

    this->d->m_videoConverter.begin();
    auto src = this->d->m_videoConverter.convert(packet);
    this->d->m_videoConverter.end();

    if (!src)
        return {};

    AkVideoPacket dst(src.caps());
    dst.copyMetadata(src);

    int oWidth = src.caps().width() + 1;
    int oHeight = src.caps().height() + 1;
    auto planes = new PixelU8[oWidth * oHeight];
    auto integral = new PixelU32[oWidth * oHeight];
    auto integral2 = new PixelU64[oWidth * oHeight];
    this->d->integralImage(src,
                           oWidth, oHeight,
                           planes, integral, integral2);

    DenoiseStaticParams staticParams {};
    staticParams.planes = planes;
    staticParams.integral = integral;
    staticParams.integral2 = integral2;
    staticParams.width = src.caps().width();
    staticParams.oWidth = oWidth;
    staticParams.weights = this->d->m_weight;
    staticParams.mu = this->d->m_mu;
    staticParams.sigma = this->d->m_sigma < 0.1? 0.1: this->d->m_sigma;

    QThreadPool threadPool;

    if (threadPool.maxThreadCount() < 8)
        threadPool.setMaxThreadCount(8);

    for (int y = 0, pos = 0; y < src.caps().height(); y++) {
        auto iLine = reinterpret_cast<const QRgb *>(src.constLine(0, y));
        auto oLine = reinterpret_cast<QRgb *>(dst.line(0, y));
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, src.caps().height() - 1) - yp + 1;

        for (int x = 0; x < src.caps().width(); x++, pos++) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, src.caps().width() - 1) - xp + 1;

            auto params = new DenoiseParams();
            params->xp = xp;
            params->yp = yp;
            params->kw = kw;
            params->kh = kh;
            params->iPixel = planes[pos];
            params->oPixel = oLine + x;
            params->alpha = qAlpha(iLine[x]);

            if (radius >= 20) {
                auto result =
                        QtConcurrent::run(&threadPool,
                                          DenoiseElementPrivate::denoise,
                                          staticParams,
                                          params);
                Q_UNUSED(result)
            } else {
                this->d->denoise(staticParams, params);
            }
        }
    }

    threadPool.waitForDone();

    delete [] planes;
    delete [] integral;
    delete [] integral2;

    if (dst)
        emit this->oStream(dst);

    return dst;
}

void DenoiseElement::setRadius(int radius)
{
    if (this->d->m_radius == radius)
        return;

    this->d->m_radius = radius;
    emit this->radiusChanged(radius);
}

void DenoiseElement::setFactor(int factor)
{
    if (this->d->m_factor == factor)
        return;

    this->d->m_factor = factor;
    emit this->factorChanged(factor);
}

void DenoiseElement::setMu(int mu)
{
    if (this->d->m_mu == mu)
        return;

    this->d->m_mu = mu;
    emit this->muChanged(mu);
}

void DenoiseElement::setSigma(qreal sigma)
{
    if (qFuzzyCompare(this->d->m_sigma, sigma))
        return;

    this->d->m_sigma = sigma;
    emit this->sigmaChanged(sigma);
}

void DenoiseElement::resetRadius()
{
    this->setRadius(1);
}

void DenoiseElement::resetFactor()
{
    this->setFactor(1024);
}

void DenoiseElement::resetMu()
{
    this->setMu(0);
}

void DenoiseElement::resetSigma()
{
    this->setSigma(1.0);
}

#include "moc_denoiseelement.cpp"
