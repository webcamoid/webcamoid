/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#include "blitzerelement.h"

BlitzerElement::BlitzerElement(): QbElement()
{
    this->m_qtToFormat[QImage::Format_Mono] = "monob";
    this->m_qtToFormat[QImage::Format_Indexed8] = "rgb8";
    this->m_qtToFormat[QImage::Format_RGB32] = "bgr0";
    this->m_qtToFormat[QImage::Format_ARGB32] = "bgra";
    this->m_qtToFormat[QImage::Format_RGB16] = "rgb565le";
    this->m_qtToFormat[QImage::Format_RGB555] = "rgb555le";
    this->m_qtToFormat[QImage::Format_RGB888] = "rgb24";
    this->m_qtToFormat[QImage::Format_RGB444] = "rgb444le";

    this->m_stringToRGBChannel["Grayscale"] = Blitz::Grayscale;
    this->m_stringToRGBChannel["Brightness"] = Blitz::Brightness;
    this->m_stringToRGBChannel["Red"] = Blitz::Red;
    this->m_stringToRGBChannel["Green"] = Blitz::Green;
    this->m_stringToRGBChannel["Blue"] = Blitz::Blue;
    this->m_stringToRGBChannel["Alpha"] = Blitz::Alpha;
    this->m_stringToRGBChannel["All"] = Blitz::All;

    this->m_stringToEffectQuality["Low"] = Blitz::Low;
    this->m_stringToEffectQuality["High"] = Blitz::High;

    this->m_stringToGradientType["VerticalGradient"] = Blitz::VerticalGradient;
    this->m_stringToGradientType["HorizontalGradient"] = Blitz::HorizontalGradient;
    this->m_stringToGradientType["DiagonalGradient"] = Blitz::DiagonalGradient;
    this->m_stringToGradientType["CrossDiagonalGradient"] = Blitz::CrossDiagonalGradient;
    this->m_stringToGradientType["PyramidGradient"] = Blitz::PyramidGradient;
    this->m_stringToGradientType["RectangleGradient"] = Blitz::RectangleGradient;
    this->m_stringToGradientType["PipeCrossGradient"] = Blitz::PipeCrossGradient;
    this->m_stringToGradientType["EllipticGradient"] = Blitz::EllipticGradient;

    this->m_stringToInvertMode["InvertRgb"] = QImage::InvertRgb;
    this->m_stringToInvertMode["InvertRgba"] = QImage::InvertRgba;

    this->m_stringToModulationType["Intensity"] = Blitz::Intensity;
    this->m_stringToModulationType["Saturation"] = Blitz::Saturation;
    this->m_stringToModulationType["HueShift"] = Blitz::HueShift;
    this->m_stringToModulationType["Contrast"] = Blitz::Contrast;

    this->m_stringToAspectRatioMode["IgnoreAspectRatio"] = Qt::IgnoreAspectRatio;
    this->m_stringToAspectRatioMode["KeepAspectRatio"] = Qt::KeepAspectRatio;
    this->m_stringToAspectRatioMode["KeepAspectRatioByExpanding"] = Qt::KeepAspectRatioByExpanding;

    this->m_stringToScaleFilterType["UndefinedFilter"] = Blitz::UndefinedFilter;
    this->m_stringToScaleFilterType["PointFilter"] = Blitz::PointFilter;
    this->m_stringToScaleFilterType["BoxFilter"] = Blitz::BoxFilter;
    this->m_stringToScaleFilterType["TriangleFilter"] = Blitz::TriangleFilter;
    this->m_stringToScaleFilterType["HermiteFilter"] = Blitz::HermiteFilter;
    this->m_stringToScaleFilterType["HanningFilter"] = Blitz::HanningFilter;
    this->m_stringToScaleFilterType["HammingFilter"] = Blitz::HammingFilter;
    this->m_stringToScaleFilterType["BlackmanFilter"] = Blitz::BlackmanFilter;
    this->m_stringToScaleFilterType["GaussianFilter"] = Blitz::GaussianFilter;
    this->m_stringToScaleFilterType["QuadraticFilter"] = Blitz::QuadraticFilter;
    this->m_stringToScaleFilterType["CubicFilter"] = Blitz::CubicFilter;
    this->m_stringToScaleFilterType["CatromFilter"] = Blitz::CatromFilter;
    this->m_stringToScaleFilterType["MitchellFilter"] = Blitz::MitchellFilter;
    this->m_stringToScaleFilterType["LanczosFilter"] = Blitz::LanczosFilter;
    this->m_stringToScaleFilterType["BesselFilter"] = Blitz::BesselFilter;
    this->m_stringToScaleFilterType["SincFilter"] = Blitz::SincFilter;

    this->m_convert = Qb::create("VCapsConvert");
    this->m_convert->setProperty("caps", "video/x-raw,format=bgra");

    QObject::connect(this->m_convert.data(),
                     SIGNAL(oStream(const QbPacket &)),
                     this,
                     SLOT(processFrame(const QbPacket &)));

    this->resetMethod();
    this->resetParams();
}

