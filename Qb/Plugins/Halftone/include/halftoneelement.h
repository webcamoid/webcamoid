/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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

#ifndef HALFTONEELEMENT_H
#define HALFTONEELEMENT_H

#include <QImage>
#include <QColor>

#include <qb.h>

class HalftoneElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString pattern READ pattern WRITE setPattern RESET resetPattern)

    Q_PROPERTY(QSize patternSize READ patternSize
                                 WRITE setPatternSize
                                 RESET resetPatternSize)

    Q_PROPERTY(float lightness READ lightness WRITE setLightness RESET resetLightness)
    Q_PROPERTY(float slope READ slope WRITE setSlope RESET resetSlope)
    Q_PROPERTY(float intercept READ intercept WRITE setIntercept RESET resetIntercept)

    public:
        explicit HalftoneElement();
        Q_INVOKABLE QString pattern() const;
        Q_INVOKABLE QSize patternSize() const;
        Q_INVOKABLE float lightness() const;
        Q_INVOKABLE float slope() const;
        Q_INVOKABLE float intercept() const;

    private:
        QString m_pattern;
        QSize m_patternSize;
        float m_lightness;
        float m_slope;
        float m_intercept;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QImage m_patternImage;
        quint8 *m_patternBits;

    public slots:
        void setPattern(const QString &pattern);
        void setPatternSize(const QSize &patternSize);
        void setLightness(float lightness);
        void setSlope(float slope);
        void setIntercept(float intercept);
        void resetPattern();
        void resetPatternSize();
        void resetLightness();
        void resetSlope();
        void resetIntercept();
        void iStream(const QbPacket &packet);
        void setState(QbElement::ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // HALFTONEELEMENT_H
