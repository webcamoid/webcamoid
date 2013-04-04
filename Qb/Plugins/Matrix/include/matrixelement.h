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

#ifndef MATRIXELEMENT_H
#define MATRIXELEMENT_H

#include <QtGui>
#include <qb.h>

#include "blip.h"

class MatrixElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(BlipMode)
    Q_PROPERTY(int nChars READ nChars WRITE setNChars RESET resetNChars)
    Q_PROPERTY(int fontWidth READ fontWidth WRITE setFontWidth RESET resetFontWidth)
    Q_PROPERTY(int fontHeight READ fontHeight WRITE setFontHeight RESET resetFontHeight)
    Q_PROPERTY(int fontDepth READ fontDepth WRITE setFontDepth RESET resetFontDepth)
    Q_PROPERTY(int mode READ mode WRITE setMode RESET resetMode)
    Q_PROPERTY(float white READ white WRITE setWhite RESET resetWhite)
    Q_PROPERTY(bool pause READ pause WRITE setPause RESET resetPause)

    public:
        enum BlipMode
        {
            BlipModeNone,
            BlipModeFall,
            BlipModeStop,
            BlipModeSlid
        };

        explicit MatrixElement();
        ~MatrixElement();

        Q_INVOKABLE int nChars() const;
        Q_INVOKABLE int fontWidth() const;
        Q_INVOKABLE int fontHeight() const;
        Q_INVOKABLE int fontDepth() const;
        Q_INVOKABLE int mode() const;
        Q_INVOKABLE float white() const;
        Q_INVOKABLE bool pause() const;

        bool event(QEvent *e);

    private:
        int m_nChars;
        int m_fontWidth;
        int m_fontHeight;
        int m_fontDepth;
        int m_mode;
        float m_white;
        bool m_pause;

        QbElementPtr m_convert;
        QImage m_oFrame;
        QbCaps m_caps;
        int m_mapWidth;
        int m_mapHeight;
        QImage m_matrixFont;
        QImage m_cmap;
        QImage m_vmap;
        QImage m_img;
        QVector<Blip> m_blips;
        QByteArray m_font;
        QVector<quint32> m_palette;

        quint32 green(uint v);
        void setPattern();
        void setPalette();
        void darkenColumn(int x);
        void blipNone(int x);
        void blipFall(int x);
        void blipStop(int x);
        void blipSlide(int x);
        void updateCharMap();
        void createImg(QImage &src);
        void drawChar(quint32 *dest, uchar c, uchar v, QSize size);

    public slots:
        void setNChars(int nChars);
        void setFontWidth(int fontWidth);
        void setFontHeight(int fontHeight);
        void setFontDepth(int fontDepth);
        void setMode(int mode);
        void setWhite(float white);
        void setPause(bool pause);
        void resetNChars();
        void resetFontWidth();
        void resetFontHeight();
        void resetFontDepth();
        void resetMode();
        void resetWhite();
        void resetPause();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet);
};

#endif // MATRIXELEMENT_H
