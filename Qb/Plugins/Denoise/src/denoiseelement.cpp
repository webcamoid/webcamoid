/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include "denoiseelement.h"

DenoiseElement::DenoiseElement(): QbElement()
{
    this->m_convert = QbElement::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    this->m_radius = 1;
    this->m_factor = 1024;
    this->m_mu = 0;
    this->m_sigma = 0;

    this->makeTable(this->m_factor);
}

QObject *DenoiseElement::controlInterface(QQmlEngine *engine, const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine, QUrl(QStringLiteral("qrc:/Denoise/share/qml/main.qml")));

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Denoise", (QObject *) this);
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);
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

int DenoiseElement::sigma() const
{
    return this->m_sigma;
}

void DenoiseElement::setRadius(int radius)
{
    if (radius != this->m_radius) {
        this->m_radius = radius;
        emit this->radiusChanged();
    }
}

void DenoiseElement::setFactor(int factor)
{
    if (factor != this->m_factor) {
        this->m_factor = factor;
        emit this->factorChanged();
    }
}

void DenoiseElement::setMu(int mu)
{
    if (mu != this->m_mu) {
        this->m_mu = mu;
        emit this->muChanged();
    }
}

void DenoiseElement::setSigma(int sigma)
{
    if (sigma != this->m_sigma) {
        this->m_sigma = sigma;
        emit this->sigmaChanged();
    }
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
    this->setSigma(0);
}

