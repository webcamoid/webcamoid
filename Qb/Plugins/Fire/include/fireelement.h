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

#ifndef FIREELEMENT_H
#define FIREELEMENT_H

#include <QtGui>
#include <qb.h>

class FireElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(int mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(int decay READ decay WRITE setDecay RESET resetDecay)
    Q_PROPERTY(int threshold READ threshold WRITE setThreshold RESET resetThreshold)
    Q_PROPERTY(int maxColor READ maxColor WRITE setMaxColor RESET resetMaxColor)

    public:
        explicit FireElement();
        ~FireElement();

        Q_INVOKABLE int mode();
        Q_INVOKABLE int decay();
        Q_INVOKABLE int threshold();
        Q_INVOKABLE int maxColor();

        bool event(QEvent *e);

    private:
        int m_mode;
        int m_decay;
        int m_threshold;
        int m_maxColor;

        QbCaps m_caps;
        QbElementPtr m_convert;
        QImage m_oFrame;
        QImage m_background;
        QImage m_diff;
        QByteArray m_buffer;
        QVector<quint32> m_palette;
        bool m_bgIsSet;

        int trunc(double f);
        void hsiToRgb(double h, double s, double i, int *r, int *g, int *b);
        void imageBgSetY(QImage &src);
        QImage imageBgSubtractY(QImage &src);
        void makePalette();
        void setBackground(QImage &src);

    protected:
        bool init();

    public slots:
        void setMode(int mode);
        void setDecay(int decay);
        void setThreshold(int threshold);
        void setMaxColor(int maxColor);
        void resetMode();
        void resetDecay();
        void resetThreshold();
        void resetMaxColor();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // FIREELEMENT_H
