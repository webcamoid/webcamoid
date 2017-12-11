/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef PHOTOCOPYELEMENT_H
#define PHOTOCOPYELEMENT_H

#include <akelement.h>

class PhotocopyElement: public AkElement
{
    Q_OBJECT
    Q_PROPERTY(qreal brightness
               READ brightness
               WRITE setBrightness
               RESET resetBrightness
               NOTIFY brightnessChanged)
    Q_PROPERTY(qreal contrast
               READ contrast
               WRITE setContrast
               RESET resetContrast
               NOTIFY contrastChanged)

    public:
        explicit PhotocopyElement();

        Q_INVOKABLE qreal brightness() const;
        Q_INVOKABLE qreal contrast() const;

    private:
        qreal m_brightness;
        qreal m_contrast;

        inline int rgbToLuma(int red, int green, int blue)
        {
            int min;
            int max;

            if (red > green) {
                max = qMax(red, blue);
                min = qMin(green, blue);
            } else {
                max = qMax(green, blue);
                min = qMin(red, blue);
            }

            return qRound((max + min) / 2.0);
        }

    protected:
        QString controlInterfaceProvide(const QString &controlId) const;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const;

    signals:
        void brightnessChanged(qreal brightness);
        void contrastChanged(qreal contrast);

    public slots:
        void setBrightness(qreal brightness);
        void setContrast(qreal contrast);
        void resetBrightness();
        void resetContrast();

        AkPacket iStream(const AkPacket &packet);
};

#endif // PHOTOCOPYELEMENT_H
