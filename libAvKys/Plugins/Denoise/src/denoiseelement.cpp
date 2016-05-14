/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#include <QtConcurrent>

#include "denoiseelement.h"

DenoiseElement::DenoiseElement(): AkElement()
{
    this->m_radius = 1;
    this->m_factor = 1024;
    this->m_mu = 0;
    this->m_sigma = 1.0;

    this->m_weight = new int[1 << 24];
    this->makeTable(this->m_factor);
}

DenoiseElement::~DenoiseElement()
{
    delete [] this->m_weight;
}

QObject *DenoiseElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Denoise/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Denoise", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

int DenoiseElement::radius() const
{
    return this->m_radius;
}

int DenoiseElement::factor() const
{
    return this->m_factor;
}

int DenoiseElement::mu() const
{
    return this->m_mu;
}

qreal DenoiseElement::sigma() const
{
    return this->m_sigma;
}

void DenoiseElement::integralImage(const QImage &image,
                                   int oWidth, int oHeight,
                                   PixelU8 *planes,
                                   PixelU32 *integral,
                                   PixelU64 *integral2)
{
    for (int y = 1; y < oHeight; y++) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y - 1));
        PixelU8 *planesLine = planes
                              + (y - 1) * image.width();

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

void DenoiseElement::denoise(const DenoiseStaticParams &staticParams,
                             const DenoiseParams *params)
{
    PixelU32 sum = integralSum(staticParams.integral, staticParams.oWidth,
                               params->xp, params->yp, params->kw, params->kh);
    PixelU64 sum2 = integralSum(staticParams.integral2, staticParams.oWidth,
                                params->xp, params->yp, params->kw, params->kh);
    quint32 ks = quint32(params->kw * params->kh);

    PixelU32 mean = sum / ks;
    PixelU32 dev = sqrt(ks * sum2 - pow2(sum)) / ks;

    mean = bound(0u, mean + staticParams.mu, 255u);
    dev = bound(0., mult(staticParams.sigma, dev), 127.);

    PixelU32 mdMask = (mean << 16) | (dev << 8);

    PixelI32 pixel;
    PixelI32 sumW;

    for (int j = 0; j < params->kh; j++) {
        const PixelU8 *line = staticParams.planes
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

void DenoiseElement::setRadius(int radius)
{
    if (this->m_radius == radius)
        return;

    this->m_radius = radius;
    emit this->radiusChanged(radius);
}

void DenoiseElement::setFactor(int factor)
{
    if (this->m_factor == factor)
        return;

    this->m_factor = factor;
    emit this->factorChanged(factor);
}

void DenoiseElement::setMu(int mu)
{
    if (this->m_mu == mu)
        return;

    this->m_mu = mu;
    emit this->muChanged(mu);
}

void DenoiseElement::setSigma(qreal sigma)
{
    if (qFuzzyCompare(this->m_sigma, sigma))
        return;

    this->m_sigma = sigma;
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

AkPacket DenoiseElement::iStream(const AkPacket &packet)
{
    int radius = this->m_radius;

    if (radius < 1)
        akSend(packet)

    QImage src = AkUtils::packetToImage(packet);

    if (src.isNull())
        return AkPacket();

    src = src.convertToFormat(QImage::Format_ARGB32);

    static int factor = 1024;

    if (this->m_factor != factor) {
        this->makeTable(factor);
        factor = this->m_factor;
    }

    QImage oFrame(src.size(), src.format());

    int oWidth = src.width() + 1;
    int oHeight = src.height() + 1;
    PixelU8 *planes = new PixelU8[oWidth * oHeight];
    PixelU32 *integral = new PixelU32[oWidth * oHeight];
    PixelU64 *integral2 = new PixelU64[oWidth * oHeight];
    this->integralImage(src,
                        oWidth, oHeight,
                        planes, integral, integral2);

    DenoiseStaticParams staticParams;
    staticParams.planes = planes;
    staticParams.integral = integral;
    staticParams.integral2 = integral2;
    staticParams.width = src.width();
    staticParams.oWidth = oWidth;
    staticParams.weights = this->m_weight;
    staticParams.mu = this->m_mu;
    staticParams.sigma = this->m_sigma < 0.1? 0.1: this->m_sigma;

    QThreadPool threadPool;

    for (int y = 0, pos = 0; y < src.height(); y++) {
        const QRgb *iLine = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        QRgb *oLine = reinterpret_cast<QRgb *>(oFrame.scanLine(y));
        int yp = qMax(y - radius, 0);
        int kh = qMin(y + radius, src.height() - 1) - yp + 1;

        for (int x = 0; x < src.width(); x++, pos++) {
            int xp = qMax(x - radius, 0);
            int kw = qMin(x + radius, src.width() - 1) - xp + 1;

            DenoiseParams *params = new DenoiseParams();
            params->xp = xp;
            params->yp = yp;
            params->kw = kw;
            params->kh = kh;
            params->iPixel = planes[pos];
            params->oPixel = oLine + x;
            params->alpha = qAlpha(iLine[x]);

            if (radius >= 20)
                QtConcurrent::run(&threadPool, DenoiseElement::denoise, staticParams, params);
            else
                this->denoise(staticParams, params);
        }
    }

    threadPool.waitForDone();

    delete [] planes;
    delete [] integral;
    delete [] integral2;

    AkPacket oPacket = AkUtils::imageToPacket(oFrame, packet);
    akSend(oPacket)
}
