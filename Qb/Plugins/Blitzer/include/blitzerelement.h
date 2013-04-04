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

#ifndef BLITZERELEMENT_H
#define BLITZERELEMENT_H

#include <QtGui>
#include <qb.h>
#include <qimageblitz/qimageblitz.h>

class BlitzerElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString method READ method WRITE setMethod RESET resetMethod)
    Q_PROPERTY(QVariantList params READ params WRITE setParams RESET resetParams)

    public:
        explicit BlitzerElement();
        ~BlitzerElement();

        Q_INVOKABLE QString method() const;
        Q_INVOKABLE QVariantList params() const;

    private:
        QString m_method;
        QVariantList m_params;
        QbElementPtr m_convert;
        QImage m_oFrame;
        QMap<QString, Blitz::RGBChannel> m_stringToRGBChannel;
        QMap<QString, Blitz::EffectQuality> m_stringToEffectQuality;
        QMap<QString, Blitz::GradientType> m_stringToGradientType;
        QMap<QString, QImage::InvertMode> m_stringToInvertMode;
        QMap<QString, Blitz::ModulationType> m_stringToModulationType;
        QMap<QString, Qt::AspectRatioMode> m_stringToAspectRatioMode;
        QMap<QString, Blitz::ScaleFilterType> m_stringToScaleFilterType;
        QMap<QImage::Format, QString> m_qtToFormat;

    public slots:
        void setMethod(QString method);
        void setParams(QVariantList params);
        void resetMethod();
        void resetParams();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // BLITZERELEMENT_H
