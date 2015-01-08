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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef MATRIXELEMENT_H
#define MATRIXELEMENT_H

#include <QQmlComponent>
#include <QQmlContext>
#include <qb.h>
#include <qbutils.h>

#include "blip.h"

class MatrixElement: public QbElement
{
    Q_OBJECT
    Q_ENUMS(BlipMode)
    Q_PROPERTY(int nChars
               READ nChars
               WRITE setNChars
               RESET resetNChars
               NOTIFY nCharsChanged)
    Q_PROPERTY(QSize fontSize
               READ fontSize
               WRITE setFontSize
               RESET resetFontSize
               NOTIFY fontSizeChanged)
    Q_PROPERTY(int fontDepth
               READ fontDepth
               WRITE setFontDepth
               RESET resetFontDepth
               NOTIFY fontDepthChanged)
    Q_PROPERTY(int mode
               READ mode
               WRITE setMode
               RESET resetMode
               NOTIFY modeChanged)
    Q_PROPERTY(qreal white
               READ white
               WRITE setWhite
               RESET resetWhite
               NOTIFY whiteChanged)
    Q_PROPERTY(bool pause
               READ pause
               WRITE setPause
               RESET resetPause
               NOTIFY pauseChanged)

    public:
        enum BlipMode
        {
            BlipModeNone,
            BlipModeFall,
            BlipModeStop,
            BlipModeSlid
        };

        explicit MatrixElement();

        Q_INVOKABLE QObject *controlInterface(QQmlEngine *engine,
                                              const QString &controlId) const;

        Q_INVOKABLE int nChars() const;
        Q_INVOKABLE QSize fontSize() const;
        Q_INVOKABLE int fontDepth() const;
        Q_INVOKABLE int mode() const;
        Q_INVOKABLE qreal white() const;
        Q_INVOKABLE bool pause() const;

    private:
        int m_nChars;
        QSize m_fontSize;
        int m_fontDepth;
        int m_mode;
        qreal m_white;
        bool m_pause;

        QbElementPtr m_convert;
        QbCaps m_caps;
        QSize m_mapSize;
        QImage m_matrixFont;
        QImage m_cmap;
        QImage m_vmap;
        QImage m_img;
        QVector<Blip> m_blips;
        QByteArray m_font;
        QVector<quint32> m_palette;

        quint32 green(uint v);
        QByteArray createPattern(int nChars, const QSize &fontSize);
        void setPalette();
        void darkenColumn(int x);
        void blipNone(int x);
        void blipFall(int x);
        void blipStop(int x);
        void blipSlide(int x);
        void updateCharMap();
        void createImg(QImage &src);
        void drawChar(quint32 *dest, uchar c, uchar v, QSize size);

    signals:
        void nCharsChanged();
        void fontSizeChanged();
        void fontDepthChanged();
        void modeChanged();
        void whiteChanged();
        void pauseChanged();

    public slots:
        void setNChars(int nChars);
        void setFontSize(const QSize &fontSize);
        void setFontDepth(int fontDepth);
        void setMode(int mode);
        void setWhite(qreal white);
        void setPause(bool pause);
        void resetNChars();
        void resetFontSize();
        void resetFontDepth();
        void resetMode();
        void resetWhite();
        void resetPause();

        QbPacket iStream(const QbPacket &packet);
};

#endif // MATRIXELEMENT_H