BlitzerElement::~BlitzerElement()
{
}

QString BlitzerElement::method() const
{
    return this->m_method;
}

QVariantList BlitzerElement::params() const
{
    return this->m_params;
}

void BlitzerElement::setMethod(QString method)
{
    this->m_method = method;
}

void BlitzerElement::setParams(QVariantList params)
{
    this->m_params = params;
}

void BlitzerElement::resetMethod()
{
    this->setMethod("");
}

void BlitzerElement::resetParams()
{
    this->setParams(QVariantList());
}

void BlitzerElement::iStream(const QbPacket &packet)
{
    if (!packet.caps().isValid() ||
        packet.caps().mimeType() != "video/x-raw" ||
        this->state() != ElementStatePlaying)
        return;

    this->m_convert->iStream(packet);
}

void BlitzerElement::setState(ElementState state)
{
    QbElement::setState(state);
    this->m_convert->setState(this->state());
}

void BlitzerElement::processFrame(const QbPacket &packet)
{
    int width = packet.caps().property("width").toInt();
    int height = packet.caps().property("height").toInt();

    QImage oFrame = QImage(packet.buffer().data(),
                           width,
                           height,
                           QImage::Format_RGB32);

    if (oFrame.isNull())
        return;

    if (this->method() == "antialias")
        oFrame = Blitz::antialias(oFrame);
    else if (this->method() == "blur")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::blur(oFrame);
        else
        {
            bool ok;
            int radius = this->params()[0].toInt(&ok);

            if (ok)
                oFrame = Blitz::blur(oFrame, radius);
        }
    }
    else if (this->method() == "channelIntensity")
    {
        if (this->params().length() > 1)
        {
            bool ok;
            float percent = this->params()[0].toFloat(&ok);

            if (ok)
            {
                QString channelString = this->params()[1].toString();

                Blitz::RGBChannel channel;

                if (this->m_stringToRGBChannel.contains(channelString))
                    channel = this->m_stringToRGBChannel[channelString];
                else
                    ok = false;

                if (ok)
                    oFrame = Blitz::channelIntensity(oFrame, percent, channel);
            }
        }
    }
    else if (this->method() == "charcoal")
        oFrame = Blitz::charcoal(oFrame);
    else if (this->method() == "contrast")
    {
        if (this->params().length() == 1)
        {
            bool sharpen = this->params()[0].toBool();

            oFrame = Blitz::contrast(oFrame, sharpen);
        }
        else if (this->params().length() > 1)
        {
            bool sharpen = this->params()[0].toBool();
            bool ok;
            int weight = this->params()[1].toInt(&ok);

            if (ok)
                oFrame = Blitz::contrast(oFrame, sharpen, weight);
        }
    }
    else if (this->method() == "convolve")
    {
        if (this->params().length() > 0)
        {
            QVariantList matrixList = this->params()[0].toList();

            if (matrixList.length() > 0)
            {
                QSharedPointer<float> matrix(new float[matrixList.length()]);
                bool ok = true;

                for (int i = 0; i < matrixList.length(); i++)
                {
                    matrix.data()[i] = matrixList[i].toFloat(&ok);

                    if (!ok)
                        break;
                }

                if (ok)
                    oFrame = Blitz::convolve(oFrame, matrixList.length(), matrix.data());
            }
        }
    }
    else if (this->method() == "convolveEdge")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::convolveEdge(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::convolveEdge(oFrame, radius);
        }
        else if (this->params().length() > 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                QString qualityString = this->params()[1].toString();

                Blitz::EffectQuality quality;

                if (this->m_stringToEffectQuality.contains(qualityString))
                    quality = this->m_stringToEffectQuality[qualityString];
                else
                    ok = false;

                if (ok)
                    oFrame = Blitz::convolveEdge(oFrame, radius, quality);
            }
        }
    }
    else if (this->method() == "convolveInteger")
    {
        if (this->params().length() == 1)
        {
            QVariantList matrixList = this->params()[0].toList();

            if (matrixList.length() > 0)
            {
                QSharedPointer <int> matrix(new int[matrixList.length()]);
                bool ok = true;

                for (int i = 0; i < matrixList.length(); i++)
                {
                    matrix.data()[i] = matrixList[i].toInt(&ok);

                    if (!ok)
                        break;
                }

                if (ok)
                    oFrame = Blitz::convolveInteger(oFrame, matrixList.length(), matrix.data());
            }
        }
        else if (this->params().length() > 1)
        {
            QVariantList matrixList = this->params()[0].toList();

            if (matrixList.length() > 0)
            {
                QSharedPointer<int> matrix(new int[matrixList.length()]);
                bool ok = true;

                for (int i = 0; i < matrixList.length(); i++)
                {
                    matrix.data()[i] = matrixList[i].toInt(&ok);

                    if (!ok)
                        break;
                }

                if (ok)
                {
                    int divisor = this->params()[0].toInt(&ok);

                    if (ok)
                        oFrame = Blitz::convolveInteger(oFrame, matrixList.length(), matrix.data(), divisor);
                }
            }
        }
    }
    else if (this->method() == "desaturate")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::desaturate(oFrame);
        else
        {
            bool ok;
            float desat = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::desaturate(oFrame, desat);
        }
    }
    else if (this->method() == "despeckle")
        oFrame = Blitz::despeckle(oFrame);
    else if (this->method() == "edge")
        oFrame = Blitz::edge(oFrame);
    else if (this->method() == "emboss")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::emboss(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::emboss(oFrame, radius);
        }
        else if (this->params().length() == 2)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float sigma = this->params()[1].toFloat(&ok);

                if (ok)
                    oFrame = Blitz::emboss(oFrame, radius, sigma);
            }
        }
        else if (this->params().length() > 2)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float sigma = this->params()[1].toFloat(&ok);

                if (ok)
                {
                    QString qualityString = this->params()[2].toString();

                    Blitz::EffectQuality quality;

                    if (this->m_stringToEffectQuality.contains(qualityString))
                        quality = this->m_stringToEffectQuality[qualityString];
                    else
                        ok = false;

                    if (ok)
                        oFrame = Blitz::emboss(oFrame, radius, sigma, quality);
                }
            }
        }
    }
    else if (this->method() == "equalize")
        Blitz::equalize(oFrame);
    else if (this->method() == "fade")
    {
        if (this->params().length() == 2)
        {
            bool ok;
            float val = this->params()[0].toFloat(&ok);

            QColor color = this->params()[1].value<QColor>();

            if (ok)
                oFrame = Blitz::fade(oFrame, val, color);
        }
    }
    else if (this->method() == "flatten")
    {
        if (this->params().length() == 2)
        {
            QColor ca = this->params()[0].value<QColor>();
            QColor cb = this->params()[1].value<QColor>();

            oFrame = Blitz::flatten(oFrame, ca, cb);
        }
    }
    else if (this->method() == "gaussianBlur")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::gaussianBlur(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::gaussianBlur(oFrame, radius);
        }
        else if (this->params().length() > 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float sigma = this->params()[1].toFloat(&ok);

                if (ok)
                    oFrame = Blitz::gaussianBlur(oFrame, radius, sigma);
            }
        }
    }
    else if (this->method() == "gaussianSharpen")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::gaussianSharpen(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::gaussianSharpen(oFrame, radius);
        }
        else if (this->params().length() == 2)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float sigma = this->params()[1].toFloat(&ok);

                if (ok)
                    oFrame = Blitz::gaussianSharpen(oFrame, radius, sigma);
            }
        }
        else if (this->params().length() > 2)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float sigma = this->params()[1].toFloat(&ok);

                if (ok)
                {
                    QString qualityString = this->params()[2].toString();

                    Blitz::EffectQuality quality;

                    if (this->m_stringToEffectQuality.contains(qualityString))
                        quality = this->m_stringToEffectQuality[qualityString];
                    else
                        ok = false;

                    if (ok)
                        oFrame = Blitz::gaussianSharpen(oFrame, radius, sigma, quality);
                }
            }
        }
    }
    else if (this->method() == "gradient")
    {
        if (this->params().length() == 4)
        {
            QSize size = this->params()[0].toSize();
            QColor ca = this->params()[1].value<QColor>();
            QColor cb = this->params()[2].value<QColor>();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
                oFrame = Blitz::gradient(size, ca, cb, type);
        }
    }
    else if (this->method() == "grayGradient")
    {
        if (this->params().length() == 4)
        {
            QSize size = this->params()[0].toSize();
            unsigned char ca = this->params()[1].toInt();
            unsigned char cb = this->params()[2].toInt();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
                oFrame = Blitz::grayGradient(size, ca, cb, type);
        }
    }
    else if (this->method() == "grayscale")
    {
        if (this->params().isEmpty())
            Blitz::grayscale(oFrame);
        else
        {
            bool reduceDepth = this->params()[0].toBool();

            Blitz::grayscale(oFrame, reduceDepth);
        }
    }
    else if (this->method() == "grayUnbalancedGradient")
    {
        if (this->params().length() == 4)
        {
            QSize size = this->params()[0].toSize();
            int ca = this->params()[1].toInt();
            int cb = this->params()[2].toInt();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
                oFrame = Blitz::grayUnbalancedGradient(size, ca, cb, type);
        }
        if (this->params().length() == 5)
        {
            QSize size = this->params()[0].toSize();
            int ca = this->params()[1].toInt();
            int cb = this->params()[2].toInt();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
            {
                int xfactor = this->params()[4].toInt(&ok);

                if (ok)
                    oFrame = Blitz::grayUnbalancedGradient(size, ca, cb, type, xfactor);
            }
        }
        if (this->params().length() > 5)
        {
            QSize size = this->params()[0].toSize();
            int ca = this->params()[1].toInt();
            int cb = this->params()[2].toInt();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
            {
                int xfactor = this->params()[4].toInt(&ok);

                if (ok)
                {
                    int yfactor = this->params()[5].toInt(&ok);

                    if (ok)
                        oFrame = Blitz::grayUnbalancedGradient(size, ca, cb, type, xfactor, yfactor);
                }
            }
        }
    }
    else if (this->method() == "implode")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::implode(oFrame);
        else
        {
            bool ok;
            float amount = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::implode(oFrame, amount);
        }
    }
    else if (this->method() == "intensity")
    {
        if (this->params().length() > 0)
        {
            bool ok;
            float percent = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::intensity(oFrame, percent);
        }
    }
    else if (this->method() == "invert")
    {
        if (this->params().isEmpty())
            Blitz::invert(oFrame);
        else
        {
            QString modeString = this->params()[0].toString();

            bool ok = true;
            QImage::InvertMode mode;

            if (this->m_stringToInvertMode.contains(modeString))
                mode = this->m_stringToInvertMode[modeString];
            else
                ok = false;

            if (ok)
                Blitz::invert(oFrame, mode);
        }
    }
    else if (this->method() == "modulate")
    {
        if (this->params().length() == 5)
        {
            QImage modImg(this->params()[0].toString());
            bool reverse = this->params()[1].toBool();

            QString typeString = this->params()[2].toString();

            bool ok = true;
            Blitz::ModulationType type;

            if (this->m_stringToModulationType.contains(typeString))
                type = this->m_stringToModulationType[typeString];
            else
                ok = false;

            if (ok)
            {
                int factor = this->params()[3].toInt(&ok);

                if (ok)
                {
                    QString channelString = this->params()[4].toString();

                    bool ok = true;
                    Blitz::RGBChannel channel;

                    if (this->m_stringToRGBChannel.contains(channelString))
                        channel = this->m_stringToRGBChannel[channelString];
                    else
                        ok = false;

                    if (ok)
                        oFrame = Blitz::modulate(oFrame, modImg, reverse, type, factor, channel);
                }
            }
        }
    }
    else if (this->method() == "normalize")
        Blitz::normalize(oFrame);
    else if (this->method() == "oilPaint")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::oilPaint(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::oilPaint(oFrame, radius);
        }
        else if (this->params().length() > 1)
        {
            bool ok;
            float radius = this->params()[0].toFloat(&ok);

            if (ok)
            {
                QString qualityString = this->params()[1].toString();

                Blitz::EffectQuality quality;

                if (this->m_stringToEffectQuality.contains(qualityString))
                    quality = this->m_stringToEffectQuality[qualityString];
                else
                    ok = false;

                if (ok)
                    oFrame = Blitz::oilPaint(oFrame, radius, quality);
            }
        }
    }
    else if (this->method() == "sharpen")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::sharpen(oFrame);
        else
        {
            bool ok;
            int radius = this->params()[0].toInt(&ok);

            if (ok)
                oFrame = Blitz::sharpen(oFrame, radius);
        }
    }
    else if (this->method() == "smoothScale")
    {
        if (!this->params().isEmpty())
        {
            if (this->params()[0].canConvert<QSize>())
            {
                if (this->params().length() == 1)
                {
                    QSize sz = this->params()[0].toSize();

                    oFrame = Blitz::smoothScale(oFrame, sz);
                }
                else if (this->params().length() > 1)
                {
                    QSize sz = this->params()[0].toSize();
                    QString aspectRatioString = this->params()[1].toString();

                    bool ok = true;
                    Qt::AspectRatioMode aspectRatio;

                    if (this->m_stringToAspectRatioMode.contains(aspectRatioString))
                        aspectRatio = this->m_stringToAspectRatioMode[aspectRatioString];
                    else
                        ok = false;

                    if (ok)
                        oFrame = Blitz::smoothScale(oFrame, sz, aspectRatio);
                }
            }
            else if (this->params()[0].canConvert<int>())
            {
                if (this->params().length() == 2)
                {
                    bool ok;
                    int dw = this->params()[0].toInt(&ok);

                    if (ok)
                    {
                        int dh = this->params()[1].toInt(&ok);

                        if (ok)
                            oFrame = Blitz::smoothScale(oFrame, dw, dh);
                    }
                }
                else if (this->params().length() > 2)
                {
                    bool ok;
                    int dw = this->params()[0].toInt(&ok);

                    if (ok)
                    {
                        int dh = this->params()[1].toInt(&ok);

                        if (ok)
                        {
                            QString aspectRatioString = this->params()[2].toString();

                            Qt::AspectRatioMode aspectRatio;

                            if (this->m_stringToAspectRatioMode.contains(aspectRatioString))
                                aspectRatio = this->m_stringToAspectRatioMode[aspectRatioString];
                            else
                                ok = false;

                            if (ok)
                                oFrame = Blitz::smoothScale(oFrame, dw, dh, aspectRatio);
                        }
                    }
                }
            }
        }
    }
    else if (this->method() == "smoothScaleFilter")
    {
        if (!this->params().isEmpty())
        {
            if (this->params()[0].canConvert<QSize>())
            {
                if (this->params().length() == 1)
                {
                    QSize sz = this->params()[0].toSize();

                    oFrame = Blitz::smoothScaleFilter(oFrame, sz);
                }
                else if (this->params().length() == 2)
                {
                    QSize sz = this->params()[0].toSize();
                    bool ok;
                    float blur = this->params()[1].toFloat(&ok);

                    if (ok)
                        oFrame = Blitz::smoothScaleFilter(oFrame, sz, blur);
                }
                else if (this->params().length() == 3)
                {
                    QSize sz = this->params()[0].toSize();
                    bool ok;
                    float blur = this->params()[1].toFloat(&ok);

                    if (ok)
                    {
                        QString filterString = this->params()[2].toString();

                        Blitz::ScaleFilterType filter;

                        if (this->m_stringToScaleFilterType.contains(filterString))
                            filter = this->m_stringToScaleFilterType[filterString];
                        else
                            ok = false;

                        if (ok)
                            oFrame = Blitz::smoothScaleFilter(oFrame, sz, blur, filter);
                    }
                }
                else if (this->params().length() > 3)
                {
                    QSize sz = this->params()[0].toSize();
                    bool ok;
                    float blur = this->params()[1].toFloat(&ok);

                    if (ok)
                    {
                        QString filterString = this->params()[2].toString();

                        Blitz::ScaleFilterType filter;

                        if (this->m_stringToScaleFilterType.contains(filterString))
                            filter = this->m_stringToScaleFilterType[filterString];
                        else
                            ok = false;

                        if (ok)
                        {
                            QString aspectRatioString = this->params()[3].toString();

                            Qt::AspectRatioMode aspectRatio;

                            if (this->m_stringToAspectRatioMode.contains(aspectRatioString))
                                aspectRatio = this->m_stringToAspectRatioMode[aspectRatioString];
                            else
                                ok = false;

                            oFrame = Blitz::smoothScaleFilter(oFrame, sz, blur, filter, aspectRatio);
                        }
                    }
                }
            }
            else if (this->params()[0].canConvert<int>())
            {
                if (this->params().length() == 2)
                {
                    bool ok;
                    int dwX = this->params()[0].toInt(&ok);

                    if (ok)
                    {
                        int dwY = this->params()[1].toInt(&ok);

                        if (ok)
                            oFrame = Blitz::smoothScaleFilter(oFrame, dwX, dwY);
                    }
                }
                else if (this->params().length() == 3)
                {
                    bool ok;
                    int dwX = this->params()[0].toInt(&ok);

                    if (ok)
                    {
                        int dwY = this->params()[1].toInt(&ok);

                        if (ok)
                        {
                            float blur = this->params()[2].toFloat(&ok);

                            if (ok)
                                oFrame = Blitz::smoothScaleFilter(oFrame, dwX, dwY, blur);
                        }
                    }
                }
                else if (this->params().length() == 4)
                {
                    bool ok;
                    int dwX = this->params()[0].toInt(&ok);

                    if (ok)
                    {
                        int dwY = this->params()[1].toInt(&ok);

                        if (ok)
                        {
                            float blur = this->params()[2].toFloat(&ok);

                            if (ok)
                            {
                                QString filterString = this->params()[3].toString();

                                Blitz::ScaleFilterType filter;

                                if (this->m_stringToScaleFilterType.contains(filterString))
                                    filter = this->m_stringToScaleFilterType[filterString];
                                else
                                    ok = false;

                                if (ok)
                                    oFrame = Blitz::smoothScaleFilter(oFrame, dwX, dwY, blur, filter);
                            }
                        }
                    }
                }
                else if (this->params().length() > 4)
                {
                    bool ok;
                    int dwX = this->params()[0].toInt(&ok);

                    if (ok)
                    {
                        int dwY = this->params()[1].toInt(&ok);

                        if (ok)
                        {
                            float blur = this->params()[2].toFloat(&ok);

                            if (ok)
                            {
                                QString filterString = this->params()[3].toString();

                                Blitz::ScaleFilterType filter;

                                if (this->m_stringToScaleFilterType.contains(filterString))
                                    filter = this->m_stringToScaleFilterType[filterString];
                                else
                                    ok = false;

                                if (ok)
                                {
                                    QString aspectRatioString = this->params()[3].toString();

                                    Qt::AspectRatioMode aspectRatio;

                                    if (this->m_stringToAspectRatioMode.contains(aspectRatioString))
                                        aspectRatio = this->m_stringToAspectRatioMode[aspectRatioString];
                                    else
                                        ok = false;

                                    if (ok)
                                        oFrame = Blitz::smoothScaleFilter(oFrame, dwX, dwY, blur, filter, aspectRatio);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (this->method() == "swirl")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::swirl(oFrame);
        else
        {
            bool ok;
            float degrees = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::swirl(oFrame, degrees);
        }
    }
    else if (this->method() == "threshold")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::threshold(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            int thresholdValue = this->params()[0].toInt(&ok);

            if (ok)
                oFrame = Blitz::threshold(oFrame, thresholdValue);
        }
        else if (this->params().length() == 2)
        {
            bool ok;
            int thresholdValue = this->params()[0].toInt(&ok);

            if (ok)
            {
                QString channelString = this->params()[1].toString();

                bool ok = true;
                Blitz::RGBChannel channel;

                if (this->m_stringToRGBChannel.contains(channelString))
                    channel = this->m_stringToRGBChannel[channelString];
                else
                    ok = false;

                if (ok)
                    oFrame = Blitz::threshold(oFrame, thresholdValue, channel);
            }
        }
        else if (this->params().length() == 3)
        {
            bool ok;
            int thresholdValue = this->params()[0].toInt(&ok);

            if (ok)
            {
                QString channelString = this->params()[1].toString();

                bool ok = true;
                Blitz::RGBChannel channel;

                if (this->m_stringToRGBChannel.contains(channelString))
                    channel = this->m_stringToRGBChannel[channelString];
                else
                    ok = false;

                if (ok)
                {
                    QColor ac = this->params()[2].value<QColor>();
                    QRgb aboveColor = qRgb(ac.red(), ac.green(), ac.blue());

                    oFrame = Blitz::threshold(oFrame, thresholdValue, channel, aboveColor);
                }
            }
        }
        else if (this->params().length() > 3)
        {
            bool ok;
            int thresholdValue = this->params()[0].toInt(&ok);

            if (ok)
            {
                QString channelString = this->params()[1].toString();

                bool ok = true;
                Blitz::RGBChannel channel;

                if (this->m_stringToRGBChannel.contains(channelString))
                    channel = this->m_stringToRGBChannel[channelString];
                else
                    ok = false;

                if (ok)
                {
                    QColor ac = this->params()[2].value<QColor>();
                    QRgb aboveColor = qRgb(ac.red(), ac.green(), ac.blue());

                    QColor bc = this->params()[3].value<QColor>();
                    QRgb belowColor = qRgb(bc.red(), bc.green(), bc.blue());

                    oFrame = Blitz::threshold(oFrame, thresholdValue, channel, aboveColor, belowColor);
                }
            }
        }
    }
    else if (this->method() == "unbalancedGradient")
    {
        if (this->params().length() == 4)
        {
            QSize size = this->params()[0].toSize();
            QColor ca = this->params()[1].value<QColor>();
            QColor cb = this->params()[2].value<QColor>();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
                oFrame = Blitz::unbalancedGradient(size, ca, cb, type);
        }
        else if (this->params().length() == 5)
        {
            QSize size = this->params()[0].toSize();
            QColor ca = this->params()[1].value<QColor>();
            QColor cb = this->params()[2].value<QColor>();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
            {
                int xfactor = this->params()[4].toInt(&ok);

                if (ok)
                    oFrame = Blitz::unbalancedGradient(size, ca, cb, type, xfactor);
            }
        }
        else if (this->params().length() > 5)
        {
            QSize size = this->params()[0].toSize();
            QColor ca = this->params()[1].value<QColor>();
            QColor cb = this->params()[2].value<QColor>();
            QString typeString = this->params()[3].toString();

            bool ok = true;
            Blitz::GradientType type;

            if (this->m_stringToGradientType.contains(typeString))
                type = this->m_stringToGradientType[typeString];
            else
                ok = false;

            if (ok)
            {
                int xfactor = this->params()[4].toInt(&ok);

                if (ok)
                {
                    int yfactor = this->params()[5].toInt(&ok);

                    if (ok)
                        oFrame = Blitz::unbalancedGradient(size, ca, cb, type, xfactor, yfactor);
                }
            }
        }
    }
    else if (this->method() == "wave")
    {
        if (this->params().isEmpty())
            oFrame = Blitz::wave(oFrame);
        else if (this->params().length() == 1)
        {
            bool ok;
            float amplitude = this->params()[0].toFloat(&ok);

            if (ok)
                oFrame = Blitz::wave(oFrame, amplitude);
        }
        else if (this->params().length() == 2)
        {
            bool ok;
            float amplitude = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float frequency = this->params()[1].toFloat(&ok);

                if (ok)
                    oFrame = Blitz::wave(oFrame, amplitude, frequency);
            }
        }
        else if (this->params().length() > 2)
        {
            bool ok;
            float amplitude = this->params()[0].toFloat(&ok);

            if (ok)
            {
                float frequency = this->params()[1].toFloat(&ok);

                if (ok)
                {
                    QColor bc = this->params()[2].value<QColor>();
                    QRgb background = qRgb(bc.red(), bc.green(), bc.blue());

                    oFrame = Blitz::wave(oFrame, amplitude, frequency, background);
                }
            }
        }
    }

    if (!this->m_qtToFormat.contains(oFrame.format()))
        oFrame = oFrame.convertToFormat(QImage::Format_ARGB32);

    QSharedPointer<uchar> oBuffer(new uchar[oFrame.byteCount()]);
    memcpy(oBuffer.data(), oFrame.constBits(), oFrame.byteCount());

    QbCaps caps(packet.caps());
    caps.setProperty("format", this->m_qtToFormat[oFrame.format()]);
    caps.setProperty("width", oFrame.width());
    caps.setProperty("height", oFrame.height());

    QbPacket oPacket(caps,
                     oBuffer,
                     oFrame.byteCount());

    oPacket.setDts(packet.dts());
    oPacket.setPts(packet.pts());
    oPacket.setDuration(packet.duration());
    oPacket.setTimeBase(packet.timeBase());
    oPacket.setIndex(packet.index());

    emit this->oStream(oPacket);
}