QbPacket DenoiseElement::iStream(const QbPacket &packet)
{
    int radius = this->m_radius;

    if (radius < 1)
        qbSend(packet)

    QbPacket iPacket = this->m_convert->iStream(packet);
    QImage src = QbUtils::packetToImage(iPacket);

    if (src.isNull())
        return QbPacket();

    static int factor = 1024;

    if (this->m_factor != factor) {
        this->makeTable(factor);
        factor = this->m_factor;
    }

    QImage oFrame(src.size(), src.format());

    const QRgb *srcBits = (const QRgb *) src.constBits();
    QRgb *destBits = (QRgb *) oFrame.bits();

    int width = src.width();
    int height = src.height();

    // Calculate integral image and cuadratic integral image.
    int videoArea = width * height;

    quint8 planeR[videoArea];
    quint8 planeG[videoArea];
    quint8 planeB[videoArea];

    quint32 integralR[videoArea];
    quint32 integralG[videoArea];
    quint32 integralB[videoArea];

    quint64 *integral2R = new quint64[videoArea];
    quint64 *integral2G = new quint64[videoArea];
    quint64 *integral2B = new quint64[videoArea];

    quint32 sumR = 0;
    quint32 sumG = 0;
    quint32 sumB = 0;

    quint64 sum2R = 0;
    quint64 sum2G = 0;
    quint64 sum2B = 0;

    for (int i = 0; i < width; i++) {
        planeR[i] = qRed(srcBits[i]);
        planeG[i] = qGreen(srcBits[i]);
        planeB[i] = qBlue(srcBits[i]);

        sumR += planeR[i];
        sumG += planeG[i];
        sumB += planeB[i];

        sum2R += planeR[i] * planeR[i];
        sum2G += planeG[i] * planeG[i];
        sum2B += planeB[i] * planeB[i];

        integralR[i] = sumR;
        integralG[i] = sumG;
        integralB[i] = sumB;

        integral2R[i] = sum2R;
        integral2G[i] = sum2G;
        integral2B[i] = sum2B;
    }

    quint32 posPrev = 0;
    quint32 pos = width;

    for (int j = 1; j < height; j++) {
        sumR = 0;
        sumG = 0;
        sumB = 0;

        sum2R = 0;
        sum2G = 0;
        sum2B = 0;

        for (int i = 0; i < width; i++, posPrev++, pos++) {
            planeR[pos] = qRed(srcBits[pos]);
            planeG[pos] = qGreen(srcBits[pos]);
            planeB[pos] = qBlue(srcBits[pos]);

            sumR += planeR[pos];
            sumG += planeG[pos];
            sumB += planeB[pos];

            sum2R += planeR[pos] * planeR[pos];
            sum2G += planeG[pos] * planeG[pos];
            sum2B += planeB[pos] * planeB[pos];

            integralR[pos] = sumR + integralR[posPrev];
            integralG[pos] = sumG + integralG[posPrev];
            integralB[pos] = sumB + integralB[posPrev];

            integral2R[pos] = sum2R + integral2R[posPrev];
            integral2G[pos] = sum2G + integral2G[posPrev];
            integral2B[pos] = sum2B + integral2B[posPrev];
        }
    }

    // Image convolution.
    int mu = this->m_mu;
    int sigma = this->m_sigma;

    for (int y = 0, pixel = 0; y < height; y++) {
        int yMin = y - radius;
        int yMax = y + radius;

        if (yMin < 0)
            yMin = 0;

        if (yMax >= height)
            yMax = height - 1;

        for (int x = 0; x < width; x++, pixel++) {
            int xMin = x - radius;
            int xMax = x + radius;

            if (xMin < 0)
                xMin = 0;

            if (xMax >= width)
                xMax = width - 1;

            int kernelSize = (xMax - xMin + 1) * (yMax - yMin + 1);

            // Calculate integral and cuadratic integral.
            int br = xMax + yMax * width;
            quint32 sr1 = integralR[br];
            quint32 sg1 = integralG[br];
            quint32 sb1 = integralB[br];

            quint32 sr2 = integral2R[br];
            quint32 sg2 = integral2G[br];
            quint32 sb2 = integral2B[br];

            int xm = xMin - 1;
            int ym = yMin - 1;

            if (xm >= 0 && ym >= 0) {
                int tl = xm + ym * width;

                sr1 += integralR[tl];
                sg1 += integralG[tl];
                sb1 += integralB[tl];

                sr2 += integral2R[tl];
                sg2 += integral2G[tl];
                sb2 += integral2B[tl];
            }

            if (xm >= 0) {
                int bl = xm + yMax * width;

                sr1 -= integralR[bl];
                sg1 -= integralG[bl];
                sb1 -= integralB[bl];

                sr2 -= integral2R[bl];
                sg2 -= integral2G[bl];
                sb2 -= integral2B[bl];
            }

            if (ym >= 0) {
                int tr = xMax + ym * width;

                sr1 -= integralR[tr];
                sg1 -= integralG[tr];
                sb1 -= integralB[tr];

                sr2 -= integral2R[tr];
                sg2 -= integral2G[tr];
                sb2 -= integral2B[tr];
            }

            // Calculate median.
            int mr = sr1 / kernelSize;
            int mg = sg1 / kernelSize;
            int mb = sb1 / kernelSize;

            // Calculate standard deviation.
            quint32 srq = sr2 - (sr1 * sr1) / kernelSize;
            quint32 sgq = sg2 - (sg1 * sg1) / kernelSize;
            quint32 sbq = sb2 - (sb1 * sb1) / kernelSize;

            // Apply factors.
            int ks = kernelSize - 1;

            mr = qBound(0, mr + mu, 255);
            mg = qBound(0, mg + mu, 255);
            mb = qBound(0, mb + mu, 255);

            int sr = sqrt(srq / ks);
            int sg = sqrt(sgq / ks);
            int sb = sqrt(sbq / ks);

            sr = qBound(0, sr + sigma, 127);
            sg = qBound(0, sg + sigma, 127);
            sb = qBound(0, sb + sigma, 127);

            // Calculate weighted average.
            int r = 0;
            int g = 0;
            int b = 0;

            int twr = 0;
            int twg = 0;
            int twb = 0;

            int pos = xMin + yMin * width;
            int diffPos = width - xMax + xMin - 1;

            for (int j = yMin, k = 0; j <= yMax; j++, pos += diffPos)
                for (int i = xMin; i <= xMax; i++, pos++, k++) {
                    int rr = planeR[pos];
                    int wr = this->m_weight[mr][sr][rr];
                    r += wr * rr;
                    twr += wr;

                    int gg = planeG[pos];
                    int wg = this->m_weight[mg][sg][gg];
                    g += wg * gg;
                    twg += wg;

                    int bb = planeB[pos];
                    int wb = this->m_weight[mb][sb][bb];
                    b += wb * bb;
                    twb += wb;
                }

            if (twr < 1)
                r = planeR[pixel];
            else
                r /= twr;

            if (twg < 1)
                g = planeG[pixel];
            else
                g /= twg;

            if (twb < 1)
                b = planeB[pixel];
            else
                b /= twb;

            int a = qAlpha(srcBits[pixel]);
            destBits[pixel] = qRgba(r, g, b, a);
        }
    }

    delete [] integral2R;
    delete [] integral2G;
    delete [] integral2B;

    QbPacket oPacket = QbUtils::imageToPacket(oFrame, iPacket);
    qbSend(oPacket)
}
