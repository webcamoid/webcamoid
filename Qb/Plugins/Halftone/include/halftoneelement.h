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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef HALFTONEELEMENT_H
#define HALFTONEELEMENT_H

#include <QColor>
#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

class HalftoneElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString pattern
               READ pattern
               WRITE setPattern
               RESET resetPattern
               NOTIFY patternChanged)
    Q_PROPERTY(QSize patternSize
               READ patternSize
               WRITE setPatternSize
               RESET resetPatternSize
               NOTIFY patternSizeChanged)
    Q_PROPERTY(qreal lightness
               READ lightness
               WRITE setLightness
               RESET resetLightness
               NOTIFY lightnessChanged)
    Q_PROPERTY(qreal slope
               READ slope
               WRITE setSlope
               RESET resetSlope
               NOTIFY slopeChanged)
    Q_PROPERTY(qreal intercept
               READ intercept
               WRITE setIntercept
               RESET resetIntercept
               NOTIFY interceptChanged)

    public:
        explicit HalftoneElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE QString pattern() const;
        Q_INVOKABLE QSize patternSize() const;
        Q_INVOKABLE qreal lightness() const;
        Q_INVOKABLE qreal slope() const;
        Q_INVOKABLE qreal intercept() const;

    private:
        QString m_pattern;
        QSize m_patternSize;
        qreal m_lightness;
        qreal m_slope;
        qreal m_intercept;

        QbElementPtr m_convert;
        QImage m_patternImage;

        QImage loadPattern(const QString &patternFile, const QSize &size) const;

    signals:
        void patternChanged();
        void patternSizeChanged();
        void lightnessChanged();
        void slopeChanged();
        void interceptChanged();

    public slots:
        void setPattern(const QString &pattern);
        void setPatternSize(const QSize &patternSize);
        void setLightness(qreal lightness);
        void setSlope(qreal slope);
        void setIntercept(qreal intercept);
        void resetPattern();
        void resetPatternSize();
        void resetLightness();
        void resetSlope();
        void resetIntercept();
        QbPacket iStream(const QbPacket &packet);
};

#endif // HALFTONEELEMENT_H
